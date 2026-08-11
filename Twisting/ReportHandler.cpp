#include "ReportHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>
#include <qdatastream.h>
#include <qtimer.h>
#include <QElapsedTimer>
#include <qmessagebox.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <unordered_map>

#include "DataDefine.h"

ReportHandler::ReportHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "report-message";
}

ReportHandler::~ReportHandler()
{
}


std::vector<ExamplePoint> mergeAndSortPoints(const std::vector<ExamplePoint>& a, const std::vector<ExamplePoint>& b, bool removeDuplicates) {
	if (!removeDuplicates) {
		std::vector<ExamplePoint> merged = a;
		merged.insert(merged.end(), b.begin(), b.end());
		std::sort(merged.begin(), merged.end(), [](const ExamplePoint& x, const ExamplePoint& y) {
			return x.time < y.time;
			});
		return merged;
	}
	else {
		std::unordered_map<double, ExamplePoint> map;
		for (const auto& p : a) map[p.time] = p;
		for (const auto& p : b) map[p.time] = p;

		std::vector<ExamplePoint> result;
		for (const auto& kv : map) result.push_back(kv.second);

		std::sort(result.begin(), result.end(), [](const ExamplePoint& x, const ExamplePoint& y) {
			return x.time < y.time;
			});
		return result;
	}
}

