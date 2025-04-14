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

ReportHandler::ReportHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "report-message";
}

ReportHandler::~ReportHandler()
{
}

bool ReportHandler::exportData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
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

    QSqlQuery testQuery(dataDb);

    QString strSql = QString("select * from detail join queue on detail.queue_id = queue.id  where queue.current=1 ");
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
		out << YZ_mm    ;
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

bool ReportHandler::fetchReportData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response) {
    QSqlQuery query(dataDb);

    QString strSql = QString("select result.* from result join queue on result.queue_id = queue.id  where queue.current=1;");
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
		QJsonObject jsonObj;
		jsonObj["name"] = query.value("name").toString();
		jsonObj["data"] = query.value("data").toString();
		jsonArray.append(jsonObj);
	}

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-fetch-report-data";
    jsonObj["status"] = "success";
    jsonObj["data"] = jsonArray;

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
		qDebug() << "Failed to fetch data:";
		qDebug() << testQuery.lastError().text();
		return false;
	}
	queueId = testQuery.value("id").toInt();

    strSql = QString("select * from detail join queue on detail.queue_id = queue.id  where queue.current=1 limit %1 offset %2;")
		.arg(recvObj["limit"].toInt())
		.arg(recvObj["offset"].toInt())
        ;
    if (!testQuery.exec(strSql)) {
        qDebug() << "Failed to fetch data:";
        qDebug() << testQuery.lastError().text();
        return false;
    }
    QJsonArray array;
    while (testQuery.next()) {
        QJsonObject object;
        double AD2 = testQuery.value("AD2").toDouble();
        double YZ_mm =testQuery.value("YZ_mm").toDouble();
        double AD1 =testQuery.value("AD1").toDouble();

		int id = testQuery.value("flow_number").toInt();

		object["id"] = id;
        object["AD1"] = AD1;
        object["AD2"] = AD2;
        object["YZ_mm"] = YZ_mm;

        array.append(object);
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
	else if (type == "export-data")
	{
		return exportData(configDb, testDataDb, recvObj, response);
	}
	else if (type == "fetch-report-data")
	{
		return fetchReportData(configDb, testDataDb, recvObj, response);
	}
    else
    {
        return false;
    }
}
