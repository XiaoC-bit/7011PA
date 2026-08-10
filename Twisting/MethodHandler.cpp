#include "MethodHandler.h"

#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qjsonarray.h>
#include <qsqldatabase.h>
#include <qjsondocument.h>
#include <qserialportinfo.h>

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


    QString specimenNumber = "";
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
    double delayTime = 0;

    QString dynamicMode = "";
    double torsionFrequency = 0;
    double stepTime = 0;

    QString direction = "";
    QString ad_direction = "";

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


        if (testModeConfig["direction"].isNull())
        {
            qDebug() << "direction error";
            return false;
        }
        direction = testModeConfig["direction"].toString();

		if (testModeConfig["adDirection"].isNull())
		{
			qDebug() << "adDirection error";
			return false;
		}
		ad_direction = testModeConfig["adDirection"].toString();

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

		if (testModeConfig["delayTime"].isNull())
		{
			qDebug() << "delayTime error";
			return false;
		}
		delayTime = testModeConfig["delayTime"].toDouble();
    }
    else if (mode == "dynamic")
    {
        if (testModeConfig["cycleCount"].isNull())
        {
            qDebug() << "cycleCount error";
            return false;
        }
        cycleCount = testModeConfig["cycleCount"].toDouble();
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

    // 通用参数配置
    if (configForm["initialMode"].isNull())
    {
        qDebug() << "initialMode error";
        return false;
    }
    auto initialMode = configForm["initialMode"].toString();
    // 角度归零
    if (configForm["initialLoadValue"].isNull())
    {
        qDebug() << "initialLoadValue error";
        return false;
    }
    double initialLoadValue = configForm["initialLoadValue"].toDouble();
    // 扭力单位
    if (configForm["unit"].isNull())
    {
        qDebug() << "unit error";
        return false;
    }
    auto unit = configForm["unit"].toString();

	//清零方式
	if (configForm["zeroMode"].isNull())
	{
		qDebug() << "zeroMode error";
		return false;
	}
    //zeroMode是一个字符串数组
	// 取出每一个元素
	QJsonArray zeroModeArray = configForm["zeroMode"].toArray();
	if (zeroModeArray.isEmpty())
	{
		qDebug() << "zeroMode array is empty";
		return false;
	}
	//遍历获取数组中对象，都是字符串
	QStringList zeroModeList;
	for (const QJsonValue& value : zeroModeArray)
	{
		if (value.isString())
		{
			QString str = value.toString();
			zeroModeList.append(str);
		}
		else
		{
			qDebug() << "zeroMode array contains non-string value";
			return false;
		}
	}
	//将字符串列表转换为逗号分隔的字符串
	QString zeroMode = zeroModeList.join(",");



    // 初始点
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
    // 最大扭矩
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
    // 断裂判定
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

    //是否复位
    if (configForm["specimenReturn"].isNull())
    {
        qDebug() << "specimenReturn error";
        return false;
    }
	int specimenReturn = configForm["specimenReturn"].toInt();

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

    strSql = QString("update method_config set is_current = 0");
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }

    strSql = QString("insert into method_config(name,remark,\
specimen_name,batch_number,production_date,operator,lab_temperature,\
lab_humidity,mode,torsion_speed,torsion_unit,initial_mode,\
initial_load_value,unit,zero_mode,start_point,end_condition,\
max_torque,max_angle,break_sensitivity,move_speed,static_mode,\
constant_angle,constant_torque,cycle_count,dynamic_mode,torsion_frequency,step_time,specimen_number,remarks,is_current,delay_time,direction,specimenReturn,ad_direction) values('%1','%2','%3','%4','%5','%6',%7,%8,\
'%9',%10,'%11','%12',%13,'%14','%15',%16,%17,%18,%19,%20,%21,'%22',%23,%24,%25,'%26',%27,%28,'%29','%30',%31,%32,'%33',%34,'%35')")
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
                 .arg(initialMode)
                 .arg(initialLoadValue)
                .arg(unit)
                .arg(zeroMode)
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
        .arg(remarks)
        .arg(1)
        .arg(delayTime)
        .arg(direction)
        .arg(specimenReturn)
        .arg(ad_direction);
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


