#include "DataProcessor.h"
#include <Windows.h>
#include <cmath>

#include <qdebug.h>
#include <qthread.h>
#include <qsqlquery.h>
#include <qsqlerror.h>
#include <qsqlrecord.h>
#include <QJsonDocument>
#include <QRegularExpression.h>

#include "MethodHandler.h"
#include "ReportSettingHandler.h"
#include "SystemConfigHandler.h"
#include "TestingHandler.h"
#include "ReportHandler.h"

extern QString gSqlType;

DataProcessor::DataProcessor(QObject* parent, int deviceId)
	: QObject(parent)
{
	startTestingTime = std::chrono::high_resolution_clock::now();
	startTime_ = 0;

	checklastMoldStatus_ = MOLD_STATUS::INIT;
	checkcurMoldStatus_ = MOLD_STATUS::INIT;


	deviceId_= deviceId;
	usingQueue_ = false;
	externalQueueId_ = 0;
	externalTemp_ = 0;
	flag_ = false;

	curTesting_ = false;
	curDoorStatus_ = DOOR_STATUS::OPEN;
	curMoldStatus_ = MOLD_STATUS::UP;

	lastTesting_ = curTesting_;
	lastDoorStatus_ = curDoorStatus_;
	lastMoldStatus_ = curMoldStatus_;

	queueId_ = -1;

	//配置前端WS消息的通道处理器
	handlers_["config-method-message"] = std::shared_ptr<MethodHandler>(new MethodHandler(this));
	handlers_["config-report-message"] = std::shared_ptr<ReportSettingHandler>(new ReportSettingHandler(this));
	handlers_["config-system-message"] = std::shared_ptr<SystemConfigHandler>(new SystemConfigHandler(this));
	handlers_["report-message"] = std::shared_ptr<ReportHandler>(new ReportHandler(this));

	//如果需要先经过数据处理的消息，再转到设备线程，使用这个通道
	handlers_["data-testing-message"] = std::shared_ptr<TestingHandler>(new TestingHandler(this));
	
	//类内部使用
	handlers_["__"] = std::shared_ptr<MsgHandler>(new MsgHandler(this));

	for (auto& it : handlers_) {
		it->setDeviceId(deviceId_);
		connect(it.get(), &MsgHandler::fileSelect, this, &DataProcessor::fileSelect);
		connect(it.get(), &MsgHandler::useRealTime, this, &DataProcessor::useRealTime);
	}

	queueTimer_ = new QTimer();
    connect(queueTimer_, &QTimer::timeout, this, &DataProcessor::queueFunc);
	queueTimer_->setInterval(1000);
	queueTimer_->start();


}

DataProcessor::~DataProcessor()
{
	handlers_.clear();
	if (queueTimer_->isActive())
		queueTimer_->stop();
}


/**
 * 获取下一个测试队列.
 *
 * \param id
 * \return
 */
bool DataProcessor::getNextQueue(int& id) {
	return true;
}


bool DataProcessor::transferMethodFunc(const int nextId) {
	return true;
}

void DataProcessor::queueFunc() {
	//qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "queueFunc" << QThread::currentThreadId();
	//已经获取了测试队列
	if (externalQueueId_ != 0) {
		return;
	}

	//备注,定时器所在线程与handleWSMsg是同一个,因此是线程安全的
	QSqlDatabase configDb;
	if (!getConfigDb(configDb)) {
		return;
	}
	QString strSql = QString("select system_config.useQueue,method_config.id from system_config join method_config on method_config.name = system_config.current_method where system_config.id = %1").arg(deviceId_);
	QSqlQuery configQuery(configDb);
	if (!configQuery.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << configDb.lastError().text();
		return;
	}
	if (!configQuery.next()) {
		//qDebug() <<"deviceId :  "<<deviceId_ << "\t" << QStringLiteral("Error 找不到系统配置数据");
		return;
	}
	if (configQuery.value("id").isNull()) {
		//没有选择方法
		return;
	}
	usingQueue_ = configQuery.value("useQueue").toInt() == 1;

	if (!usingQueue_) {
		//没有使用队列
		return;
	}

	int nextId = 0;
	if(!getNextQueue(nextId)) {
		return;
	}

	if (!transferMethodFunc(nextId))
		return;

	externalQueueId_ = nextId;
}



void DataProcessor::handleWSMsg(const QJsonObject& recvObj) {
	QString channel = recvObj["__channel"].toString();
	auto it = handlers_.find(channel);
	if (it == handlers_.end())
		return;
	QString response;
	QJsonObject tmp = recvObj;
	if (!it.value()->handleWsMsg(tmp, response)) {
		return;
	}
	if (response.size())
		emit fireEvent(response);

	if (it.value()->isTransfer(recvObj)) {
		//需要转发给设备线程
		//例如，传送测试方法到设备
		emit requestMach(tmp);
	}
}


