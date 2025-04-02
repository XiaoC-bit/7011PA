#include "ReportHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>

#include <qmessagebox.h>

ReportHandler::ReportHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "report-message";
}

ReportHandler::~ReportHandler()
{
}

bool ReportHandler::liveTestingData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery testQuery(dataDb);

    QString strSql = QString("select * from detail join queue on detail.queue_id = queue.id  where queue.current=1 ");
    if (!testQuery.exec(strSql)) {
        qDebug() << "Failed to fetch data:";
        qDebug() << testQuery.lastError().text();
        return false;
    }
    QJsonArray array;
    while (testQuery.next()) {
        QJsonObject object;
        double torque, angle, yz_mm;
        angle= testQuery.value("angle").toDouble();
        torque=testQuery.value("torque").toDouble();
        yz_mm=testQuery.value("YZ_mm").toDouble();

        object["angle"] = angle;
        object["torque"] = torque;
        object["yz_mm"] = yz_mm;

        array.append(object);
    }



    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-live-testing-data";
    jsonObj["data"] = array;
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
    else
    {
        return false;
    }
}
