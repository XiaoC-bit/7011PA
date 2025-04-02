// communicationthread.cpp
#include "communicationthread.h"
#include <Windows.h>
#include <qdebug.h>
#include <qhostaddress.h>
#include <qjsonobject.h>
#include <qnetworkproxy.h>

#include "NormalCommHandler.h"
#include "ControlTestCommHandler.h"
#include "PIDCommHandler.h"


CommunicationThread::CommunicationThread(QObject* parent) : QThread(parent), m_socket(nullptr), m_sendTimer(nullptr), recNum1_(0), powerOn_(true)
{

	commHandlers_["normal-message"] = new NormalCommHandler(u65Info, this);//常规通讯
	commHandlers_["control-message"] = new ControlTestCommHandler(u65Info, this);//常规通讯
	commHandlers_["pid-message"] = new PIDCommHandler(u65Info, this);//常规通讯

	useRealTime_ = false;

	lastSendRealData_ = 0;
}

CommunicationThread::~CommunicationThread()
{
	if (m_socket) {
		m_socket->deleteLater();
	}
	if (m_sendTimer) {
		m_sendTimer->deleteLater();
	}
	commHandlers_.clear();
}


void CommunicationThread::setDeviceId(int id) {
	deviceId_ = id;
}

QString CommunicationThread::ip() {
	return m_deviceAddress;
}

void CommunicationThread::run()
{
	qDebug() << QThread::currentThreadId();
	if (m_socket == nullptr) {
		m_socket = new QTcpSocket();
	}

	if (m_socket->state() != QAbstractSocket::ConnectedState) {
		m_socket->setProxy(QNetworkProxy::NoProxy);//对于开了VPN的机器，需要设置此项
#ifdef _DEBUG
	/*	m_socket->connectToHost(m_deviceAddress, m_port);
		if (!m_socket->waitForConnected()) {
			qDebug() << m_socket->errorString();
		}*/
#else
		m_socket->connectToHost(m_deviceAddress, m_port);
		if (!m_socket->waitForConnected()) {
			qDebug() << m_socket->errorString();
		}
#endif
	}

	if (m_sendTimer == nullptr) {
		//在子线程中创建对象，不要指定parent，否则会报错：在子线程中创建了子对象，而父对象却在另一个线程
		m_sendTimer = new QTimer();

		//不要直接建立信号槽函数到CommunicationThread，这样会在主线程执行
		// 我们需要在子线程中执行
	   // connect(m_sendTimer, &QTimer::timeout, this, &CommunicationThread::timerFunc);
		QObject::connect(m_sendTimer, &QTimer::timeout, [&]() {
			this->timerFunc();
			});

		connect(this, SIGNAL(startTimer()), m_sendTimer, SLOT(start()));
		connect(this, SIGNAL(stopTimer()), m_sendTimer, SLOT(stop()));
	}
	//每600ms读取一次数据，可能有其他通讯任务，将定时器间隔设置小一点
	//读取历史资料区的话，可以不用这么频繁，待优化
	m_sendTimer->setInterval(100);
	m_sendTimer->start();

	exec(); // Start event loop

	//退出循环前断开连接
	if (m_sendTimer->isActive())
		m_sendTimer->stop();
	m_socket->disconnectFromHost();
}

bool CommunicationThread::writeData(const QByteArray& data)
{
	if (!m_socket)
		return false;
	if (!m_socket->isOpen())
		return false;
	if (m_socket->state() != QAbstractSocket::ConnectedState)
		return false;
	if (m_socket && m_socket->isOpen()) {
		m_socket->write(data);
		return m_socket->waitForBytesWritten(); // Wait for data to be written
	}
	return false;
}

bool CommunicationThread::readData(QByteArray& data, int timeout) {
	if (!m_socket)
		return false;
	if (!m_socket->isOpen())
		return false;
	if (m_socket->state() != QAbstractSocket::ConnectedState)
		return false;
	if (!m_socket->waitForReadyRead(timeout))
		return false;
	data = m_socket->readAll();
	return true;
}


