#include "MultiMachDispatcher.h"

#include <qdebug.h>
#include <qsqlquery.h>
#include <qsqlerror.h>
#include <qsqlrecord.h>
#include <qfiledialog.h>
#include <qjsonarray.h>
#include <qjsondocument.h>

#include "DataProcessor.h"

MultiMachDispatcher::MultiMachDispatcher(QObject *parent)
	: MsgHandler(parent)
{
	//特殊设备ID，不存在，只是为了和设备的数据库连接实例名称区分
	setDeviceId(100);

	init();

	auto devs = devices();
	int count = -1;
	for (auto it : devs) {
		count++;
		qDebug() << "machThread" << deviceId_;
		machThread[count].setDeviceId(count+1);
		machThread[count].init(it.ip, 1500);
		connect(&machThread[count], &CommunicationThread::wsResponse, this, &MultiMachDispatcher::wsResponse);

		if (!it.enable)
			continue;

		machThread[count].start();
	}

	connect(this, &MultiMachDispatcher::forwardToMach1, &machThread[0], &CommunicationThread::handleWsRequest);
	connect(this, &MultiMachDispatcher::forwardToMach2, &machThread[1], &CommunicationThread::handleWsRequest);
	connect(this, &MultiMachDispatcher::forwardToMach3, &machThread[2], &CommunicationThread::handleWsRequest);
	connect(this, &MultiMachDispatcher::forwardToMach4, &machThread[3], &CommunicationThread::handleWsRequest);


	for (int i = 0; i < 4; i++) {
		//启动数据处理线程
		//qDebug() << "DataProcessor" << i;
		dataProcessor_[i] = new DataProcessor(nullptr, i+1);
		processThread_[i] = new QThread(this);
		dataProcessor_[i]->moveToThread(processThread_[i]);
		processThread_[i]->start();
	}

	//转发前端消息到数据处理对象
	connect(this, &MultiMachDispatcher::forwardToDataProcessor1, dataProcessor_[0], &DataProcessor::handleWSMsg);
	connect(this, &MultiMachDispatcher::forwardToDataProcessor2, dataProcessor_[1], &DataProcessor::handleWSMsg);
	connect(this, &MultiMachDispatcher::forwardToDataProcessor3, dataProcessor_[2], &DataProcessor::handleWSMsg);
	connect(this, &MultiMachDispatcher::forwardToDataProcessor4, dataProcessor_[3], &DataProcessor::handleWSMsg);

	

	for (size_t i = 0; i < 4; i++) {

		connect(dataProcessor_[i], &DataProcessor::fireEvent,this, [=](const QString& message, const QString& ip) {
			

			QByteArray byteArray = message.toUtf8();
			QJsonParseError jsonError;
			QJsonDocument jsonDoc = QJsonDocument::fromJson(byteArray, &jsonError);
			if (jsonError.error != QJsonParseError::NoError) {
				return;
			}
			QJsonObject recvObj = jsonDoc.object();
			QString type = recvObj.value("__channel").toString();

			QSet<QString> filterType;//不需要过滤的消息
			filterType.insert("testing-message-queryTestCurve");
			filterType.insert("testing-message-queryTempCurve");
			if (recvObj.value("__nofilter").isNull()) {
				if (filterType.find(type) == filterType.end()) {
					int deviceId = getDeviceId();
					if (deviceId != i + 1)
						return;
				}
			}
			
			
			
			emit fireEvent(message, ip);
		});

		connect(dataProcessor_[i], &DataProcessor::fileSelect, this, &MultiMachDispatcher::fileSelect,  Qt::BlockingQueuedConnection);


		//设备通讯 & 数据处理  之间的信号槽，此处稍微有点耦合
		{
			//有些WS请求跟设备通讯，通讯前，需要先获取数据库信息，再转发给通讯线程。
			connect(dataProcessor_[i], &DataProcessor::requestMach, &machThread[i], &CommunicationThread::handleWsRequest);
			connect(dataProcessor_[i], &DataProcessor::useRealTime, &machThread[i], &CommunicationThread::useRealTime);
			//测试数据发送至数据处理线程，处理后保存于数据库
			connect(&machThread[i], &CommunicationThread::fireRegularInfo, dataProcessor_[i], &DataProcessor::handleRegularInfo);
		}

	}
	
}

MultiMachDispatcher::~MultiMachDispatcher()
{
	for (size_t i = 0; i < 4; i++) {
		dataProcessor_[i]->deleteLater();
		if (processThread_[i]) {
			processThread_[i]->quit();
			processThread_[i]->deleteLater();
		}
	}
	
}