bool MethodHandler::modifyData(const QSqlDatabase& db, const QJsonObject& recvObj, QString& response)
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


    double setAngle = 0.0;
    if (testModeConfig["setAngle"].isNull())
    {
        qDebug() << "setAngle error";
        return false;
    }
    setAngle = testModeConfig["setAngle"].toDouble();



    double torsionSpeed = 0;
    QString torsionUnit = "";
    QString mode = testModeConfig["mode"].toString();

    QString staticMode = "";
    double constantAngle = 0;
    double constantTorque = 0;
    double cycleCount = 0;
    double delayTime = 0;

    QString dynamicMode = "";
    double torsionFrequency = 0;
    double stepTime = 0;
    QString direction = "";
    QString ad_direction = "";


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


        if (testModeConfig["direction"].isNull())
        {
            qDebug() << "direction error";
            return false;
        }
        direction = testModeConfig["direction"].toString();

		if (testModeConfig["adDirection"].isNull())
		{
			qDebug() << "adDirection error";
			return false;
		}
		ad_direction = testModeConfig["adDirection"].toString();
        
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


        //delay_time
		if (testModeConfig["delayTime"].isNull())
		{
			qDebug() << "delayTime error";
			return false;
		}
		delayTime = testModeConfig["delayTime"].toDouble();

    }
    else if (mode == "dynamic")
    {
        if (testModeConfig["cycleCount"].isNull())
        {
            qDebug() << "cycleCount error";
            return false;
        }
        cycleCount = testModeConfig["cycleCount"].toDouble();
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

    // 通用参数配置
    if (configForm["initialMode"].isNull())
    {
        qDebug() << "initialMode error";
        return false;
    }
    auto initialMode = configForm["initialMode"].toString();
    // 角度归零
    if (configForm["initialLoadValue"].isNull())
    {
        qDebug() << "initialLoadValue error";
        return false;
    }
    double initialLoadValue = configForm["initialLoadValue"].toDouble();
    // 扭力单位
    if (configForm["unit"].isNull())
    {
        qDebug() << "unit error";
        return false;
    }
    auto unit = configForm["unit"].toString();

    //清零方式
    if (configForm["zeroMode"].isNull())
    {
        qDebug() << "zeroMode error";
        return false;
    }
    //zeroMode是一个字符串数组
    // 取出每一个元素
    QJsonArray zeroModeArray = configForm["zeroMode"].toArray();
    if (zeroModeArray.isEmpty())
    {
        qDebug() << "zeroMode array is empty";
        return false;
    }
    //遍历获取数组中对象，都是字符串
    QStringList zeroModeList;
    for (const QJsonValue& value : zeroModeArray)
    {
        if (value.isString())
        {
            QString str = value.toString();
            zeroModeList.append(str);
        }
        else
        {
            qDebug() << "zeroMode array contains non-string value";
            return false;
        }
    }
    //将字符串列表转换为逗号分隔的字符串
    QString zeroMode = zeroModeList.join(",");



    // 初始点
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
    // 最大扭矩
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
    // 断裂判定
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

    //是否复位
    if (configForm["specimenReturn"].isNull())
    {
        qDebug() << "specimenReturn error";
        return false;
    }
    int specimenReturn = configForm["specimenReturn"].toInt();

    QString strSql;
    strSql = QString("UPDATE method_config SET \
        specimen_name = '%1',\
        batch_number = '%2',\
        production_date = '%3',\
        operator = '%4',\
        lab_temperature = %5,\
        lab_humidity = %6,\
        mode = '%7',\
        torsion_speed = %8,\
        torsion_unit = '%9',\
        initial_mode = '%10',\
        initial_load_value = %11,\
        unit = '%12',\
        zero_mode = '%13',\
        start_point = %14,\
        end_condition = %15,\
        max_torque = %16,\
        max_angle = %17,\
        break_sensitivity = %18,\
        move_speed = %19,\
        static_mode = '%20',\
        constant_angle = %21,\
        constant_torque = %22,\
        cycle_count = %23,\
        dynamic_mode = '%24',\
        torsion_frequency = %25,\
        step_time = %26,\
        specimen_number = '%27',\
        remarks = '%28',\
        delay_time = %29,\
        direction = '%30'\
        ,specimenReturn = %31\
        ,ad_direction = '%32' \
        ,set_angle = %33\
        where is_current = 1\
        ")
        .arg(specimenName)
        .arg(batchNumber)
        .arg(productionDate)
        .arg(strOperator)
        .arg(labTemperature)
        .arg(labHumidity)
        .arg(mode)
        .arg(torsionSpeed)
        .arg(torsionUnit)
        .arg(initialMode)
        .arg(initialLoadValue)
        .arg(unit)
        .arg(zeroMode)
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
        .arg(remarks)
        .arg(delayTime)
        .arg(direction)
		.arg(specimenReturn)
        .arg(ad_direction)
        .arg(setAngle)
        ;

  
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-modifyData";
    jsonObj["status"] = "success";
    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}