//更换ip地址
void CommunicationThread::setDeviceAddress(const QString& ip) {
	m_deviceAddress = ip;
}

void CommunicationThread::init(const QString& deviceAddress, quint16 port)
{
	m_deviceAddress = deviceAddress;
	m_port = port;
}


void CommunicationThread::normalTimerFunc() {
	QString strErr;
	QJsonObject obj;
	obj["__channel"] = "normal-message";
	obj["__type"] = "real-data";
	obj["__deviceId"] = deviceId_;
	bool ret = commHandlers_["normal-message"]->commFunc(this, obj, strErr);//与设备通讯，配置jsonObj
	if (!ret) {
		log(m_deviceAddress, strErr);
		return;
		//m_socket->disconnectFromHost();
	} 
	  
	//qDebug() << obj["machType"].toString() <<"ip " << m_deviceAddress << " dev id " << deviceId_;
	//设备通讯完毕，通知前端
	//emit wsResponse(obj);
	looseFireRealData(obj);

	//通知Processor,进行测试数据处理
	U65RawData info;
	//将通讯数据获取后，组合结构体，发到数据处理线程
	ReadU65Struct rawData = commHandlers_["normal-message"]->U65Info_;
	info.sStar = rawData.sStar;
	info.sQuotation = rawData.sQuotation;
	info.sDoubleQuotation = rawData.sDoubleQuotation;
	if (rawData.sQuotation == 0) {
		info.tanPA = 0;
	}
	else
		info.tanPA = rawData.sDoubleQuotation / rawData.sQuotation;// 自己计算tanPA
	info.angle = rawData.SITA * 3.14 / 180;
	info.P = rawData.AD_2;
	info.upperTemp = rawData.upperTemp;
	info.lowerTemp = rawData.lowerTemp;
	info.U65Info = rawData;

	if (!useRealTime_) {
		//通过读取资料区的方式,模拟采集
		return fakeData(info);
	}
	else {
		//通知数据处理线程
		emit fireRegularInfo(QVariant::fromValue(info));
	}
}

/**
 * .
 * 
 * \param info	从及时资料区读取的数据
 * \param fakeInfo 从历史资料区5000个点，读取的数据。重新填入fakeInfo，伪造数据给数据处理线程
 * \param readRec2 是否使用历史资料区的第二个区域
 * \return 
 */
bool CommunicationThread::readRecData(const U65RawData& info, U65RawData &fakeInfo, bool readRec2) {
	QString strErr;
	QJsonObject obj;

	if (!readRec2) {
		if (recNum1_ * pow(2.0, int(info.U65Info.REC_COMP)) < (5000 - 32))
		{
			obj["__channel"] = "normal-message";
			obj["__type"] = "read-data";
			obj["addr"] = 0x20000 + 32 * pow(2.0, int(info.U65Info.REC_COMP)) * (recNum1_ + 1);
			obj["length"] = 32;
			bool ret = commHandlers_["normal-message"]->commFunc(this, obj, strErr);//与设备通讯，配置jsonObj
			if (!ret) {
				log(m_deviceAddress, strErr);
				return false;
			}
		}
		else
		{
			int ttl, ttk;
			ttl = int(recNum1_ * pow(2.0, int(info.U65Info.REC_COMP))) % 5000;
			ttk = (recNum1_ * pow(2.0, int(info.U65Info.REC_COMP))) / 5000;
			if (ttl <= (5000 - pow(2.0, int(info.U65Info.REC_COMP))))
			{
				obj["__channel"] = "normal-message";
				obj["__type"] = "read-data";
				obj["addr"] = 0x20000 + 32 * (ttl + ttk);
				obj["length"] = 32;
				bool ret = commHandlers_["normal-message"]->commFunc(this, obj, strErr);//与设备通讯，配置jsonObj
				if (!ret) {
					log(m_deviceAddress, strErr);
					return false;
				}
			}
		}
	}
	else {

		obj["__channel"] = "normal-message";
		obj["__type"] = "read-data";
		obj["addr"] = 0x047120 + 32 * (recNum2_);
		obj["length"] = 32;
		bool ret = commHandlers_["normal-message"]->commFunc(this, obj, strErr);//与设备通讯，配置jsonObj
		if (!ret) {
			log(m_deviceAddress, strErr);
			return false;
		}

	}
	

	fakeInfo = info;
	fakeInfo.U65Info.TEST_TIMER = obj["testTimer"].toDouble();
	fakeInfo.sStar = obj["sStar"].toDouble();
	fakeInfo.sQuotation = obj["sQuotation"].toDouble();
	
	fakeInfo.sDoubleQuotation = obj["sDoubleQuotation"].toDouble();
	if (fakeInfo.sQuotation == 0) {
		fakeInfo.tanPA = 0;
	}
	else {
		fakeInfo.tanPA = obj["tanPA"].toDouble();
	}
	fakeInfo.angle = obj["angle"].toDouble();
	fakeInfo.P = obj["P"].toDouble();
	fakeInfo.upperTemp = obj["upperTemp"].toDouble();
	fakeInfo.lowerTemp = obj["lowerTemp"].toDouble();
	fakeInfo.stepNo = obj["stepNo"].toInt();
	/*qDebug() << "fakeInfo.upperTemp " << fakeInfo.upperTemp;
	qDebug() << "fakeInfo.lowerTemp " << fakeInfo.lowerTemp;*/

	//设置关门\合模的状态
	fakeInfo.U65Info.U65_MSG3 |= (1 << 9);
	fakeInfo.U65Info.IO1_IN &= ~(1 << 2);

	//设置为测试中
	fakeInfo.U65Info.U65_MODE = 3;
	fakeInfo.U65Info.U65_MSG = 11;

	return true;
}