bool DataProcessor::getTestDataDB(QSqlDatabase& db) {
	auto handler = handlers_["__"];
	if (!handler->getTestDataDB(db)) {
		return false;
	}
	return true;
}


bool DataProcessor::getConfigDb(QSqlDatabase& db) {
	auto handler = handlers_["__"];
	if (!handler->getConfigDB(db)) {
		return false;
	}
	return true;
}




void DataProcessor::sumupQueue() {
	QSqlDatabase configDb, testDataDb;

	if (!getConfigDb(configDb))
		return;

	if (!getTestDataDB(testDataDb))
		return;


	//将当前测试记录设置为测试结束
	QString strSql = QString("update queue set status = 2 where id = %1").arg(queueId_);
	QSqlQuery testQuery(testDataDb);
	if (!testQuery.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Failed to fetch data:";
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << testDataDb.lastError().text();
		return;
	}
}


/**
 * 通知设备线程,输送下一个料.
 *
 */
void DataProcessor::sendNextItem() {

	qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "sendNextItem";

	QJsonObject obj;
	obj["__channel"] = "control-testing";
	obj["__type"] = "sendNextItem";
	emit requestMach(obj);
}


void DataProcessor::stopTesting() {
	QJsonObject obj;
	obj["__channel"] = "control-testing";
	obj["__type"] = "stop";
	emit requestMach(obj);
}

/**
 * 判断是否需要输送下一个料.
 *
 * @param info
 * @return
 */
bool DataProcessor::checkAutoFeeding(const U65RawData& info) {
	/**
	有送料機的狀況下 …
	測試步驟停在 7(=暫停) 時, 再PC 變更測試條件後
	恢復送料&測試
	 */
	if (info.U65Info.U65_MSG == 0x07) {
		//确认是有送料机的
		/*if ((info.U65Info.Sys_Flag1 >> 3) & 1) {
			return true;
		}*/
		return true;
	}
	return false;
}


void DataProcessor::endTestQueueFunc(const U65RawData& info) {

	qDebug() << "deviceId :  " << deviceId_ << "\t" << "endTestQueue";

	//是否有自动送料
	if (!checkAutoFeeding(info)) {

		qDebug() << "deviceId :  " << deviceId_ << "\t" << "checkAutoFeeding not ";
		qDebug() << "deviceId :  " << deviceId_ << "\t" << info.U65Info.U65_MSG;
		qDebug() << "deviceId :  " << deviceId_ << "\t" << info.U65Info.U65_MODE;
		qDebug() << "deviceId :  " << deviceId_ << "\t" << info.U65Info.Sys_Flag1;

		if (usingQueue_)
			externalQueueId_ = 0;

		return;
	}

	qDebug() << "deviceId :  " << deviceId_ << "\t" << "usingQueue_  " << usingQueue_;
	//不使用外部队列
	if (!usingQueue_) {
		sendNextItem();
		return;
	}

	int nextId = 0;
	if (!getNextQueue(nextId)) {
		externalQueueId_ = 0;
		stopTesting();//没有料了,结束测试

		QJsonObject obj;
		obj["__channel"] = "global-message";
		obj["__type"] = "message";
		obj["type"] = "error";
		obj["duration"] = 0;
		obj["message"] = "queue is empty";

		QJsonDocument jsonDoc(obj);
		QString response = jsonDoc.toJson();
		emit fireEvent(response);
		qDebug()<<response;

		return;
	}

	//自动送料，直接获取下一个队列
	{
		//下发温度、时间
		qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("结束测试重新获取队列");
		if (!transferMethodFunc(nextId))
			return;

		externalQueueId_ = nextId;

	}

	return sendNextItem();
}

void DataProcessor::endTestQueue(const U65RawData& info) {
	endTestQueueFunc(info);
	//结算
	sumupQueue();	
}

void DataProcessor::handleTesting(const U65RawData& info) {
	
	//记录原始数据
	recordDetail(info);

	if (accountTime_ > 5) {
		return;
	}

	unsigned __int64 interval = 10000;

	//结算时间间隙加大
	interval = 10000 + 20000* accountTime_;

	if (lastAccountTime_ == 0) {
		lastAccountTime_ = GetTickCount64();
		return;
	}
	else {
		//每10秒结算一次，看看现场情况，可以用线程优化下
		if (GetTickCount64() - lastAccountTime_ < interval) {
			return;
		}
		lastAccountTime_ = GetTickCount64();
	}
	accountTime_ = 1;
	QSqlDatabase configDb, testDataDb;

	if (!getConfigDb(configDb))
		return;

	if (!getTestDataDB(testDataDb))
		return;

	//TODO 准备结算
}


