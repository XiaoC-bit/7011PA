#include "ReportHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>

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
			return x.id < y.id;
			});
		return merged;
	}
	else {
		std::unordered_map<int, ExamplePoint> map;
		for (const auto& p : a) map[p.id] = p;
		for (const auto& p : b) map[p.id] = p;

		std::vector<ExamplePoint> result;
		for (const auto& kv : map) result.push_back(kv.second);

		std::sort(result.begin(), result.end(), [](const ExamplePoint& x, const ExamplePoint& y) {
			return x.id < y.id;
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
	while (testQuery.next()) {
		QJsonObject object;
		double AD2 = testQuery.value("AD2").toDouble();
		double YZ_mm = testQuery.value("YZ_mm").toDouble();
		double AD1 = testQuery.value("AD1").toDouble();
		int CT = testQuery.value("flow_number").toInt();

		out << CT;
		out << ",";
		out << AD2;
		out << ",";
		out << YZ_mm;
		out << ",";
		out << AD1;
		out << "\n";

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

	while (hasMoreData) {
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
			int CT = testQuery.value("flow_number").toInt();
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

	strSql = QString("select count(*) as total from detail where queue_id=%1;").arg(queueId);
	if (!testQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	size_t total = 0;
	if (testQuery.next()) {
		total = testQuery.value("total").toInt();
	}
	int mod = 0;
	if (total < 10000) {
		mod = 1;
	}
	else if (total < 10000) {
		mod = 2;
	}
	else if (total < 100000) {
		mod = 5;
	}
	else if (total < 1000000) {
		mod = 10;
	}
	else {
		mod = 20;
	}

	strSql = QString("select detail.* from detail join queue on detail.queue_id = queue.id  where queue.current=1 and flow_number %%1=0").arg(mod);
    if (!testQuery.exec(strSql)) {
        qDebug() << "Failed to fetch data:";
        qDebug() << testQuery.lastError().text();
        return false;
    }

	const int DATA_COUNT = 500;//保留样本数量

    QJsonArray array;

	std::vector<ExamplePoint> points;

    while (testQuery.next()) {

		ExamplePoint point;
		point.AD1 = testQuery.value("AD1").toDouble();
		point.AD2 = testQuery.value("AD2").toDouble();
		point.YZ_mm = testQuery.value("YZ_mm").toDouble();
		point.id = testQuery.value("flow_number").toInt();
		points.push_back(point);
		continue;
    }
	if (0) {

		//不过滤
		
		for (auto& it : points) {
			QJsonObject object;
			object["id"] = it.id;
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
			object["id"] = it.id;
			object["AD1"] = it.AD1;
			object["AD2"] = it.AD2;
			object["YZ_mm"] = it.YZ_mm;
			array.append(object);
		}
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
	QString strSql = QString("select count(*) as total from detail where queue_id = %1 ;").arg(recvObj["req_queue_id"].toInt());
	if (!testQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	if (testQuery.next()) {
		total = testQuery.value("total").toInt();
		if (total < 10000) {
			mod = 1;
		}
		else if (total < 10000) {
			mod = 2;
		}
		else if (total < 100000) {
			mod = 5;
		}
		else if (total < 1000000) {
			mod = 10;
		}
		else {
			mod = 20;
		}
	}
	

	strSql = QString("select * from detail where queue_id = %1 and flow_number %%2=0;")
		.arg(recvObj["req_queue_id"].toInt())
		.arg(mod)
		;

	if (!testQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	const int DATA_COUNT = 500;//保留样本数量
	std::vector<ExamplePoint> points;
	QJsonArray array;
	while (testQuery.next()) {
		ExamplePoint point;
		point.AD1 = testQuery.value("AD1").toDouble();
		point.AD2 = testQuery.value("AD2").toDouble();
		point.YZ_mm = testQuery.value("YZ_mm").toDouble();
		point.id = testQuery.value("flow_number").toInt();
		points.push_back(point);
		continue;

		
	}

	if (0) {
		//不过滤

		for (auto& it : points) {
			QJsonObject object;
			object["id"] = it.id;
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
			object["id"] = it.id;
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

	QSqlDatabase methodDb;
	if (!getTestDataDB("project", strMethod, methodDb))
	{
		qDebug() << "getTestDataDB";
		return false;
	}


	QSqlQuery query(methodDb);
	//先查询total
	QString strSql = QString("select count(*) as total from queue ;");
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
		jsonQueueObj["remarks"] = query.value("remarks").toString();
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
        return liveTestingData(configDb, testDataDb, recvObj, response);
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
