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
#include <math.h>
#include <qdatastream.h>
#include <QElapsedTimer.h>

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

	last_REAL_MSG_CT_ = cur_REAL_MSG_CT_ = 0;	


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


bool DataProcessor::account(QSqlDatabase& configDb, QSqlDatabase& testDb, bool final) {
	return true;
}
double findClosestTorque(
	const std::vector<TwistingData>& data,
	double targetAngle,
	const char* phase = "all" // "rise", "fall", "rise2", "all"
) {
	if (data.empty()) return 0.0;

	// 1. 自动检测关键点（角度最大值和最小值）
	auto max_angle_it = std::max_element(data.begin(), data.end(),
		[](const TwistingData& a, const TwistingData& b) {
			return a.angle < b.angle;
		});
	auto min_angle_it = std::min_element(data.begin(), data.end(),
		[](const TwistingData& a, const TwistingData& b) {
			return a.angle < b.angle;
		});

	size_t peak_angle_idx = std::distance(data.begin(), max_angle_it);
	size_t valley_angle_idx = std::distance(data.begin(), min_angle_it);

	// 2. 定义搜索范围（基于角度变化趋势）
	std::pair<size_t, size_t> search_range;
	if (strcmp(phase, "rise") == 0) {
		// 第一次上升段：从开始到角度峰值
		search_range = { 0, peak_angle_idx };
	}
	else if (strcmp(phase, "fall") == 0) {
		// 下降段：从角度峰值到角度波谷
		search_range = { peak_angle_idx, valley_angle_idx };
	}
	else if (strcmp(phase, "rise2") == 0) {
		// 第二次上升段：从角度波谷到最后
		search_range = { valley_angle_idx, data.size() - 1 };
	}
	else {
		// 全部数据
		search_range = { 0, data.size() - 1 };
	}

	// 3. 在指定范围内线性搜索最接近的角度值
	double min_diff = std::abs(data[search_range.first].angle - targetAngle);
	double closest_torque = data[search_range.first].torque;

	for (size_t i = search_range.first; i <= search_range.second; ++i) {
		double current_diff = std::abs(data[i].angle - targetAngle);
		if (current_diff < min_diff) {
			min_diff = current_diff;
			closest_torque = data[i].torque;
		}
	}

	return closest_torque;
}

// 查找最接近目标扭矩的角度（支持分段搜索）
double findClosestAngle(
	const std::vector<TwistingData>& data,
	double targetTorque,
	const char* phase = "all" // "rise", "fall", "rise2", "all"
) {
	if (data.empty()) return 0.0;

	// 1. 自动检测关键点（峰值和波谷）
	auto max_it = std::max_element(data.begin(), data.end(),
		[](const TwistingData& a, const TwistingData& b) {
			return a.torque < b.torque;
		});
	auto min_it = std::min_element(data.begin(), data.end(),
		[](const TwistingData& a, const TwistingData& b) {
			return a.torque < b.torque;
		});

	size_t peak_idx = std::distance(data.begin(), max_it);
	size_t valley_idx = std::distance(data.begin(), min_it);

	// 2. 定义搜索范围
	std::pair<size_t, size_t> search_range;
	if (strcmp(phase, "rise") == 0) {
		search_range = { 0, peak_idx }; // 上升段
	}
	else if (strcmp(phase, "fall") == 0) {
		search_range = { peak_idx, valley_idx }; // 下降段
	}
	else if (strcmp(phase, "rise2") == 0) {
		search_range = { valley_idx, data.size() - 1 }; // 二次上升段
	}
	else {
		search_range = { 0, data.size() - 1 }; // 全部数据
	}

	// 3. 在指定范围内线性搜索最接近值
	double min_diff = std::abs(data[search_range.first].torque - targetTorque);
	double closest_angle = data[search_range.first].angle;

	for (size_t i = search_range.first; i <= search_range.second; ++i) {
		double current_diff = std::abs(data[i].torque - targetTorque);
		if (current_diff < min_diff) {
			min_diff = current_diff;
			closest_angle = data[i].angle;
		}
	}

	return closest_angle;
}