double CVF_GetMedia(double* Data, int mi_count)
{
	double md_temp, md_tempdata;
	for (int j = 0; j < mi_count - 1; j++)
		for (int mi_temp = j; mi_temp < mi_count - 1; mi_temp++)
		{
			if (Data[mi_temp] < Data[mi_temp + 1])
			{
				md_temp = Data[mi_temp];
				Data[mi_temp] = Data[mi_temp + 1];
				Data[mi_temp + 1] = Data[mi_temp];
			}
		}
	md_tempdata = Data[(mi_count + 1) / 2];

	//   double md_tempdata=md_Second;
	//   if(md_First>=md_tempdata)
	//   {  if(md_tempdata<md_Third)
	//	  {  md_tempdata=md_Third;
	//	  }
	//   }
	//   else
	//   {  md_tempdata=md_First;
	//	  if( md_Third>md_First)
	//	  { md_tempdata=md_Third;
	//	  }
	//   }
	return md_tempdata;
}

/**
 * 记录原始数据.
 *
 * @param info
 */
void DataProcessor::recordDetail(const U65RawData& info) {
	//当前时间减去当前胶料的测试开始时间
	float  time = info.U65Info.TEST_TIMER - startTime_;
	time = round(time * 10) / 10;//保留一位小数


	////只记录不一样的数据
	//if (lastTime_ == time && time != 0) {
	//	return;
	//}

	//if (time - lastTime_ > 5 && lastTime_ ==0) {
	//	//
	//	qDebug() << "time error";
	//	return;
	//}


	lastTime_ = time;
	//原来在测试,继续测试
	//写入一条常规数据到detail中即可.
	QSqlDatabase testDataDb;
	if (!getTestDataDB(testDataDb)) {
		qDebug()<<"getTestDataDB";
		return;
	}
	QSqlQuery query(testDataDb);
	QString strSql = QString("insert into detail(queue_id,angle,torque,YZ_mm) values(%1,%2,%3,%4)")
		.arg(queueId_)
		.arg(info.twistingData.angle)
		.arg(info.twistingData.torque)
		.arg(info.twistingData.axialDisplacement);

	if (!query.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << query.lastError().text();
		return;
	}
}

void  DataProcessor::handleRegularInfoFunc(const U65RawData& info) {

	

	if (lastTesting_) {
		//数据异常,不记录任何数据
		if (!flag_) 
			return;

		//结算当前测试记录
		if (!curTesting_) 			
			return endTestQueue(info);
		//测试中
		return handleTesting(info);
	}
	else if (curTesting_) {
		details_.clear();


		step1StartTime_ = 0;
		step2StartTime_ = 0;
		lastStep_ = 0;

		//门尼有时候不会检测到开模，导致开始时间不为0
		startTime_ = 0;

		return beginTestQueue(info);
	}

}

/**
	 * 开始下一条测试队列
	 *
	 * @param info
	 */
void DataProcessor::beginTestQueue(const U65RawData& info) {
	//第一步获取当前是否使用队列
	QSqlDatabase configDb;
	if (!getConfigDb(configDb))
		return;
	QString strSql = QString("select useQueue from system_config where id = %1").arg(deviceId_);
	QSqlQuery configQuery(configDb);
	if (!configQuery.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Failed to fetch data:";
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << configDb.lastError().text();
		return;
	}
	if (!configQuery.next()) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << QStringLiteral("Error 系统数据异常");
		return;
	}
	usingQueue_ = configQuery.value("useQueue").toInt() == 1;

	
	flag_ = beginInternalTest();
	if (!flag_) {
		qDebug() << "error";
	}
}


bool DataProcessor::beginInternalTest() {
	QSqlDatabase testDataDb,configDb;
	if (!getTestDataDB(testDataDb))
		return false;
	if (!getConfigDb(configDb))
		return false;

	QString strSql;

	//获取下一条测试记录,更新为测试中
	strSql = QString("select id from queue where current = 1 ORDER BY id limit 1");
	QSqlQuery testQuery(testDataDb);
	if (!testQuery.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Failed to fetch data:";
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << testDataDb.lastError().text();
		return false;
	}
	if (!testQuery.next()) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << QStringLiteral("Error 没有可以测试的队列");
		return false;
	}
	//更新队列ID
	queueId_ = testQuery.value("id").toInt();
	return true;
}