void CommunicationThread::fakeData(const U65RawData& info) {
	//待测模式
	if (info.U65Info.U65_MODE == 2) {		
		if (recNum1_ != 0) {
			while (info.U65Info.REC_NO1 > recNum1_)
			{
				U65RawData fakeInfo;
				if (!readRecData(info, fakeInfo))
					return;
				//通知数据处理线程
				emit fireRegularInfo(QVariant::fromValue(fakeInfo));
				recNum1_++;
			}

			//结束测试，直接转发
			emit fireRegularInfo(QVariant::fromValue(info));

		}
		recNum1_ = 0;
		recNum2_ = 0;
	}
	else if (info.U65Info.U65_MODE == 3) {

		if (info.U65Info.U65_MSG == 7) {			
			//等待送料，这里与旧软件逻辑不同。
			if (recNum1_ != 0) {
				while (info.U65Info.REC_NO1 > recNum1_)
				{
					U65RawData fakeInfo;
					if (!readRecData(info, fakeInfo))
						return;
					//通知数据处理线程
					emit fireRegularInfo(QVariant::fromValue(fakeInfo));
					recNum1_++;
				}
			}			

			emit fireRegularInfo(QVariant::fromValue(info));
			recNum1_ = 0;
			recNum2_ = 0;
			return;
		}

		if (info.U65Info.U65_MSG < 10) 
			return;
		if (recNum1_ == 0) {
			
			//首次获取到测试数据，发送几个虚拟的消息

			U65RawData fakeInfo;
			fakeInfo.U65Info.U65_MODE = 3;
			fakeInfo.U65Info.U65_MSG = 11;
			fakeInfo.U65Info.AD_2 = 1;
			//测试中
			emit fireRegularInfo(QVariant::fromValue(fakeInfo));

			//模具打开
			fakeInfo.U65Info.AD_2 = 2;
			fakeInfo.U65Info.IO1_IN |= (1 << 2);
			emit fireRegularInfo(QVariant::fromValue(fakeInfo));
			
			//模具关闭
			fakeInfo.U65Info.AD_2 = 3;
			fakeInfo.U65Info.IO1_IN &= ~(1 << 2);
			emit fireRegularInfo(QVariant::fromValue(fakeInfo));

			//正常数据
			if (!readRecData(info, fakeInfo))
				return;
			emit fireRegularInfo(QVariant::fromValue(fakeInfo));
			recNum1_++;
			return;
		}

		for (int index = 0; index < 20; index++) {

			if (info.U65Info.REC_NO1 > recNum1_) {
				U65RawData fakeInfo;
				if (!readRecData(info, fakeInfo))
					return;
				//通知数据处理线程
				emit fireRegularInfo(QVariant::fromValue(fakeInfo));
				recNum1_++;
			}
			else if (info.U65Info.REC_NO1 < recNum1_) {
				//旧软件有这部分逻辑，拷贝过来。但正常情况不应该走到这里，只是为了记录异常情况进行分析。
				qDebug() << "recNum1_ reset";
				recNum1_ = info.U65Info.REC_NO1;
			}
			else {
				if (info.U65Info.REC_NO2 > recNum2_) {
					U65RawData fakeInfo;
					if (!readRecData(info, fakeInfo,true))
						return;
					//通知数据处理线程
					emit fireRegularInfo(QVariant::fromValue(fakeInfo));
					recNum2_++;
				}
				else {
					break;
				}
			}
		}

		
	}
	else {
		if (recNum1_ != 0) {
			recNum1_ = 0;
			recNum2_ = 0;
			qDebug() << info.U65Info.U65_MODE;
		}		
	}
}


