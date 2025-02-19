#include "MethodHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>

#include <qmessagebox.h>

MethodHandler::MethodHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "config-method-message";
}

MethodHandler::~MethodHandler()
{
}

bool MethodHandler::addData(const QSqlDatabase &db, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery query(db);

    if (!recvObj["configForm"].isObject())
    {
        qDebug() << "configForm data error";
        return false;
    }
    if (!recvObj["testModeConfig"].isObject())
    {
        qDebug() << "testModeConfig data error";
        return false;
    }
    QJsonObject configForm = recvObj["configForm"].toObject();
    QJsonObject testModeConfig = recvObj["testModeConfig"].toObject();

    if (!testModeConfig["mode"].isString())
    {
        qDebug() << "mode data error";
        return false;
    }
    QString methodName = "";
    if (recvObj["methodName"].isNull())
    {
        qDebug() << "methodName error";
        return false;
    }
    methodName = recvObj["methodName"].toString();
    QString methodRemark = "";
    if (!recvObj["methodRemark"].isNull())
    {
        methodRemark = recvObj["methodRemark"].toString();
    }

    QString specimenName = "";
    if (testModeConfig["specimenName"].isNull())
    {
        qDebug() << "specimenName error";
        return false;
    }
    specimenName = testModeConfig["specimenName"].toString();


    QString specimenNumber;
    if (testModeConfig["specimenNumber"].isNull())
    {
        qDebug() << "specimenNumber error";
        return false;
    }
    specimenNumber = testModeConfig["specimenNumber"].toString();


    QString remarks;
    if (testModeConfig["remarks"].isNull())
    {
        qDebug() << "remarks error";
        return false;
    }
    remarks = testModeConfig["remarks"].toString();

    QString batchNumber = "";
    if (testModeConfig["batchNumber"].isNull())
    {
        qDebug() << "batchNumber error";
        return false;
    }
    batchNumber = testModeConfig["batchNumber"].toString();
    QString productionDate = "";
    if (testModeConfig["productionDate"].isNull())
    {
        qDebug() << "productionDate error";
        return false;
    }
    productionDate = testModeConfig["productionDate"].toString();
    QString strOperator = "";
    if (testModeConfig["operator"].isNull())
    {
        qDebug() << "operator error";
        return false;
    }
    strOperator = testModeConfig["operator"].toString();
    double labTemperature = 0.0;
    if (testModeConfig["labTemperature"].isNull())
    {
        qDebug() << "labTemperature error";
        return false;
    }
    labTemperature = testModeConfig["labTemperature"].toDouble();
    double labHumidity = 0.0;
    if (testModeConfig["labHumidity"].isNull())
    {
        qDebug() << "labHumidity error";
        return false;
    }
    labHumidity = testModeConfig["labHumidity"].toDouble();

    double torsionSpeed = 0;
    QString torsionUnit = "";
    QString mode = testModeConfig["mode"].toString();

    QString staticMode = "";
    double constantAngle = 0;
    double constantTorque = 0;
    double cycleCount = 0;

    QString dynamicMode = "";
    double torsionFrequency = 0;
    double stepTime = 0;

    if (mode == "destructive")
    {

        if (testModeConfig["torsionSpeed"].isNull())
        {
            qDebug() << "torsionSpeed error";
            return false;
        }
        torsionSpeed = testModeConfig["torsionSpeed"].toDouble();

        if (testModeConfig["torsionUnit"].isNull())
        {
            qDebug() << "torsionUnit error";
            return false;
        }
        torsionUnit = testModeConfig["torsionUnit"].toString();
    }
    else if (mode == "static")
    {

        if (testModeConfig["torsionSpeed"].isNull())
        {
            qDebug() << "torsionSpeed error";
            return false;
        }
        torsionSpeed = testModeConfig["torsionSpeed"].toDouble();

        if (testModeConfig["torsionUnit"].isNull())
        {
            qDebug() << "torsionUnit error";
            return false;
        }
        torsionUnit = testModeConfig["torsionUnit"].toString();

        if (testModeConfig["staticMode"].isNull())
        {
            qDebug() << "staticMode error";
            return false;
        }
        staticMode = testModeConfig["staticMode"].toString();
        if (staticMode == "torque")
        {
            if (testModeConfig["constantTorque"].isNull())
            {
                qDebug() << "constantTorque error";
                return false;
            }
            constantTorque = testModeConfig["constantTorque"].toDouble();
        }
        else if (staticMode == "angle")
        {
            if (testModeConfig["constantAngle"].isNull())
            {
                qDebug() << "constantAngle error";
                return false;
            }
            constantAngle = testModeConfig["constantAngle"].toDouble();
        }

        if (testModeConfig["cycleCount"].isNull())
        {
            qDebug() << "cycleCount error";
            return false;
        }
        cycleCount = testModeConfig["cycleCount"].toDouble();
    }
    else if (mode == "dynamic")
    {
        if (testModeConfig["dynamicMode"].isNull())
        {
            qDebug() << "dynamicMode error";
            return false;
        }
        dynamicMode = testModeConfig["dynamicMode"].toString();
        if (dynamicMode == "triangle")
        {
            if (testModeConfig["stepTime"].isNull())
            {
                qDebug() << "stepTime error";
                return false;
            }
            stepTime = testModeConfig["stepTime"].toDouble();
        }

        if (testModeConfig["torsionFrequency"].isNull())
        {
            qDebug() << "torsionFrequency error";
            return false;
        }
        torsionFrequency = testModeConfig["torsionFrequency"].toDouble();

        staticMode = testModeConfig["staticMode"].toString();
        if (staticMode == "torque")
        {
            if (testModeConfig["constantTorque"].isNull())
            {
                qDebug() << "constantTorque error";
                return false;
            }
            constantTorque = testModeConfig["constantTorque"].toDouble();
        }
        else if (staticMode == "angle")
        {
            if (testModeConfig["constantAngle"].isNull())
            {
                qDebug() << "constantAngle error";
                return false;
            }
            constantAngle = testModeConfig["constantAngle"].toDouble();
        }
    }
    else
    {
        qDebug() << "mode type error";
        return false;
    }

    // 通用参数检查
    // 扭力归零
    if (configForm["initialLoadTorque"].isNull())
    {
        qDebug() << "initialLoadTorque error";
        return false;
    }
    double initialLoadTorque = configForm["initialLoadTorque"].toDouble();
    // 角度归零
    if (configForm["initialLoadAngle"].isNull())
    {
        qDebug() << "initialLoadAngle error";
        return false;
    }
    double initialLoadAngle = configForm["initialLoadAngle"].toDouble();
    // 变形归零
    if (configForm["initialLoadDisplacement"].isNull())
    {
        qDebug() << "initialLoadDisplacement error";
        return false;
    }
    double initialLoadDisplacement = configForm["initialLoadDisplacement"].toDouble();
    // 起始点
    if (configForm["startPoint"].isNull())
    {
        qDebug() << "startPoint error";
        return false;
    }
    double startPoint = configForm["startPoint"].toDouble();
    // 结束点
    if (configForm["endCondition"].isNull())
    {
        qDebug() << "endCondition error";
        return false;
    }
    double endCondition = configForm["endCondition"].toDouble();
    // 最大扭力
    if (configForm["maxTorque"].isNull())
    {
        qDebug() << "maxTorque error";
        return false;
    }
    double maxTorque = configForm["maxTorque"].toDouble();
    // 最大角度
    if (configForm["maxAngle"].isNull())
    {
        qDebug() << "maxAngle error";
        return false;
    }
    double maxAngle = configForm["maxAngle"].toDouble();
    // 断裂敏感度
    if (configForm["breakSensitivity"].isNull())
    {
        qDebug() << "breakSensitivity error";
        return false;
    }
    double breakSensitivity = configForm["breakSensitivity"].toDouble();
    // 移动速度
    if (configForm["moveSpeed"].isNull())
    {
        qDebug() << "moveSpeed error";
        return false;
    }
    double moveSpeed = configForm["moveSpeed"].toDouble();

    QString strSql = QString("select * from method_config where name = '%1'").arg(methodName);
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }
    if (query.next())
    {
        QJsonObject jsonObj;
        jsonObj["__channel"] = channel_ + "-addData";
        jsonObj["status"] = "error";
        jsonObj["message"] = "duplicate method name";
        QJsonDocument jsonDoc(jsonObj);
        response = jsonDoc.toJson();
        return true;
    }

    strSql = QString("insert into method_config(name,remark,\
specimen_name,batch_number,production_date,operator,lab_temperature,\
lab_humidity,mode,torsion_speed,torsion_unit,initial_load_torque,\
initial_load_angle,initial_load_displacement,start_point,end_condition,\
max_torque,max_angle,break_sensitivity,move_speed,static_mode,\
constant_angle,constant_torque,cycle_count,dynamic_mode,torsion_frequency,step_time,specimen_number,remarks) values('%1','%2','%3','%4','%5','%6',%7,%8,\
'%9',%10,'%11',%12,%13,%14,%15,%16,%17,%18,%19,%20,'%21',%22,%23,%24,'%25',%26,%27,'%28','%29')")
                 .arg(methodName)
                 .arg(methodRemark)
                 .arg(specimenName)
                 .arg(batchNumber)
                 .arg(productionDate)
                 .arg(strOperator)
                 .arg(labTemperature)
                 .arg(labHumidity)
                 .arg(mode)
                 .arg(torsionSpeed)
                 .arg(torsionUnit)
                 .arg(initialLoadTorque)
                 .arg(initialLoadAngle)
                 .arg(initialLoadDisplacement)
                 .arg(startPoint)
                 .arg(endCondition)
                 .arg(maxTorque)
                 .arg(maxAngle)
                 .arg(breakSensitivity)
                 .arg(moveSpeed)
                 .arg(staticMode)
                 .arg(constantAngle)
                 .arg(constantTorque)
                 .arg(cycleCount)
                 .arg(dynamicMode)
        .arg(torsionFrequency)
        .arg(stepTime)
        .arg(specimenNumber)
        .arg(remarks);
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-addData";
    jsonObj["status"] = "success";
    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}


