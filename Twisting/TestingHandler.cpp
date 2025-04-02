#include "TestingHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>

#include <qmessagebox.h>

TestingHandler::TestingHandler(QObject *parent)
    : MsgHandler(parent)
{
    channel_ = "data-testing-message";
}

TestingHandler::~TestingHandler()
{
}

bool TestingHandler::transferMethodPreHandle(const QSqlDatabase& configDb, const QSqlDatabase& testDb, QJsonObject &recvObj, QString &response)
{

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
    if (configForm["initialLoadTorque"].isNull())
    {
        qDebug() << "initialLoadTorque error";
        return false;
    }
    double initialLoadTorque = configForm["initialLoadTorque"].toDouble();

    if (configForm["startPoint"].isNull())
    {
        qDebug() << "startPoint error";
        return false;
    }
    double startPoint = configForm["startPoint"].toDouble();

    if (configForm["initialLoadAngle"].isNull())
    {
        qDebug() << "initialLoadAngle error";
        return false;
    }
    double initialLoadAngle = configForm["initialLoadAngle"].toDouble();
    if (configForm["initialLoadDisplacement"].isNull())
    {
        qDebug() << "initialLoadDisplacement error";
        return false;
    }
    double initialLoadDisplacement = configForm["initialLoadDisplacement"].toDouble();
    if (configForm["endCondition"].isNull())
    {
        qDebug() << "endCondition error";
        return false;
    }
    double endCondition = configForm["endCondition"].toDouble();
    if (configForm["maxTorque"].isNull())
    {
        qDebug() << "maxTorque error";
        return false;
    }
    double maxTorque = configForm["maxTorque"].toDouble();
    if (configForm["maxAngle"].isNull())
    {
        qDebug() << "maxAngle error";
        return false;
    }
    double maxAngle = configForm["maxAngle"].toDouble();
    if (configForm["breakSensitivity"].isNull())
    {
        qDebug() << "breakSensitivity error";
        return false;
    }
    double breakSensitivity = configForm["breakSensitivity"].toDouble();
    if (configForm["moveSpeed"].isNull())
    {
        qDebug() << "moveSpeed error";
        return false;
    }
    double moveSpeed = configForm["moveSpeed"].toDouble();

    QString strSql = QString("select id from method_config where is_current = 1");
    QSqlQuery configQuery(configDb);
    QSqlQuery testQuery(testDb);
    if (!configQuery.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << configQuery.lastError().text();
        return false;
    }
    int methodId = 0;
    if (configQuery.next())
    {
        methodId = configQuery.value("id").toInt();
    }

    strSql = QString("INSERT INTO queue(\
        specimen_name, batch_number, production_date, operator, lab_temperature, \
        lab_humidity, specimen_number, remarks, method_id,current)\
         VALUES ('%1','%2', '%3', '%4', %5, %6, %7, '%8', %9,1);")
                 .arg(specimenName)
                 .arg(batchNumber)
                 .arg(productionDate)
                 .arg(strOperator)
                 .arg(labTemperature)
                 .arg(labHumidity)
                 .arg(specimenNumber)
                 .arg(remarks)
                 .arg(methodId);
    if (!testQuery.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << testQuery.lastError().text();
        return false;
    }

    if (!testQuery.exec(QString("SELECT last_insert_rowid()"))) {
        qDebug() << "Failed to fetch data:";
        qDebug() << testQuery.lastError().text();
        return false;
    }
    int lastId = -1;
    if (testQuery.next()) {
        lastId = testQuery.value(0).toInt();//ID 就是最后一次插入数据自增的id
    }

    recvObj["__channel"] = "control-message";
    recvObj["queue_id"] = lastId;

    return true;
}

bool TestingHandler::isTransfer(const QJsonObject &obj)
{
    auto type = obj["__type"];
    if (type == "transfer-method")
    {
        return true;
    }
    return false;
}

bool TestingHandler::handleWsMsg(QJsonObject &recvObj, QString &response)
{
    QSqlDatabase configDb,testDb;
    if (!getConfigDB(configDb))
        return false;

    if (!getTestDataDB(testDb))
        return false;

    auto type = recvObj["__type"];
    if (type == "transfer-method")
    {
        return transferMethodPreHandle(configDb,testDb, recvObj, response);
    }
    else
    {
        return false;
    }
}