bool ReportHandler::exportHistoryData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
	QFile file;
	QString filter = "CSV files (*.csv)";
	QString fileName;
	QTextStream out;

	emit fileSelect(fileName);
	if (fileName.size() == 0)
		return true;
	file.setFileName(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug() << "Failed to open file for writing:" << fileName;
		return false;
	}
	out.setDevice(&file);
	out << "CT Number";
	out << ",";
	out << "angle";
	out << ",";
	out << "torque";
	out << ",";
	out << "displacement";
	out << "\n";

	QString strMethod = obj["method"].toString();

	QSqlDatabase methodDb;
	if (!getTestDataDB("project", strMethod, methodDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}

	QSqlQuery testQuery(methodDb);

	QString strSql = QString("select * from detail where queue_id = %1").arg(obj["queue_id"].toInt());
	if (!testQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	if (testQuery.next()) {
		int totalNumber = testQuery.value("totalNumber").toInt();
		QByteArray data = testQuery.value("data").toByteArray();
		QDataStream stream(&data, QIODevice::ReadWrite);
		for (int i = 0; i < totalNumber; i++) {
			float AD1, AD2, YZ_MM, time;
			int c1, c2;
			stream >> AD1 >> AD2 >> YZ_MM >> time >> c1 >> c2;

			out << time;
			out << ",";
			out << YZ_MM;
			out << ",";
			out << AD2;
			out << ",";
			out << AD1;
			out << "\n";
		}

		

	}

	file.close();
	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-export-data";
	jsonObj["status"] = "success";
	jsonObj["message"] = "export data success";
	QJsonDocument jsonDoc1(jsonObj);
	response = jsonDoc1.toJson();


	return true;
}


bool ReportHandler::deleteHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {

	QString strMethod = obj["method"].toString();

	QSqlDatabase methodDb;
	if (!getTestDataDB("project", strMethod, methodDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}

	QSqlQuery testQuery(methodDb);

	QJsonArray arr = obj["queue_id"].toArray();
	for (const QJsonValue& value : arr)
	{
		int queueId = value.toInt();
		QString strSql = QString("delete from detail where queue_id = %1").arg(queueId);
		if (!testQuery.exec(strSql)) {
			qDebug() << "Failed to fetch data:";
			qDebug() << testQuery.lastError().text();
			return false;
		}
		strSql = QString("delete from queue where id = %1").arg(queueId);
		if (!testQuery.exec(strSql)) {
			qDebug() << "Failed to fetch data:";
			qDebug() << testQuery.lastError().text();
			return false;
		}

		strSql = QString("delete from result where queue_id = %1").arg(queueId);
		if (!testQuery.exec(strSql)) {
			qDebug() << "Failed to fetch data:";
			qDebug() << testQuery.lastError().text();
			return false;
		}

	}



	
	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-delete-history-data";
	jsonObj["status"] = "success";
	jsonObj["message"] = "delete data success";
	QJsonDocument jsonDoc1(jsonObj);
	response = jsonDoc1.toJson();
	return true;
}

extern double findClosestAngle(
	const std::vector<TwistingData>& data,
	double targetTorque,
	const char* phase = "all" // "rise", "fall", "rise2", "all"
);

extern double findClosestTorque(
	const std::vector<TwistingData>& data,
	double targetAngle,
	const char* phase = "all" // "rise", "fall", "rise2", "all"
);

extern void sumupQueueFunc(
	const QSqlDatabase& configDb,
	const QSqlDatabase& testDataDb,
	const int queueId_
);

bool ReportHandler::recalculateHistoryData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
	
	QString strMethod = obj["method"].toString();

	QSqlDatabase testDataDb;
	if (!getTestDataDB("project", strMethod, testDataDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}
	QJsonArray arr = obj["queue_id"].toArray();
	for (const QJsonValue& value : arr)
	{
		int queueId = value.toInt();
		sumupQueueFunc(configDb, testDataDb, queueId);
	}

	


	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-recalculate-history-data";
	jsonObj["status"] = "success";
	QJsonDocument jsonDoc1(jsonObj);
	response = jsonDoc1.toJson();

	return true;
}

bool ReportHandler::exportData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
	QFile file;
	QString filter = "CSV files (*.csv)";
	QString fileName;
	QTextStream out;

	emit fileSelect(fileName);
	if (fileName.size() == 0) return true;

	file.setFileName(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug() << "Failed to open file for writing:" << fileName;
		return false;
	}

	// 设置较大的缓冲区

	out.setDevice(&file);

	// 写入CSV表头 (一次性写入)
	out << "CT Number,angle,torque,displacement\n";

	// 应用SQLite性能优化的PRAGMA指令
	QSqlQuery pragmaQuery(dataDb);

	// 关闭同步写入 - 速度提升但降低安全性，仅用于导出等非关键操作
	pragmaQuery.exec("PRAGMA synchronous = OFF");

	// 将日志模式更改为内存模式
	pragmaQuery.exec("PRAGMA journal_mode = MEMORY");

	// 增加缓存大小
	pragmaQuery.exec("PRAGMA cache_size = 10000");

	// 关闭临时存储区域 - 减少磁盘I/O
	pragmaQuery.exec("PRAGMA temp_store = MEMORY");

	// 开启较大的内存映射
	pragmaQuery.exec("PRAGMA mmap_size = 30000000000");

	// 开始事务

	const int BATCH_SIZE = 50000; // 每批处理50000条记录
	int offset = 0;
	bool hasMoreData = true;
	if (1) {
		QSqlQuery testQuery(dataDb);
		QString strSql = QString("SELECT detail.* from detail JOIN queue ON detail.queue_id = queue.id "
			"WHERE queue.current=1 ");
		if (!testQuery.exec(strSql)) {
			qDebug() << "Failed to fetch data:" << testQuery.lastError().text();
			file.close();
			return false;
		}
		if (!testQuery.next()) {
			qDebug() << "Failed to fetch data:" << testQuery.lastError().text();
			file.close();
			return false;

		}
		int totalNumber = testQuery.value("totalNumber").toInt();
		QByteArray data = testQuery.value("data").toByteArray();
		QDataStream stream(&data, QIODevice::ReadWrite);

		QString batchData;
		for (int i = 0; i < totalNumber; i++) {
			float AD1, AD2, YZ_MM, time;
			int c1, c2;
			stream >> AD1 >> AD2 >> YZ_MM >> time >> c1 >> c2;
				
			batchData = QString("%1,%2,%3,%4\n")
			.arg(time)
			.arg(YZ_MM)
			.arg(AD2)
			.arg(AD1);
			out << batchData;
		}
	}

	while (false) {
		QSqlQuery testQuery(dataDb);
		QString strSql = QString("SELECT detail.flow_number, detail.AD2, detail.YZ_mm, detail.AD1 "
			"FROM detail JOIN queue ON detail.queue_id = queue.id "
			"WHERE queue.current=1 "
			"LIMIT %1 OFFSET %2").arg(BATCH_SIZE).arg(offset);

		if (!testQuery.exec(strSql)) {
			qDebug() << "Failed to fetch data:" << testQuery.lastError().text();
			file.close();
			return false;
		}

		// 收集批量数据然后一次写入
		QString batchData;
		int rowCount = 0;

		while (testQuery.next()) {
			rowCount++;
			double CT = testQuery.value("flow_number").toDouble();
			double AD2 = testQuery.value("AD2").toDouble();
			double YZ_mm = testQuery.value("YZ_mm").toDouble();
			double AD1 = testQuery.value("AD1").toDouble();

			batchData += QString("%1,%2,%3,%4\n")
				.arg(CT)
				.arg(AD2)
				.arg(YZ_mm)
				.arg(AD1);
		}

		// 一次性写入批量数据
		out << batchData;

		// 检查是否还有更多数据
		if (rowCount < BATCH_SIZE) {
			hasMoreData = false;
		}

		offset += BATCH_SIZE;

		// 提供进度信息
		qDebug() << "Processed" << offset << "records...";
	}

	// 提交事务

	// 恢复SQLite默认设置
	pragmaQuery.exec("PRAGMA synchronous = NORMAL");
	pragmaQuery.exec("PRAGMA journal_mode = DELETE");
	pragmaQuery.exec("PRAGMA cache_size = 2000");
	pragmaQuery.exec("PRAGMA temp_store = DEFAULT");

	file.close();

	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-export-data";
	jsonObj["status"] = "success";
	jsonObj["message"] = "export data success";
	QJsonDocument jsonDoc1(jsonObj);
	response = jsonDoc1.toJson();

	return true;
}



bool ReportHandler::fetchReportData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
    QSqlQuery query(dataDb);
    //先查询total
	QString strSql = QString("select count(*) as total from queue where show=1;");
	if (!query.exec(strSql))
	{
		QString strErr = "Failed to fetch data:";
		qDebug() << strErr << query.lastError().text();
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-fetchData";
		jsonObj["status"] = "error";
		jsonObj["message"] = strErr;
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}
	int total = 0;
	if (query.next())
	{
		total = query.value("total").toInt();
	}

    int pageSize = obj["pageSize"].toInt();
    int page = obj["page"].toInt();
    int offset = pageSize * (page - 1);
	strSql = QString("select * from queue where show=1 order by id desc limit %1 offset %2 ")
		.arg(pageSize).arg(offset);
	if (!query.exec(strSql))
	{
		QString strErr = "Failed to fetch data:";
		qDebug() << strErr << query.lastError().text();
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-fetchData";
		jsonObj["status"] = "error";
		jsonObj["message"] = strErr;
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}
	QVector<int> queueIdSet;
	while (query.next())
	{
		int queueId = query.value("id").toInt();
		queueIdSet.push_back(queueId);
	}


    QJsonArray jsonArray;
	QJsonArray jsonColumns;
	QSet<QString> columnSet;
	jsonColumns.push_back("queue_id");
	//再查询数据
	for (auto it = queueIdSet.begin(); it != queueIdSet.end(); it++)
	{
		QJsonObject jsonQueueObj;
		jsonQueueObj["queue_id"] = *it;
		strSql = QString("select * from result where queue_id = %1").arg(*it);
		if (!query.exec(strSql))
		{
			QString strErr = "Failed to fetch data:";
			qDebug() << strErr << query.lastError().text();
			QJsonObject jsonObj;
			jsonObj["__channel"] = channel_ + "-fetchData";
			jsonObj["status"] = "error";
			jsonObj["message"] = strErr;
			QJsonDocument jsonDoc(jsonObj);
			response = jsonDoc.toJson();
			return true;
		}
		while (query.next())
		{
			int queueId = query.value("queue_id").toInt();
            QString name = query.value("name").toString();
            QString data = query.value("data").toString();
			jsonQueueObj[name] = data;
			if (!columnSet.contains(name))
			{
				jsonColumns.append(name);
				columnSet.insert(name);
			}
		}
		jsonArray.append(jsonQueueObj);
	}


   

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-fetch-report-data";
    jsonObj["status"] = "success";
    jsonObj["data"] = jsonArray;    
    jsonObj["columns"] = jsonColumns;
	jsonObj["total"] = total;

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}