bool MethodHandler::fetchDetail(const QSqlDatabase& db, const QJsonObject& recvObj, QString& response)
{
    QSqlQuery query(db);

    int id = recvObj["key"].toInt();
    QString strSql;
    if (id == 0) {
        strSql = QString("select * from method_config where is_current = 1");
    }
    else {

        strSql = QString("update method_config set is_current = 1 where id = %1").arg(id);
        if (!query.exec(strSql))
        {
            qDebug() << "Failed to fetch data:";
            qDebug() << query.lastError().text();
            return false;
        }

        strSql = QString("update method_config set is_current = 0 where id != %1").arg(id);
        if (!query.exec(strSql))
        {
            qDebug() << "Failed to fetch data:";
            qDebug() << query.lastError().text();
            return false;
        }



        strSql = QString("update system_config set current_method=(select name from method_config where id = %1) where id= %2").arg(id).arg(deviceId_);
        if (!query.exec(strSql))
        {
            qDebug() << "Failed to fetch data:";
            qDebug() << query.lastError().text();
            return false;
        }


        strSql = QString("select * from method_config where id = %1").arg(id);
    }
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

            if (fieldName == "zero_mode") {
				QStringList zeroModeList = fieldValue.toString().split(",");
				QJsonArray zeroModeArray;
				for (const QString& mode : zeroModeList) {
					zeroModeArray.append(mode);
				}
				obj1[fieldName] = zeroModeArray;
				continue;
            }
            else {
                obj1[fieldName] = QJsonValue::fromVariant(fieldValue);
            }
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
		jsonObj["is_current"] = query.value("is_current").toInt();
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
        QJsonDocument jsonDoc(jsonObj);
        response = jsonDoc.toJson();
        return true;
    }


    QString strSql = QString("select count(*) as total from method_config ");
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }
    while (query.next())
    {
        if (query.value("total").toInt() == 1) {
            QJsonObject jsonObj;
            jsonObj["__channel"] = channel_ + "-deleteData";
            jsonObj["status"] = "error";
            jsonObj["message"] = "only one record left,can not delete.";
            QJsonDocument jsonDoc(jsonObj);
            response = jsonDoc.toJson();
            return true;
        }

    }


    QJsonArray array = recvObj["ids"].toArray();
    for (auto it = array.begin(); it != array.end(); it++)
    {
         strSql = QString("delete from method_config where id = %1").arg(it->toInt());
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
    auto type = recvObj["__type"];
    if (type == "fetchSerialPorts")
    {
        return fetchSerialPorts(recvObj, response);
    }

    QSqlDatabase db;
    if (!getConfigDB(db))
        return false;

    if (type == "addData")
    {
        return addData(db, recvObj, response);
    }
	else if (type == "modifyData")
	{
		return modifyData(db, recvObj, response);
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
    else if (type == "updateModbusSerialPort")
    {
        return updateModbusSerialPort(db, recvObj, response);
    }
    else
    {
        return false;
    }
}

bool MethodHandler::fetchSerialPorts(const QJsonObject& recvObj, QString& response)
{
    QJsonArray portsArray;
    QJsonArray desArray;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports)
    {
        portsArray.append(info.portName());
        desArray.append(info.description());
    }

    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-fetchSerialPorts";
    jsonObj["status"] = "success";
    jsonObj["data"] = portsArray;
    jsonObj["description"] = desArray;

    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;
}

bool MethodHandler::updateModbusSerialPort(const QSqlDatabase& db, QJsonObject& recvObj, QString& response)
{
    QString modbusSerialPort = recvObj.contains("modbusSerialPort") ? recvObj["modbusSerialPort"].toString() : "";

    QString strSql = QString("select id from method_config where is_current = 1");
    QSqlQuery query(db);
    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }
    int methodId = 0;
    if (query.next())
    {
        methodId = query.value("id").toInt();
    }

    strSql = QString("UPDATE method_config SET modbus_serial_port = '%1' \
        where id = %2\
        ")
        .arg(modbusSerialPort)
        .arg(methodId)
        ;

    if (!query.exec(strSql))
    {
        qDebug() << "Failed to fetch data:";
        qDebug() << query.lastError().text();
        return false;
    }



    QJsonObject jsonObj;
    jsonObj["__channel"] = channel_ + "-modifyData";
    jsonObj["status"] = "success";
    QJsonDocument jsonDoc(jsonObj);
    response = jsonDoc.toJson();
    return true;

    return true;
}