void CommunicationThread::useRealTime(bool use) {
	useRealTime_ = use;
}

void CommunicationThread::handleWsRequest(const QJsonObject& obj) {
	QString channel = obj["__channel"].toString();
	qDebug() << channel;
	qDebug()<< obj["__type"].toString();
	qDebug() << ip();
	if (channel == "control-comm-system") {
		ctrlMtx_.lock();
		commCtrlQueue_.push_back(obj);
		ctrlMtx_.unlock();
	}
	else {
		writeMtx_.lock();
		writeQueue_.push_back(obj);
		writeMtx_.unlock();
	}
}


//处理通讯控制消息
void CommunicationThread::handleCommCtrlMsg(const QJsonObject& recvObj) {
	const QString channel = "control-comm-system";
	auto type = recvObj["__type"].toString();
	QJsonObject outObj;
	outObj["__channel"] = channel;
	outObj["__type"] = type;
	if (type == "getIp") {
		outObj["ip"] = m_deviceAddress;
	}
	else if (type == "disconnect") {
		powerOn_ = false;
	}
	else if (type == "connect") {
		m_socket->disconnect();//断开连接
		setDeviceAddress(recvObj["ip"].toString());
		powerOn_ = true;
	}


	emit wsResponse(outObj);
}

void CommunicationThread::otherTimerFunc() {
	qDebug() << "otherTimerFunc";
	//取出待发送数据
	writeMtx_.lock();
	QJsonObject obj = writeQueue_.front();
	writeQueue_.pop_front();
	writeMtx_.unlock();

	QString channel = obj["__channel"].toString();

	auto it = commHandlers_.find(channel);
	if (it == commHandlers_.end()) {
		return;
	}
	QString strErr;
	bool ret = it.value()->commFunc(this, obj, strErr);//与设备通讯，配置jsonObj
	if (!ret) {

		QString type = obj["__type"].toString();
		qDebug()<<"Error channel\t" <<  channel<<"\ttype\t" << type;
		log(m_deviceAddress, strErr);
		return;
		// m_socket->disconnectFromHost();
	}

	//设备通讯完毕，通知前端
	emit wsResponse(obj);
}
#ifdef _DEBUG
static int __realTimeTest = 0;
static int __historyTest = 1;
#endif // DEBUG