void ReportHandler::sumupQueue(const QSqlDatabase& configDb, const QSqlDatabase& testDataDb) {
	//读取report_setting
	QString strSql = QString("select * from report_setting ");
	QSqlQuery configQuery(configDb);
	if (!configQuery.exec(strSql)) {
		qDebug() << "deviceId :  " << deviceId_ << "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " << deviceId_ << "\t" << configDb.lastError().text();
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
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("stiffness error");
				continue;
			}

			bool ok1, ok2;
			double torque = list[0].toDouble(&ok1);
			int twistCount = list[1].toInt(&ok2);
			if (!ok1 || !ok2) {
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("stiffness error");
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
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("stiffness error");
				continue;
			}

			bool ok1, ok2;
			double angle = list[0].toDouble(&ok1);
			int twistCount = list[1].toInt(&ok2);
			if (!ok1 || !ok2) {
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("stiffness error");
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
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("stiffness error");
				continue;
			}
			bool ok1, ok2, ok3;
			double torque = list[0].toDouble(&ok1);
			double torque2 = list[1].toDouble(&ok2);
			int twistCount = list[2].toInt(&ok3);
			if (!ok1 || !ok2 || !ok3) {
				qDebug() << "deviceId :  " << deviceId_ << "\t" << QStringLiteral("stiffness error");
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
		qDebug() << "deviceId :  " << deviceId_ << "\t" << "Failed to fetch data:";
		qDebug() << "deviceId :  " << deviceId_ << "\t" << configDb.lastError().text();
		return;
	}
	if (configQuery.next()) {
		testMode = configQuery.value("mode").toString();
	}

	QVector< TwistingData> vecTwistingData_;// 记录当前的测试数据

	QMap<int, std::vector<TwistingData>> perTwistingData;
	{
#ifdef _DEBUG
		//queueId_ = 67;
#endif
		strSql = QString("select * from detail where queue_id=%1").arg(229);
		QSqlQuery testQuery(testDataDb);
		if (!testQuery.exec(strSql)) {
			qDebug() << "deviceId :  " << deviceId_ << "\t" << "Failed to fetch data:";
			qDebug() << "deviceId :  " << deviceId_ << "\t" << testDataDb.lastError().text();
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

				vecTwistingData_.push_back(twistingData);
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
	if (vecTwistingData_.size()) {
		lastAngle = vecTwistingData_.at(0).angle;
		lastTorque = vecTwistingData_.at(0).torque;
	}

	//数据是类似一个sin波形，根据此属性，找到对应的报告数据
	//一开始的扭矩和角度都是0
	for (auto& it : vecTwistingData_) {

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


	{
		//插入角度最大值
		strSql = QString("insert into result(queue_id,name,data) values(%1,'maxAngleToTorque',%2)").arg(229).arg(angleToTorquePair.second);

		
		//插入扭矩最大值
		strSql = QString("insert into result(queue_id,name,data) values(%1,'maxTorqueToAngle',%2)").arg(229).arg(torqueToAnglePair.second);
		
	}


}


bool ReportHandler::liveTestingData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery testQuery(dataDb);
    int queueId  =0; 
	QString strSql = QString("select * from queue where current=1");
	if (!testQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	if (!testQuery.next()) {		
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-live-testing-data";
		jsonObj["queue_id"] = 0;
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}
	queueId = testQuery.value("id").toInt();

	size_t total = testQuery.value("totalNumber").toInt();
	QByteArray data = testQuery.value("raw_data").toByteArray();

	const int DATA_COUNT = 5000;

    QJsonArray array;

	QDataStream stream(data);
	for (size_t i = 0; i < total; i++) {
		qint64 sampleTimeUs;
		double torque;
		stream >> sampleTimeUs >> torque;
	}
	

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-live-testing-data";
    jsonObj["data"] = array;
    jsonObj["offset"] = recvObj["offset"].toInt();
    jsonObj["queue_id"] = queueId;
    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}


bool ReportHandler::fetchTestHistoryDetail(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& recvObj, QString& response)
{
	QString strMethod = recvObj["method"].toString();

	QSqlDatabase methodDb;
	if (!getTestDataDB("project", strMethod, methodDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}

	QSqlQuery testQuery(methodDb);
	size_t total = 0;
	int mod = 0;

	const int DATA_COUNT = 5000;//保留样本数量
	std::vector<ExamplePoint> points;
	QString strSql = QString("select * from detail where queue_id = %1 ;").arg(recvObj["req_queue_id"].toInt());
	if (!testQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	if (testQuery.next()) {
		total = testQuery.value("totalNumber").toInt();
		QByteArray data = testQuery.value("data").toByteArray();
		QDataStream stream(&data, QIODevice::ReadWrite);
		for (size_t i = 0; i < total; i++) {
			ExamplePoint point;
			float AD1, AD2, YZ_MM, time;
			int c1, c2;
			stream >> AD1 >> AD2 >> YZ_MM >> time >> c1 >> c2;
			point.AD1 = AD1;
			point.AD2 = AD2;
			point.YZ_mm = YZ_MM;
			point.time = time;
			points.push_back(point);
		}
	}
	else {
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-fetch-test-history-detail";
		jsonObj["status"] = "error";
		jsonObj["message"] = "no data";
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}
	

	QJsonArray array;


	if (0) {
		//不过滤

		for (auto& it : points) {
			QJsonObject object;
			object["id"] = it.time;
			object["AD1"] = it.AD1;
			object["AD2"] = it.AD2;
			object["YZ_mm"] = it.YZ_mm;
			array.append(object);
		}
	}
	else {
		total = points.size();
		if (total > DATA_COUNT) {
			total = DATA_COUNT;
		}

		std::vector<ExamplePoint> outPoints1;
		std::vector<ExamplePoint> outPoints2;
		LttbAD2_YZmm::Downsample(points.begin(), points.size(), std::back_inserter(outPoints1), total);
		LttbAD2_AD1::Downsample(points.begin(), points.size(), std::back_inserter(outPoints2), total);

		auto merged = mergeAndSortPoints(outPoints1, outPoints2, true); // 去重合并
		for (auto& it : merged) {
			QJsonObject object;
			object["id"] = it.time;
			object["AD1"] = it.AD1;
			object["AD2"] = it.AD2;
			object["YZ_mm"] = it.YZ_mm;
			array.append(object);
		}
	}


	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-fetch-test-history-detail";
	jsonObj["data"] = array;
	QJsonDocument jsonDoc(jsonObj);
	response = jsonDoc.toJson();
	return true;
}


bool ReportHandler::fetchHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
	
	QString strMethod = obj["method"].toString();
	QString strGroup = "";
	if (!obj["group"].isNull()) {
		strGroup = obj["group"].toString();	
	}

	QSqlDatabase methodDb;
	if (!getTestDataDB("project", strMethod, methodDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}


	QSqlQuery query(methodDb);
	//先查询total
	QString strSql = QString("select count(*) as total from queue ;");
	if (!strGroup.isEmpty()) {
		strSql = QString("select count(*) as total from queue where remarks = '%1' ;").arg(strGroup);
	}

	if (!query.exec(strSql))
	{
		QString strErr = "Failed to fetch data:";
		qDebug() << strErr << query.lastError().text();
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-fetchData";
		jsonObj["status"] = "error";
		jsonObj["message"] = strErr;
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}
	int total = 0;
	if (query.next())
	{
		total = query.value("total").toInt();
	}

	int pageSize = obj["pageSize"].toInt();
	int page = obj["page"].toInt();
	int offset = pageSize * (page - 1);
	strSql = QString("select * from queue order by id desc limit %1 offset %2 ")
		.arg(pageSize).arg(offset);
	if (!strGroup.isEmpty()) {
		strSql = QString("select * from queue where remarks = '%1' order by id desc limit %2 offset %3 ")
			.arg(strGroup).arg(pageSize).arg(offset);
	}
	if (!query.exec(strSql))
	{
		QString strErr = "Failed to fetch data:";
		qDebug() << strErr << query.lastError().text();
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-fetchData";
		jsonObj["status"] = "error";
		jsonObj["message"] = strErr;
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}

	QJsonArray groupArray;
	QSet<QString> groupSet;

	QJsonArray jsonArray;
	while (query.next())
	{
		QJsonObject jsonQueueObj;
		jsonQueueObj["key"] = query.value("id").toInt();
		jsonQueueObj["specimen_name"] = query.value("specimen_name").toString();
		jsonQueueObj["batch_number"] = query.value("batch_number").toString();
		jsonQueueObj["production_date"] = query.value("production_date").toString();
		jsonQueueObj["operator"] = query.value("operator").toString();
		jsonQueueObj["lab_temperature"] = query.value("lab_temperature").toDouble();
		jsonQueueObj["lab_humidity"] = query.value("lab_humidity").toDouble();
		jsonQueueObj["specimen_number"] = query.value("specimen_number").toString();
		QString group = query.value("remarks").toString();//把remarks作为分组依据
		jsonQueueObj["remarks"] = group;
		if (!groupSet.contains(group))
		{
			groupSet.insert(group);
			groupArray.append(group);
		}
		jsonArray.append(jsonQueueObj);
	}


	QJsonArray jsonColumns;
	jsonColumns.push_back("key");
	jsonColumns.push_back("specimen_name");
	jsonColumns.push_back("batch_number");
	jsonColumns.push_back("production_date");
	jsonColumns.push_back("operator");
	jsonColumns.push_back("lab_temperature");
	jsonColumns.push_back("lab_humidity");
	jsonColumns.push_back("specimen_number");
	jsonColumns.push_back("remarks");


	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-fetch-history-data";
	jsonObj["status"] = "success";
	jsonObj["data"] = jsonArray;
	jsonObj["columns"] = jsonColumns;
	jsonObj["group"] = groupArray;
	jsonObj["total"] = total;

	QJsonDocument jsonDoc(jsonObj);
	response = jsonDoc.toJson();
	return true;
}



bool ReportHandler::fetchReportHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
	QString strMethod = obj["method"].toString();

	QSqlDatabase methodDb;
	if (!getTestDataDB("project", strMethod, methodDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}
	
	
	QSqlQuery query(methodDb);
	int total = 1;

	QString strSql = QString("select * from queue where id = %1 ").arg(obj["req_queue_id"].toInt());
	if (!query.exec(strSql))
	{
		QString strErr = "Failed to fetch data:";
		qDebug() << strErr << query.lastError().text();
		QJsonObject jsonObj;
		jsonObj["__channel"] = channel_ + "-fetchData";
		jsonObj["status"] = "error";
		jsonObj["message"] = strErr;
		QJsonDocument jsonDoc(jsonObj);
		response = jsonDoc.toJson();
		return true;
	}
	QVector<int> queueIdSet;
	while (query.next())
	{
		int queueId = query.value("id").toInt();
		queueIdSet.push_back(queueId);
	}


	QJsonArray jsonArray;
	QJsonArray jsonColumns;
	QSet<QString> columnSet;
	jsonColumns.push_back("queue_id");
	//再查询数据
	for (auto it = queueIdSet.begin(); it != queueIdSet.end(); it++)
	{
		QJsonObject jsonQueueObj;
		jsonQueueObj["queue_id"] = *it;
		strSql = QString("select * from result where queue_id = %1").arg(*it);
		if (!query.exec(strSql))
		{
			QString strErr = "Failed to fetch data:";
			qDebug() << strErr << query.lastError().text();
			QJsonObject jsonObj;
			jsonObj["__channel"] = channel_ + "-fetchData";
			jsonObj["status"] = "error";
			jsonObj["message"] = strErr;
			QJsonDocument jsonDoc(jsonObj);
			response = jsonDoc.toJson();
			return true;
		}
		while (query.next())
		{
			int queueId = query.value("queue_id").toInt();
			QString name = query.value("name").toString();
			QString data = query.value("data").toString();
			jsonQueueObj[name] = data;
			if (!columnSet.contains(name))
			{
				jsonColumns.append(name);
				columnSet.insert(name);
			}
		}
		jsonArray.append(jsonQueueObj);
	}




	QJsonObject jsonObj;
	jsonObj["__channel"] = channel_ + "-fetch-report-history-data";
	jsonObj["status"] = "success";
	jsonObj["data"] = jsonArray;
	jsonObj["columns"] = jsonColumns;
	jsonObj["total"] = total;

	QJsonDocument jsonDoc(jsonObj);
	response = jsonDoc.toJson();
	return true;
}


bool ReportHandler::handleWsMsg(QJsonObject &recvObj, QString &response)
{
    QSqlDatabase configDb;
    if (!getConfigDB(configDb))
        return false;

    QSqlDatabase testDataDb;
    if (!getTestDataDB(testDataDb)) {
        qDebug() << "getTestDataDB";
        return false;
    }

    auto type = recvObj["__type"];
    if (type == "live-testing-data")
    {
		//计算耗时
		//QElapsedTimer timer;
		//timer.start();
		//qDebug() << "start time: " << timer.elapsed();
		bool ret = liveTestingData(configDb, testDataDb, recvObj, response);
		//qDebug() << "end time: " << timer.elapsed();
		return ret;
    }
	else if (type == "fetch-test-history-detail") {
		return fetchTestHistoryDetail(configDb, testDataDb, recvObj, response);
	}
	else if (type == "export-data")
	{
		return exportData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "export-history-data")
	{
		return exportHistoryData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "delete-history-data")
	{
		return deleteHistoryData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "recalculate-history-data") {
		return recalculateHistoryData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "fetch-report-data")
	{
		return fetchReportData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "fetch-history-data")
	{
		return fetchHistoryData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "fetch-report-history-data") {
		return fetchReportHistoryData(configDb, testDataDb, recvObj, response);
	}
    else
    {
        return false;
    }
}
