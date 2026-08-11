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
	running_ = false;
	commHandlers_["normal-message"] = new NormalCommHandler(u65Info, this);//常规通讯
	commHandlers_["control-message"] = new ControlTestCommHandler(u65Info, this);//常规通讯
	commHandlers_["pid-message"] = new PIDCommHandler(u65Info, this);//常规通讯

	useRealTime_ = false;

	lastSendRealData_ = 0;
}

CommunicationThread::~CommunicationThread()
{
	running_ = false;

	wait();

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
		m_socket->connectToHost(m_deviceAddress, m_port);
		if (!m_socket->waitForConnected()) {
			qDebug() << m_socket->errorString();
		}
	}

	running_ = true;
	while (running_) {
		this->timerFunc();

		// 关键：这里必须有短暂让出，否则会跑满一个CPU核心，
		// 且完全无法响应停止请求（如果timerFunc内部阻塞的话）
		QThread::msleep(1); // 或者更小，看你实时性要求，1~5ms通常够用
	}

	// 退出循环，做清理
	if (m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->disconnectFromHost();
	}


	return;
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
	obj["connectErr"] = false;
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

	if (false) {
		//通过读取资料区的方式,模拟采集
		return fakeData(info);
	}
	else {
		//qDebug() << "fire " << info.U65Info.REAL_MSG_CT;
		//读取缓冲区
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
#endif
	auto tid = QThread::currentThread()->currentThreadId();
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
		obj["connectErr"] = true;
		looseFireRealData(obj);

		m_socket->connectToHost(m_deviceAddress, m_port);
		if (!m_socket->waitForConnected()) {
			//log(m_deviceAddress, m_socket->errorString());
		}

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
	return;//写日志太频繁了
	qDebug()
		<< "ip "
		<< ip
		<< "\t"
		<< str;
	return;
}
