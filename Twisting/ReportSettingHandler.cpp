#include "ReportSettingHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>

#include <qmessagebox.h>

ReportSettingHandler::ReportSettingHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "config-report-message";
}

ReportSettingHandler::~ReportSettingHandler()
{
}

bool ReportSettingHandler::addData(const QSqlDatabase &db, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery query(db);
    if (!recvObj["reportMetas"].isArray()) {
        QJsonObject jsonObj;
        jsonObj["__channel"] = channel_ + "-addData";
        jsonObj["status"] = "error";
        jsonObj["message"] = "reportMetas error";
        QJsonDocument jsonDoc(jsonObj);
        response = jsonDoc.toJson();
        return true;
    }
    QString strSql = QString("delete from report_setting");
    if (!query.exec(strSql)) {
        QString strErr = "Failed to delete data:";
        qDebug() << strErr << query.lastError().text();

        QJsonObject jsonObj;
        jsonObj["__channel"] = channel_ + "-addData";
        jsonObj["status"] = "error";
        jsonObj["message"] = strErr;
        QJsonDocument jsonDoc(jsonObj);
        response = jsonDoc.toJson();
        return true;
    }
    QJsonArray array = recvObj["reportMetas"].toArray();
    for (auto it = array.begin(); it != array.end(); it++) {
        strSql = QString("insert into report_setting(name,memo) values('%1','%2')")
            .arg(it->toString())
            .arg("");

        if (!query.exec(strSql)) {
            QString strErr = "Failed to insert data:";
            qDebug() << strErr << query.lastError().text();

            QJsonObject jsonObj;
            jsonObj["__channel"] = channel_ + "-addData";
            jsonObj["status"] = "error";
            jsonObj["message"] = strErr;
            QJsonDocument jsonDoc(jsonObj);
            response = jsonDoc.toJson();
            return true;
        }
    }
   

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-addData";
    jsonObj["status"] = "success";
    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}



bool ReportSettingHandler::fetchData(const QSqlDatabase &db, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery query(db);

    QString strSql = QString("select * from report_setting");
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
        jsonArray.append(query.value("name").toString());
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-fetchData";
    jsonObj["status"] = "success";
    jsonObj["reportMetas"] = jsonArray;

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}


bool ReportSettingHandler::handleWsMsg(QJsonObject &recvObj, QString &response)
{
    QSqlDatabase db;
    if (!getConfigDB(db))
        return false;

    auto type = recvObj["__type"];
    if (type == "addData")
    {
        return addData(db, recvObj, response);
    }
    else if (type == "fetchData")
    {
        return fetchData(db, recvObj, response);
    }
    else
    {
        return false;
    }
}