void MultiMachDispatcher::init() {
	QSqlDatabase configDb,testDb;
	if (!getConfigDB(configDb)) {
		qDebug() << configDb.lastError().text();
		return;
	}
	if (!getTestDataDB(testDb,1)) {
		qDebug() << testDb.lastError().text();
		return;
	}

	//启动时，将所有机台的当前测试记录设置为0
	QString strSql = QString("update queue set current = 0,show=0");
	QSqlQuery query(testDb);
	if (!query.exec(strSql)) {
		qDebug() << "deviceId :  " << deviceId_ << "\t" << testDb.lastError().text();
		return;
	}

}

void MultiMachDispatcher::fileSelect(QString& fileName) {
	QString filter = "CSV files (*.csv)";
	fileName = QFileDialog::getSaveFileName(nullptr, "Save File", QDir::homePath(), filter);

	if (!fileName.isEmpty()) {
		// Ensure the file has a .csv extension
		if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
			fileName += ".csv";
		}
	}
}


QVector<MultiMachDispatcher::DEVICE_INFO> MultiMachDispatcher::devices() {
	QVector<MultiMachDispatcher::DEVICE_INFO> res;
	QSqlDatabase db;
	if (!getConfigDB(db)) {
		qDebug() << db.lastError().text();
		return res;
	}
	QString strSql = QString("select * from ip_config ");
	QSqlQuery query(db);
	if (!query.exec(strSql)) {
		qDebug() << query.lastError().text();
		return res;
	}
	if (query.record().count() != 4) {
		qDebug() << "device data error";
		return res;
	}
	while (query.next()) {
		DEVICE_INFO device;
		device.ip = query.value("ip").toString();
		device.enable = query.value("enabled").toInt()==1;
		res.push_back(device);
	}
	return res;
}

int MultiMachDispatcher::getDeviceId() {
	QSqlDatabase db;
	if (!getConfigDB(db)) {
		qDebug() << db.lastError().text();
		return -1;
	}

	QString strSql = QString("select * from other_config where item ='deviceId'");
	QSqlQuery query(db);
	if (!query.exec(strSql)) {
		qDebug() << query.lastError().text();
		return -1;
	}
	if (!query.next()) {
		qDebug() << "no data for deviceId";
		return -1;
	}
	int deviceId = query.value("data").toInt();
	return deviceId;
}


void MultiMachDispatcher::handleRestFulMsg(const QJsonObject& recvObj, QJsonObject& obj){
	int deviceId = 0;
	if (recvObj["deviceId"].isNull()) {
		//没有传入deviceId，默认选择当前机台
		deviceId = getDeviceId();
	}
	else {
		deviceId = recvObj["deviceId"].toInt();
		if (deviceId == 0) {
			deviceId = getDeviceId();
		}
	}

	switch (deviceId)
	{
	case 1:
	{
		emit forwardRestFulMsg1(recvObj, obj);
		break;
	}
	case 2:
	{
		emit forwardRestFulMsg2(recvObj, obj);
		break;
	}
	case 3:
	{
		emit forwardRestFulMsg3(recvObj, obj);
		break;
	}
	case 4:
	{
		emit forwardRestFulMsg4(recvObj, obj);
		break;
	}
	default:
		break;
	}
}


void MultiMachDispatcher::handleWSMachReq(const QJsonObject& obj) {
	int deviceId = getDeviceId();
	switch (deviceId)
	{
	case 1:
	{
		emit forwardToMach1(obj);
		break;
	}
	case 2:
	{
		emit forwardToMach2(obj);
		break;
	}
	case 3:
	{
		emit forwardToMach3(obj);
		break;
	}
	case 4:
	{
		emit forwardToMach4(obj);
		break;
	}
	default:
		break;
	}
}

void MultiMachDispatcher::handleWSMsg(const QJsonObject& obj){
	int deviceId = getDeviceId();

	QString channel = obj["__channel"].toString();
	QString type = obj["__type"].toString();

	switch (deviceId)
	{
	case 1:
	{
		emit forwardToDataProcessor1(obj);
		break;
	}
	case 2:
	{
		emit forwardToDataProcessor2(obj);
		break;
	}
	case 3:
	{
		emit forwardToDataProcessor3(obj);
		break;
	}
	case 4:
	{
		emit forwardToDataProcessor4(obj);
		break;
	}
	default:
		break;
	}
}