bool MethodHandler::fetchDetail(const QSqlDatabase& db, const QJsonObject& recvObj, QString& response)
{
    QSqlQuery query(db);

    int id = recvObj["key"].toInt();
    QString strSql = QString("select * from method_config where id = %1").arg(id);
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }
    QJsonArray jsonArray;
    QSqlRecord record = query.record();
    while (query.next()) {
        QJsonObject obj1;
        obj1["key"] = query.value(0).toInt();
        for (int i = 1; i < record.count(); ++i) {
            QString fieldName = record.fieldName(i);
            QVariant fieldValue = query.value(i);
            obj1[fieldName] = QJsonValue::fromVariant(fieldValue);
        }
        jsonArray.append(obj1);
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-fetchDetail";
    jsonObj["status"] = "success";
    jsonObj["data"] = jsonArray;

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}

bool MethodHandler::fetchData(const QSqlDatabase &db, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery query(db);

    int pageSize = recvObj["pageSize"].toInt();
    int page = recvObj["page"].toInt();
    int offset = pageSize * (page - 1);

    int total = 0;
    QString strSql = QString("select count(*) as total from method_config");
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }
    if (query.next())
    {
        total = query.value("total").toInt();
    }

    strSql = QString("select * from method_config order by id limit %1 offset %2 ").arg(pageSize).arg(offset);
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }
    QJsonArray jsonArray;
    while (query.next())
    {
        QJsonObject jsonObj;
        jsonObj["key"] = query.value("id").toInt();
        jsonObj["name"] = query.value("name").toString();
        jsonObj["remark"] = query.value("remark").toString();
        jsonArray.append(jsonObj);
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-fetchData";
    jsonObj["status"] = "success";
    jsonObj["total"] = total;
    jsonObj["methods"] = jsonArray;

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}

bool MethodHandler::deleteData(const QSqlDatabase &db, const QJsonObject &recvObj, QString &response)
{
    QSqlQuery query(db);
    if (!recvObj["ids"].isArray())
    {
        QJsonObject jsonObj;
        jsonObj["__channel"] = channel_ + "-deleteData";
        jsonObj["status"] = "error";
        jsonObj["message"] = "do not has id to delete";
        return true;
    }

    QJsonArray array = recvObj["ids"].toArray();
    for (auto it = array.begin(); it != array.end(); it++)
    {
        QString strSql = QString("delete from method_config where id = %1").arg(it->toInt());
        if (!query.exec(strSql))
        {
            qDebug() << "Failed to fetch data:";
            qDebug() << query.lastError().text();
            return false;
        }
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-deleteData";
    jsonObj["status"] = "success";

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}

bool MethodHandler::handleWsMsg(QJsonObject &recvObj, QString &response)
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
    else if (type == "fetchDetail") {
        return fetchDetail(db, recvObj, response);
    }
    else if (type == "deleteData")
    {
        return deleteData(db, recvObj, response);
    }
    else
    {
        return false;
    }
}
