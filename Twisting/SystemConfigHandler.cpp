#include "SystemConfigHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>

#include <qmessagebox.h>

SystemConfigHandler::SystemConfigHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "config-system-message";
}

SystemConfigHandler::~SystemConfigHandler()
{
}

bool SystemConfigHandler::fetchData(const QSqlDatabase &db, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery query(db);

    QString strSql = QString("select * from system_config limit 1");
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


    QJsonObject jsonObj;

    QSqlRecord record = query.record();
    while (query.next()) {
        for (int i = 1; i < record.count(); ++i) {
            QString fieldName = record.fieldName(i);
            QVariant fieldValue = query.value(i);
            jsonObj[fieldName] = QJsonValue::fromVariant(fieldValue);
        }
    }


    jsonObj["__channel"] = channel_ + "-fetchData";
    jsonObj["status"] = "success";

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}

bool SystemConfigHandler::changeLanguage(const QSqlDatabase& db, const QJsonObject& recvObj, QString& response) {
    QSqlQuery query(db);
    QString language = recvObj["language"].toString();

    QString strSql = QString("update system_config set language='%1' where id = %2").arg(language).arg(1);
    if (!query.exec(strSql)) {
        qDebug() << query.lastError().text();
        return false;
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-changeLanguage";
    QJsonDocument jsonDoc(jsonObj);

    response = jsonDoc.toJson();
    return true;
}

bool SystemConfigHandler::handleWsMsg(QJsonObject &recvObj, QString &response)
{
    QSqlDatabase db;
    if (!getConfigDB(db))
        return false;

    auto type = recvObj["__type"];
    if (type == "fetchData")
    {
        return fetchData(db, recvObj, response);
    }
    else if (type == "changeLanguage")
    {
        return changeLanguage(db, recvObj, response);
    }
    else
    {
        return false;
    }
}