void sumupQueueFunc(
	const QSqlDatabase &configDb, 
	const QSqlDatabase& testDataDb,
	const int queueId_
) {
	
	//读取report_setting
	QString strSql = QString("select * from report_setting ");
	QSqlQuery configQuery(configDb);
	if (!configQuery.exec(strSql)) {
		qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " <<  "\t" << configDb.lastError().text();
		return;
	}
	QVector<QString> reportSetting;

	struct _TEST_DATA {
		double torque;
		double angle;
		double torque2;
		double angle2;


		_TEST_DATA() {

			torque = 0;
			angle = 0;;
			torque2 = 0;;
			angle2 = 0;;
		}
	};


	std::pair<double, double> torqueToAnglePair;//最大扭矩对应的角度
	std::pair<double, double> angleToTorquePair;//最大角度对应的扭矩
	torqueToAnglePair.first = 0;
	torqueToAnglePair.second = 0;
	angleToTorquePair.first = 0;
	angleToTorquePair.second = 0;

	typedef std::vector< _TEST_DATA> TEST_DATA_VEC;
	QMap<int, TEST_DATA_VEC> ReportTorqueToAngle;//int是循环次数  第二个元素是记录所有需要找的扭矩对应的角度
	QMap<int, TEST_DATA_VEC> ReportAngleToTorque;//int是循环次数  第二个元素是记录所有需要找的角度对应的扭矩
	QMap<int, TEST_DATA_VEC> ReportStiffnessAngle;//int是循环次数  第二个元素是记录所有需要找的扭转刚度
	while (configQuery.next())
	{
		QString name = configQuery.value("name").toString();
		//如果前面字符串是"torque"
		if (name.startsWith("torque-")) {
			//去掉前面字符串
			name = name.remove(0, 7);

			//剩下的格式是 1-2  提取这两个数字，可能是小数
			QStringList list = name.split("-");
			if (list.size() != 2) {
				qDebug() << "deviceId :  " <<  "\t" << QStringLiteral("stiffness error");
				continue;
			}

			bool ok1, ok2;
			double torque = list[0].toDouble(&ok1);
			int twistCount = list[1].toInt(&ok2);
			if (!ok1 || !ok2) {
				qDebug() << "deviceId :  " <<  "\t" << QStringLiteral("stiffness error");
				continue;
			}
			auto it = ReportTorqueToAngle.find(twistCount);

			if (it == ReportTorqueToAngle.end()) {
				ReportTorqueToAngle[twistCount] = std::vector< _TEST_DATA>();
			}
			_TEST_DATA testData;
			testData.torque = torque;
			ReportTorqueToAngle[twistCount].push_back(testData);
		}
		//如果前面字符串是"angle"
		else if (name.startsWith("angle-")) {
			//去掉前面字符串
			name = name.remove(0, 6);
			//剩下的格式是 1-2  提取这两个数字，可能是小数
			QStringList list = name.split("-");
			if (list.size() != 2) {
				qDebug() << "deviceId :  " <<  "\t" << QStringLiteral("stiffness error");
				continue;
			}

			bool ok1, ok2;
			double angle = list[0].toDouble(&ok1);
			int twistCount = list[1].toInt(&ok2);
			if (!ok1 || !ok2) {
				qDebug() << "deviceId :  " <<  "\t" << QStringLiteral("stiffness error");
				continue;
			}
			auto it = ReportAngleToTorque.find(twistCount);

			if (it == ReportAngleToTorque.end()) {
				ReportAngleToTorque[twistCount] = std::vector< _TEST_DATA>();
			}
			_TEST_DATA testData;
			testData.angle = angle;
			ReportAngleToTorque[twistCount].push_back(testData);
		}
		//如果前面字符串是"stiffness"
		else if (name.startsWith("stiffness-")) {
			//去掉前面字符串
			name = name.remove(0, 10);
			//剩下的格式是 1-2  提取这两个数字，可能是小数
			QStringList list = name.split("-");
			if (list.size() != 3) {
				qDebug() << "deviceId :  " <<  "\t" << QStringLiteral("stiffness error");
				continue;
			}
			bool ok1, ok2, ok3;
			double torque = list[0].toDouble(&ok1);
			double torque2 = list[1].toDouble(&ok2);
			int twistCount = list[2].toInt(&ok3);
			if (!ok1 || !ok2 || !ok3) {
				qDebug() << "deviceId :  " <<  "\t" << QStringLiteral("stiffness error");
				continue;
			}

			auto it = ReportStiffnessAngle.find(twistCount);

			if (it == ReportStiffnessAngle.end()) {
				ReportStiffnessAngle[twistCount] = std::vector< _TEST_DATA>();
			}
			_TEST_DATA testData;
			testData.torque = torque;
			testData.torque2 = torque2;
			ReportStiffnessAngle[twistCount].push_back(testData);
		}

		reportSetting.push_back(configQuery.value("name").toString());
	}

	QString testMode = "";
	strSql = QString("select method_config.* from method_config join system_config on system_config.current_method = method_config.name");
	if (!configQuery.exec(strSql)) {
		qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " << "\t" << configDb.lastError().text();
		return;
	}
	if (configQuery.next()) {
		testMode = configQuery.value("mode").toString();
	}

	QVector< TwistingData> vecTwistingData;

	QMap<int, std::vector<TwistingData>> perTwistingData;
	{
#ifdef _DEBUG
		//queueId_ = 67;
#endif
		strSql = QString("select * from detail where queue_id=%1").arg(queueId_);
		QSqlQuery testQuery(testDataDb);
		if (!testQuery.exec(strSql)) {
			qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
			qDebug() << "deviceId :  " <<  "\t" << testDataDb.lastError().text();
			return;
		}

		if (testQuery.next()) {
			QByteArray data = testQuery.value("data").toByteArray();
			QDataStream stream(&data, QIODevice::ReadWrite);
			int total = testQuery.value("totalNumber").toInt();
			//int queueId = testQuery.value("id").toInt();
			//qDebug() << "deviceId :  " << deviceId_ << "\t" << "totalNumber:" << total;
			//qDebug() << "deviceId :  " << deviceId_ << "\t" << "queueId:" << queueId;
			//qDebug() << "deviceId :  " << deviceId_ << "\t" << "data:" << data.size();
			//qDebug() << "deviceId :  " << deviceId_ << "\t" << "data:" << data.toHex();

			for (size_t i = 0; i < total; i++) {
				ExamplePoint point;
				float AD1, AD2, YZ_MM, time;
				int c1, c2;
				stream >> AD1 >> AD2 >> YZ_MM >> time >> c1 >> c2;
				point.AD1 = AD1;
				point.AD2 = AD2;
				point.YZ_mm = YZ_MM;
				point.time = time;

				TwistingData twistingData;
				twistingData.torque = AD2;
				twistingData.angle = YZ_MM;
				twistingData.testTimer = time;

				if (testMode == "dynamic") {
					twistingData.twistCount = c2;
				}
				else {
					twistingData.twistCount = c1;
				}

				vecTwistingData.push_back(twistingData);
				//将不同的twistCount，存入perTwistingData
				auto it = perTwistingData.find(twistingData.twistCount);
				if (it == perTwistingData.end()) {
					perTwistingData[twistingData.twistCount] = std::vector<TwistingData>();
				}
				perTwistingData[twistingData.twistCount].push_back(twistingData);
			}



		}
	}

	double lastAngle = 0;
	double lastTorque = 0;
	if (vecTwistingData.size()) {
		lastAngle = vecTwistingData.at(0).angle;
		lastTorque = vecTwistingData.at(0).torque;
	}

	//数据是类似一个sin波形，根据此属性，找到对应的报告数据
	//一开始的扭矩和角度都是0
	for (auto& it : vecTwistingData) {

		int twistCount = it.twistCount;

		if (it.torque > torqueToAnglePair.first) {
			torqueToAnglePair.first = it.torque;
			torqueToAnglePair.second = it.angle;
		}
		if (it.angle > angleToTorquePair.first) {
			angleToTorquePair.first = it.angle;
			angleToTorquePair.second = it.torque;
		}

		/*

	QMap<int, TEST_DATA_VEC> ReportTorqueToAngle;
	QMap<int, TEST_DATA_VEC> ReportAngleToTorque;
	QMap<int, TEST_DATA_VEC> ReportStiffnessAngle;
		*/

	}

	QSqlQuery testQuery(testDataDb);
	strSql = QString("delete from result where queue_id = %1").arg(queueId_);

	if (!testQuery.exec(strSql)) {
		qDebug() << "deviceId :  " << "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " << "\t" << testDataDb.lastError().text();
		return;
	}

	for (auto it = ReportTorqueToAngle.begin(); it != ReportTorqueToAngle.end(); it++) {
		auto it2 = perTwistingData.find(it.key());
		if (it2 == perTwistingData.end()) {
			continue;
		}

		for (auto& testData : it.value()) {
			// 查找最接近的扭矩对应的角度
			testData.angle = findClosestAngle(*it2, testData.torque, "rise");


			QString strTmp = QString("torque-%1-%2").arg(testData.torque).arg(it.key());
			strSql = QString("insert into result(queue_id,name,data) values(%1,'%3',%2)").arg(queueId_).arg(testData.angle).arg(strTmp);

			if (!testQuery.exec(strSql)) {
				qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
				qDebug() << "deviceId :  " << "\t" << testDataDb.lastError().text();
				return;
			}
		}
	}
	for (auto it = ReportAngleToTorque.begin(); it != ReportAngleToTorque.end(); it++) {
		auto it2 = perTwistingData.find(it.key());
		if (it2 == perTwistingData.end()) {
			continue;
		}
		for (auto& testData : it.value()) {
			// 查找最接近的角度对应的扭矩
			testData.torque = findClosestTorque(*it2, testData.angle, "rise");
			QString strTmp = QString("angle-%1-%2").arg(testData.angle).arg(it.key());
			strSql = QString("insert into result(queue_id,name,data) values(%1,'%3',%2)").arg(queueId_).arg(testData.torque).arg(strTmp);
			if (!testQuery.exec(strSql)) {
				qDebug() << "deviceId :  " << "\t" << "Failed to fetch data:";
				qDebug() << "deviceId :  " << "\t" << testDataDb.lastError().text();
				return;
			}
		}
	}

	for (auto it = ReportStiffnessAngle.begin(); it != ReportStiffnessAngle.end(); it++) {
		auto it2 = perTwistingData.find(it.key());
		if (it2 == perTwistingData.end()) {
			continue;
		}
		for (auto& testData : it.value()) {
			std::string phase = "rise";
			if (testData.torque > testData.torque2) {
				phase = "fall";
			}

			// 查找最接近的扭矩对应的角度
			testData.angle = findClosestAngle(*it2, testData.torque, phase.c_str());
			testData.angle2 = findClosestAngle(*it2, testData.torque2, phase.c_str());
			QString strTmp = QString("stiffness-%1-%2-%3").arg(testData.torque).arg(testData.torque2).arg(it.key());
			if (testData.angle - testData.angle2 == 0) {
				//除0异常
				strSql = QString("insert into result(queue_id,name,data) values(%1,'%3','%2')").arg(queueId_).arg("not found").arg(strTmp);
			}
			else {
				double res = (testData.torque - testData.torque2) / (testData.angle - testData.angle2);
				strSql = QString("insert into result(queue_id,name,data) values(%1,'%3',%2)").arg(queueId_).arg(res).arg(strTmp);
			}

			if (!testQuery.exec(strSql)) {
				qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
				qDebug() << "deviceId :  " <<  "\t" << testDataDb.lastError().text();
				return;
			}
		}
	}

	for(auto &it: reportSetting)
	{

		//插入最大扭力
		strSql = QString("insert into result(queue_id,name,data) values(%1,'maxTorque',%2)").arg(queueId_).arg(torqueToAnglePair.first);
		if (!testQuery.exec(strSql)) {
			qDebug() << "deviceId :  " << "\t" << "Failed to fetch data:";
			qDebug() << "deviceId :  " << "\t" << testDataDb.lastError().text();
			return;
		}

		//插入最大角度
		strSql = QString("insert into result(queue_id,name,data) values(%1,'maxAngle',%2)").arg(queueId_).arg(angleToTorquePair.first);
		if (!testQuery.exec(strSql)) {
			qDebug() << "deviceId :  " << "\t" << "Failed to fetch data:";
			qDebug() << "deviceId :  " << "\t" << testDataDb.lastError().text();
			return;
		}

		//插入最大角度对应扭力
		strSql = QString("insert into result(queue_id,name,data) values(%1,'maxAngleToTorque',%2)").arg(queueId_).arg(angleToTorquePair.second);

		if (!testQuery.exec(strSql)) {
			qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
			qDebug() << "deviceId :  " <<  "\t" << testDataDb.lastError().text();
			return;
		}

		//插入最大扭力对应角度
		strSql = QString("insert into result(queue_id,name,data) values(%1,'maxTorqueToAngle',%2)").arg(queueId_).arg(torqueToAnglePair.second);
		if (!testQuery.exec(strSql)) {
			qDebug() << "deviceId :  " <<  "\t" << "Failed to fetch data:";
			qDebug() << "deviceId :  " <<  "\t" << testDataDb.lastError().text();
			return;
		}

		//TODO  后面需要过滤再加上
		break;
	}
}