void CommunicationThread::timerFunc()
{

#ifdef _DEBUG
	__historyTest = 0;//历史资料区测试
	if (__historyTest) {
		__historyTest--;
		U65RawData info;
		//将通讯数据获取后，组合结构体，发到数据处理线程
		ReadU65Struct rawData;
		info.sStar = rawData.sStar;
		info.sQuotation = rawData.sQuotation;
		info.sDoubleQuotation = rawData.sDoubleQuotation;
		info.tanPA = rawData.sDoubleQuotation / rawData.sQuotation;// 自己计算tanPA
		info.angle = rawData.SITA * 3.14 / 180;
		info.P = rawData.AD_2;
		info.upperTemp = rawData.upperTemp;
		info.lowerTemp = rawData.lowerTemp;
		info.U65Info = rawData;

		info.U65Info.U65_MODE = 3;
		info.U65Info.U65_MSG = 11;
		

		fakeData(info);
	}
	
	//及时资料区测试
	int timeout = 6; 
	//用于测试外部测试队列
	/*if (!writeQueue_.empty()) {
		writeQueue_.pop_back();
		__realTimeTest = 0;
	}*/
	//__realTimeTest = 0;
	if (__realTimeTest++ < 1) {
		//打开这里,关闭程序的时候,会异常
		//通知Processor,进行测试数据处理

		float time = 0;
		{
			U65RawData info;
			//	info.u65Mode = U65_MODE::INIT;
			info.sStar = 5 + __realTimeTest;
			info.tanPA = 7 + __realTimeTest;
			info.lowerTemp = __realTimeTest + 1;
			info.upperTemp = __realTimeTest + 2;
			info.U65Info.TEST_TIMER = time;
			info.U65Info.U65_MODE = 0;
			info.U65Info.U65_MSG3 = 512;

			info.twistingData.torque = 1.1 + __realTimeTest;
			info.twistingData.angle = __realTimeTest;
			info.twistingData.axialDisplacement = __realTimeTest;


			QThread::msleep(10);
			emit fireRegularInfo(QVariant::fromValue(info));
		}

		time += 0.6;
		{
			U65RawData info;
			//	info.u65Mode = U65_MODE::TESTING;//测试中
			info.sStar = 5 + __realTimeTest;
			info.tanPA = 7 + __realTimeTest;
			info.lowerTemp = __realTimeTest + 1;
			info.upperTemp = __realTimeTest + 2;
			info.U65Info.TEST_TIMER = time;
			info.U65Info.IO1_IN = 4;//打开模具
			info.U65Info.U65_MSG3 = 512;
			info.U65Info.U65_MODE = 11;

			info.twistingData.torque = 1.1 + __realTimeTest;
			info.twistingData.angle = __realTimeTest;
			info.twistingData.axialDisplacement = __realTimeTest;
			emit fireRegularInfo(QVariant::fromValue(info));
		}

		for (int i = 0; i < 500; i++)
		{
			time += 0.6;
			for (int j = 0; j < 1; j++) {
				U65RawData info;
				//info.u65Mode = U65_MODE::TESTING;
				info.tanPA = 7 + __realTimeTest;
				info.angle = 10 + __realTimeTest;
				info.sStar = 5 + time;
				info.sQuotation = 10 + time;
				info.sDoubleQuotation = 20 + time;
				if (i == 30) {
					info.sStar = 10;
				}
				info.lowerTemp = __realTimeTest + 1;
				info.upperTemp = __realTimeTest + 2;
				info.U65Info.TEST_TIMER = time;
				info.U65Info.IO1_IN = 0;// 关闭模具
				info.U65Info.U65_MSG3 = 512;
				info.U65Info.U65_MODE = 11;

				info.twistingData.torque = 1.1 + i;
				info.twistingData.angle = i;
				info.twistingData.axialDisplacement = 10.2+i;

				QThread::msleep(1000);
				emit fireRegularInfo(QVariant::fromValue(info));
			}
			QThread::msleep(timeout);
		}


		{
			time += 0.6;
			U65RawData info;
			//	info.u65Mode = U65_MODE::TESTING;
			info.sStar = 5 + __realTimeTest;
			info.tanPA = 7 + __realTimeTest;
			info.lowerTemp = __realTimeTest + 1;
			info.upperTemp = __realTimeTest + 2;
			info.U65Info.TEST_TIMER = time;
			info.U65Info.U65_MODE = 0;
			info.U65Info.IO1_IN = 4;//打开模具 准备结算



			info.twistingData.torque = 1.1 + __realTimeTest;
			info.twistingData.angle = __realTimeTest;
			info.twistingData.axialDisplacement = __realTimeTest;


			emit fireRegularInfo(QVariant::fromValue(info));
		}

		__realTimeTest = 1;

	}
#endif

	if (!commCtrlQueue_.empty()) {
		ctrlMtx_.lock();
		QJsonObject obj = commCtrlQueue_.front();
		commCtrlQueue_.pop_front();
		ctrlMtx_.unlock();
		return handleCommCtrlMsg(obj);
	}

	if (!powerOn_) {

		//断开连接时，也需要通知前端
		QJsonObject obj;
		obj["__channel"] = "normal-message";
		obj["__type"] = "real-data";
		obj["__deviceId"] = deviceId_;
		obj["connectErr"] = true;
		looseFireRealData(obj);

		return;
	}
	//检查套接字状态
	if (m_socket->state() != QAbstractSocket::ConnectedState) {
		QJsonObject obj;
		obj["__channel"] = "normal-message";
		obj["__type"] = "real-data";
		obj["__deviceId"] = deviceId_;
#ifdef _DEBUG
		//用来测试单位显示
		obj["connectErr"] = false;
		obj["sStar"] = 10;
		obj["sQuotation"] = 10;
		obj["sDoubleQuotation"] = 10;
		obj["upperTemp"] = 10;
		obj["lowerTemp"] = 10;

		obj["torque"] = 1.1;//扭转机  扭矩
		obj["angle"] = 2.2;//扭转机  扭矩
		obj["axialDisplacement"] = 3.3;//扭转机  轴向位移

		obj["twistCount"] = 4;//扭转机  轴向位移
		obj["testTimer"] = 3389;//扭转机  轴向位移
		
#else
		obj["connectErr"] = true;
#endif
		
		looseFireRealData(obj);

#ifdef _DEBUG
		//m_socket->connectToHost(m_deviceAddress, m_port);
		//if (!m_socket->waitForConnected()) {
		//	//log(m_deviceAddress, m_socket->errorString());
		//}
#else
		m_socket->connectToHost(m_deviceAddress, m_port);
		if (!m_socket->waitForConnected()) {
			//log(m_deviceAddress, m_socket->errorString());
		}
#endif

		if (!writeQueue_.empty()) {
			QJsonObject obj;
			obj["__channel"] = "global";
			obj["__type"] = "message";
			obj["type"] = "error";
			obj["message"] = "can not connect to machine";
			emit wsResponse(obj);
			//通讯不上,清空所有的待通讯任务
#ifdef _DEBUG
			//测试不清空
			//writeQueue_.clear();
#else
			writeQueue_.clear();
#endif

		}

#ifdef _DEBUG
		//下一次循环再通讯
		//return;
#else
		return;
#endif
	}

	if (!writeQueue_.empty()) {//普通通讯收发过程结束，且待发送队列非空
		//暂时断开信号槽，在函数中直接读取数据
		otherTimerFunc();
	}
	else {
		normalTimerFunc();
	}
}


void CommunicationThread::looseFireRealData(const QJsonObject& obj) {

	unsigned __int64 interval = 1000;
	if (lastSendRealData_ == 0) {
		lastSendRealData_ = GetTickCount64();
		return;
	}
	else {
		if (GetTickCount64() - lastSendRealData_ < interval) {
			return;
		}
		lastSendRealData_ = GetTickCount64();
	}

	emit wsResponse(obj);
}

unsigned __int64 lastSendRealData_;

void CommunicationThread::log(const QString& str) {
	if (m_socket->state() == QAbstractSocket::ConnectedState) {
		qDebug()
			<< "ip "
			<< m_socket->peerAddress()
			<< "port "
			<< m_socket->peerPort()
			<< "\t"
			<< str;
		return;
	}
	qDebug() << str;
}

void CommunicationThread::log(const QString& ip, const QString& str) {
	qDebug()
		<< "ip "
		<< ip
		<< "\t"
		<< str;
	return;
}