/**
* 从设备线程发送过来的数据，需要进行数据处理，随后保存到数据库.
*
* @param data
*/
void DataProcessor::handleRegularInfo(const QVariant& data) {
	//QThread::msleep(600);
	// 
	//转换数据类型
	U65RawData info = data.value<U65RawData>();

	do {
		long long timeOut = 2000;
		auto end = std::chrono::high_resolution_clock::now();

		checkcurMoldStatus_ = ((info.U65Info.IO1_IN >> 2) & 0x01) ? MOLD_STATUS::UP : MOLD_STATUS::DOWN;

		auto tmpLastStatus = checklastMoldStatus_;
		checklastMoldStatus_ = checkcurMoldStatus_;

		if (checkcurMoldStatus_ == MOLD_STATUS::DOWN) {
			//如果是合模状态，不需要过滤
			break;
		}

		if (tmpLastStatus == MOLD_STATUS::DOWN) {
			//当前开模，但是上次是合模
			startTestingTime = std::chrono::high_resolution_clock::now();
			return;
		}

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTestingTime);
		if (duration.count() < timeOut) {
			//开模时间未到
			qDebug() << "3sec recv error open status";
			return;
		}
	} while (0);



	//更新本次循环的状态
	curDoorStatus_ = ((info.U65Info.U65_MSG3 >> 9) & 0x01) ? DOOR_STATUS::CLOSE : DOOR_STATUS::OPEN;
	curMoldStatus_ = ((info.U65Info.IO1_IN >> 2) & 0x01) ? MOLD_STATUS::UP : MOLD_STATUS::DOWN;
	curTesting_ = info.isTesting();

	if (lastDoorStatus_ != curDoorStatus_ || 
		curMoldStatus_ != lastMoldStatus_ ||
		curTesting_ != lastTesting_
		) {

		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "\n\n******************************STATUS CHANGE***************************\n\n";
		if (curDoorStatus_ == DOOR_STATUS::CLOSE) {
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Door Close";
		}
		else
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Door Open";

		if (curMoldStatus_ == MOLD_STATUS::UP) {
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Mold Up";
		}
		else
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "Mold Down";
		if (curTesting_)
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "testing";
		else
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "nottesting ";
	}



	//recordDetail(info);


	//处理数据
	handleRegularInfoFunc(info);

	//记录上一次的状态
	lastDoorStatus_ = curDoorStatus_;
	lastMoldStatus_ = curMoldStatus_;
	lastTesting_ = curTesting_;
}

/**
 * 测试数据点写入测试数据库.
 *
 * @param testDb
 * @param items
 * @return
 */
bool DataProcessor::recordItems(QSqlDatabase& testDb, const QVector<QPair<QString, QString>>& items, const QMap<QString, QPair<double, double>>&limitDetail, bool& result) {
	QString strSql;
	QSqlQuery query(testDb);

	//先清空原有结果
	strSql = QString("delete from result where queue_id=%1").arg(queueId_);
	if (!query.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << testDb.lastError().text();
		return false;
	}
	result = true;
	bool hasAcceptItem = false;
	for (auto it = items.begin();it != items.end();it++) {
		QString strName = it->first;
		if (strName == "acceptance test") {
			hasAcceptItem = true;
			continue;
		}
		strSql = QString("insert into result(queue_id,name,data) values(%1,'%2','%3')")
			.arg(queueId_)
			.arg(strName.replace("'", "''"))
			.arg(it->second);
		if (!query.exec(strSql)) {
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << testDb.lastError().text();
			return false;
		}

		auto limitIt = limitDetail.find(strName);
		if (limitIt != limitDetail.end()) {
			auto iValue = it->second.toFloat();
			if (iValue < limitIt.value().first || iValue > limitIt.value().second)
				result = false;
		}
	}

	strSql = QString("insert into result(queue_id,name,data) values(%1,'%2','%3')")
		.arg(queueId_)
		.arg("result")
		.arg(result?"PASS":"NG");
	if (!query.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << testDb.lastError().text();
		return false;
	}

	if (hasAcceptItem) {

		strSql = QString("insert into result(queue_id,name,data) values(%1,'%2','%3')")
			.arg(queueId_)
			.arg("acceptance test")
			.arg(result ? "PASS" : "NG");
		if (!query.exec(strSql)) {
			qDebug() << "deviceId :  " << deviceId_ << "\t" << testDb.lastError().text();
			return false;
		}
	}

	//更新测试日期
	strSql = QString("update result set data = datetime('now','localtime') where queue_id = %1 and `name` ='test_date' ").arg(queueId_);
	if (!query.exec(strSql)) {
		qDebug() <<"deviceId :  "<<deviceId_ << "\t" << testDb.lastError().text();
		return false;
	}

	return true;
}