void DataProcessor::sumupQueue() {
	QSqlDatabase configDb, testDataDb;

	if (!getConfigDb(configDb))
		return;


	if (!getTestDataDB(testDataDb))
		return;

	sumupQueueFunc(configDb, testDataDb, queueId_);

	//将当前测试记录设置为测试结束
	QString strSql = QString("update queue set status = 2 where id = %1").arg(queueId_);
	QSqlQuery testQuery(testDataDb);
	if (!testQuery.exec(strSql)) {
		qDebug() << "deviceId :  " << deviceId_ << "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " << deviceId_ << "\t" << testDataDb.lastError().text();
		return;
	}

	return;

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


extern int SAMPLE_RATE;
void DataProcessor::record(const U65RawData& info, bool forceInput) {

	// 采样率设置
	float sample_freq = 1.0f;
	switch (SAMPLE_RATE) {
	case 0: sample_freq = 10000.0f; break;
	case 1: sample_freq = 5000.0f; break;
	case 3: sample_freq = 2500.0f; break;
	case 4: sample_freq = 2000.0f; break;
	case 9: sample_freq = 1000.0f; break;
	case 19: sample_freq = 500.0f; break;
	default: sample_freq = 1.0f; break;
	}

	int availableDataCount = cur_REAL_MSG_CT_ - last_REAL_MSG_CT_;
	if (availableDataCount > 0) {
		int maxValidData = 12;

		// 如果新增的数据条数大于缓存上限，跳过过期的数据，只保留最后12条
		int startIdx = (availableDataCount > maxValidData)
			? cur_REAL_MSG_CT_ - maxValidData
			: last_REAL_MSG_CT_;

		

		for (int i = 0; i < (availableDataCount > maxValidData ? maxValidData : availableDataCount); i++) {
			int realIndex = (startIdx + i) % maxValidData; // 缓存是12大小

			float YZ_MM = info.U65Info.YZ_MM[realIndex];
			float AD1 = info.U65Info.AD1[realIndex];
			float AD2 = info.U65Info.AD2[realIndex];
			//AD2 /= 1000;

			// 生成结构体
			TwistingData twistingData;
			twistingData.angle = YZ_MM;
			twistingData.torque = AD2;
			twistingData.axialDisplacement = AD1;
			twistingData.realTime = (cur_REAL_MSG_CT_ + i) / sample_freq;
			twistingData.twistCount = info.U65Info.twistingCount;
			twistingData.twistCountSin = info.U65Info.twistingCountSin;
			vecTwistingData_.push_back(twistingData);


			//				stream << AD1 << AD2 << YZ_MM << time << info.U65Info.twistingCount << info.U65Info.twistingCountSin;
		}
	}

	if (vecTwistingData_.size() == recordCount_)
		return;
	

	qDebug() << "vecTwistingData_" << vecTwistingData_.size() << "forceInput"<< forceInput << "   recordCount_" << recordCount_ << " sample_freq "<< sample_freq;
	if (!forceInput) {
		if (vecTwistingData_.size() - recordCount_ < sample_freq) {
			return;
		}
	}
	qDebug() << "recordCount_" << recordCount_;

	//寫入數據庫
	QSqlDatabase testDataDb;
	if (!getTestDataDB(testDataDb)) {
		qDebug() << "getTestDataDB";
		return;
	}
	QSqlQuery query(testDataDb);

	QString strSql = QString("select * from detail where queue_id = %1").arg(queueId_);
	if (!query.exec(strSql)) {
		qDebug() << "deviceId :  " << deviceId_ << "\t" << query.lastError().text();
		return;
	}

	int totalNumber = 0;
	QByteArray data = QByteArray();
	if (!query.next()) {
		strSql = QString("insert into detail(queue_id) values(%1)")
			.arg(queueId_);

		if (!query.exec(strSql)) {
			qDebug() << "deviceId :  " << deviceId_ << "\t" << query.lastError().text();

			if (!query.exec("rollback")) {
				qDebug() << "deviceId :  " << deviceId_ << "\t" << query.lastError().text();
				return;
			}
			return;
		}

	}
	else {
		totalNumber = query.value("totalNumber").toInt();
		data = query.value("data").toByteArray();
	}

	// 3. 序列化新数据
	QByteArray newBlobData;
	QDataStream stream(&newBlobData, QIODevice::WriteOnly);

	for (size_t i = recordCount_;i < vecTwistingData_.size();i++) {

		float YZ_MM = vecTwistingData_.at(i).angle;
		float AD1 = vecTwistingData_.at(i).axialDisplacement;
		float AD2 = vecTwistingData_.at(i).torque;
		float time = vecTwistingData_.at(i).realTime;
		//AD2 /= 1000;
		int twistingCount = vecTwistingData_.at(i).twistCount;;
		int twistingCountSin = vecTwistingData_.at(i).twistCountSin;;


		stream << AD1 << AD2 << YZ_MM << time << twistingCount << twistingCountSin;
		totalNumber++;
	}

	recordCount_ = vecTwistingData_.size();


	data.append(newBlobData);

	query.prepare("update detail set data = ?,totalNumber =? where queue_id = ?");
	query.addBindValue(data);
	query.addBindValue(totalNumber);
	query.addBindValue(queueId_);
	if (!query.exec()) {
		qDebug() << "deviceId :  " << deviceId_ << "\t" << query.lastError().text();
		return;
	}


}


void  DataProcessor::handleRegularInfoFunc(const U65RawData& info) {


	if (lastTesting_) {
		//数据异常,不记录任何数据
		if (!flag_)
			return;
		
		if (cur_REAL_MSG_CT_ != 0) {
			bool forceInput = false;
			if (!curTesting_)
				forceInput = true;
			record(info, forceInput);
		}

		if (last_REAL_MSG_CT_ == cur_REAL_MSG_CT_) {
			if (!curTesting_) {
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("结束测试重新获取队列");
				return endTestQueue(info);
			}
		}

		
			
	}
	else if (curTesting_) {

		startTime_ = 0;
		last_REAL_MSG_CT_ = 0;

		flag_ = true;
		return beginTestQueue(info);
	}

	return;



	//if (lastTesting_) {
	//	//数据异常,不记录任何数据
	//	if (!flag_) 
	//		return;




	//	return;

	//	//结算当前测试记录
	//	if (!curTesting_) 			
	//		return endTestQueue(info);
	//	//测试中
	//	return handleTesting(info);
	//}
	//else if (curTesting_) {
	//	details_.clear();


	//	step1StartTime_ = 0;
	//	step2StartTime_ = 0;
	//	lastStep_ = 0;

	//	//门尼有时候不会检测到开模，导致开始时间不为0
	//	startTime_ = 0;

	//	return beginTestQueue(info);
	//}

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

	vecTwistingData_.clear();
	recordCount_ = 0;
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

	//转换数据类型
	U65RawData info = data.value<U65RawData>();
	if (!info.writeData) {
		return;
	}

	if (!beginInternalTest()) {
		return;
	}


	// 3. 序列化新数据
	QByteArray newBlobData;
	QDataStream stream(&newBlobData, QIODevice::WriteOnly);
	for (size_t i = 0; i < info.datas.size(); i++) {
		qint64 sampleTimeUs = info.datas.at(i).sampleTimeUs;
		double torque = info.datas.at(i).torque;
		stream << sampleTimeUs << torque;
	}


	QSqlDatabase testDataDb;
	if (!getTestDataDB(testDataDb)) {
		qDebug() << "getTestDataDB";
		return;
	}
	QSqlQuery query(testDataDb);
	//query.prepare("update queue set raw_data = ?,totalNumber =? where queue_id = ?");
	//query.addBindValue(newBlobData);
	//query.addBindValue(static_cast<int>(info.datas.size()));     // 如果数据量不超过 21亿
	//query.addBindValue(queueId_);
	


	// ✅ 修复1：用命名占位符，避免位置混淆
	query.prepare("UPDATE queue SET raw_data = :raw, totalNumber = :total,speed= :speed ,status = 2 WHERE id = :id");

	// ✅ 修复2：用 bindValue 明确类型
	query.bindValue(":raw", QVariant(newBlobData));  // 明确是 BLOB
	query.bindValue(":total", static_cast<int>(info.datas.size()));
	query.bindValue(":speed", info.U65Info.speed);
	query.bindValue(":id", queueId_);   // 明确转为数字类型
	if (!query.exec()) {
		qDebug() << "deviceId :  " << deviceId_ << "\t" << query.lastError().text();
		return;
	}

	//TODO 这里还要计算平均速度

	//插入最大速度
	QString strSql = QString("insert into result(queue_id,name,data) values(%1,'max_speed',%2)").arg(queueId_).arg(info.U65Info.speed);
	if (!query.exec(strSql)) {
		qDebug() << "deviceId :  " << "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " << "\t" << testDataDb.lastError().text();
		return;
	}

	return;

#ifdef _DEBUG
	//sumupQueue();
#endif
	// 

	//更新本次循环的状态
	curDoorStatus_ = ((info.U65Info.U65_MSG3 >> 9) & 0x01) ? DOOR_STATUS::CLOSE : DOOR_STATUS::OPEN;
	curMoldStatus_ = ((info.U65Info.IO1_IN >> 2) & 0x01) ? MOLD_STATUS::UP : MOLD_STATUS::DOWN;
	curTesting_ = info.isTesting();

	
	

	cur_REAL_MSG_CT_ = info.U65Info.REAL_MSG_CT;

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
		if (curTesting_) {
			qDebug() << "deviceId :  " << deviceId_ << "\t" << "testing";
		}
		else
			qDebug() <<"deviceId :  "<<deviceId_ << "\t" << "nottesting ";

		/*qDebug() << "U65Info.U65_MODE" << info.U65Info.U65_MODE;
		qDebug() << "U65Info.U65_MSG" << info.U65Info.U65_MSG;*/

	}


	//qDebug() << "catch cur_REAL_MSG_CT_" << cur_REAL_MSG_CT_;

	//recordDetail(info);


	//qDebug()<<"catch" << info.U65Info.REAL_MSG_CT;

	//处理数据
	QElapsedTimer timer;
	timer.start();
	handleRegularInfoFunc(info);
	//qDebug() << "start timerrr: " << timer.elapsed();


	//float AD1[12];//轴向位移
	//float X[12];//编码器反馈的扭矩
	//float YZ_MM[12];//角度
	//float AD2[12];//扭矩输出



	

	//记录上一次的状态
	lastDoorStatus_ = curDoorStatus_;
	lastMoldStatus_ = curMoldStatus_;
	lastTesting_ = curTesting_;

	if (last_REAL_MSG_CT_ != cur_REAL_MSG_CT_) {
		if (!curTesting_) {
			//还有东西没采集完
			lastTesting_ = true;
			qDebug() << "lastTesting_ case";
		}
		
	}

	last_REAL_MSG_CT_ = cur_REAL_MSG_CT_;
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

