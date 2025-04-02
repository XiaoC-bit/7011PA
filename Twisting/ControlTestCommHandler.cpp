#include "ControlTestCommHandler.h"
#include "DataDefine.h"
ControlTestCommHandler::ControlTestCommHandler(ReadU65Struct& ref, QObject* parent)
	: CommHandler(ref,parent)
{}

ControlTestCommHandler::~ControlTestCommHandler()
{
}

bool ControlTestCommHandler::commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err) {

	QString type = obj["__type"].toString();
	if (type == "home") {
		return home(socket, obj, err);
	}
	else if (type == "stop") {
		return stop(socket, obj, err);
	}
	else if (type == "spin") {
		return spin(socket, obj, err);
	}
	else if (type == "re-spin") {
		return reSpin(socket, obj, err);
	}
	else if (type == "start-test") {
		return startTest(socket, obj, err);
	}
	else if (type == "transfer-method") {
		return transferMehod(socket, obj, err);
	}
	return false;
}



bool ControlTestCommHandler::transferMehod(CommunicationThread* socket, QJsonObject& recvObj, QString& err) {

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

    QVector< DF_SET>  dfSets;
    DF_SET df_set;
    // TODO
    // 把方法设定传至DF SET方法设定中

    /*
    *STEP#2
    * 遍历方法的每组设定信息
    * 最后一组信息需要设置为0，告知下位机方法设定信息结束
    */
    for (int i = 0; i <= dfSets.length(); i++) {
        if (i != dfSets.length()) {
            if (!perSetDfSet(socket, dfSets[i], err))
                return false;
            continue;
        }
        //写完最后一个DF SET，写入结束数据
        DF_SET tmp;
        tmp.mui_TEST_MODE = 0;
        tmp.mui_IR_TEMP = 0;
        tmp.mi_TEST_EndTime = 0;
        tmp.mui_GroupNo = i;
        if (!perSetDfSet(socket, tmp, err))
            return false;
    }

    /*
    文档解释:將 PC_ADD 指定的 動態組別 整組 搬到 及時顯示區  	DF_SET[PC_ADD]  0X09B0
    旧版代码中有此命令,搬运此段逻辑
    */

    QByteArray buffer;
    packPC_KEY(0x3a, 0, buffer);//这里，就是一直下发不了温度的原因
    if (!socket->writeData(buffer)) {
        err = "writeData error " + socket->socketError();
        return false;
    }
    QByteArray recvData;
    if (!socket->readData(recvData)) {
        err = "readData error " + socket->socketError();
        return false;
    }
    if (!checkSum(recvData)) {
        err = "readData error ,checksum error";
        return false;
    }


    QJsonObject responseObj;
    responseObj["__channel"] = recvObj["__channel"];
    responseObj["__type"] = recvObj["__type"];
    recvObj = responseObj;




    return true;
}



bool ControlTestCommHandler::perSetDfSet(CommunicationThread* socket, DF_SET& df_set, QString& err) {
    QByteArray buffer;
    buffer.fill(0x00, 528);
    setCmd(E_Mode::Write, buffer);
    setLength(0x20, buffer);
    setAddr(0x4000 + 0x20 * df_set.mui_GroupNo, buffer);

    int mi_Add = 8;
    buffer[4 + mi_Add] = int8_t((df_set.mui_GroupNo & 0x00FF));
    buffer[5 + mi_Add] = int8_t((df_set.mui_GroupNo & 0xFF00) >> 8);

    buffer[6 + mi_Add] = int8_t((df_set.mui_DISCARD_SAMPLE & 0x00FF));
    buffer[7 + mi_Add] = int8_t((df_set.mui_DISCARD_SAMPLE & 0xFF00) >> 8);

    buffer[8 + mi_Add] = int8_t((df_set.mui_TEST_MODE & 0x00FF));
    buffer[9 + mi_Add] = int8_t((df_set.mui_TEST_MODE & 0xFF00) >> 8);

    buffer[10 + mi_Add] = int8_t((df_set.mui_SAMPLE & 0x00FF));
    buffer[11 + mi_Add] = int8_t((df_set.mui_SAMPLE & 0xFF00) >> 8);


    if (isLitteEndian()) {
        char* ptr = reinterpret_cast<char*>(&df_set.md_IR_CPM);
        buffer[12 + mi_Add] = ptr[0];
        buffer[13 + mi_Add] = ptr[1];
        buffer[14 + mi_Add] = ptr[2];
        buffer[15 + mi_Add] = ptr[3];
    }
    else {
        char* ptr = reinterpret_cast<char*>(&df_set.md_IR_CPM);
        buffer[12 + mi_Add] = ptr[3];
        buffer[13 + mi_Add] = ptr[2];
        buffer[14 + mi_Add] = ptr[1];
        buffer[15 + mi_Add] = ptr[0];
    }


    if (isLitteEndian()) {
        char* ptr = reinterpret_cast<char*>(&df_set.md_IR_ANG);
        buffer[16 + mi_Add] = ptr[0];
        buffer[17 + mi_Add] = ptr[1];
        buffer[18 + mi_Add] = ptr[2];
        buffer[19 + mi_Add] = ptr[3];
    }
    else {
        char* ptr = reinterpret_cast<char*>(&df_set.md_IR_ANG);
        buffer[16 + mi_Add] = ptr[3];
        buffer[17 + mi_Add] = ptr[2];
        buffer[18 + mi_Add] = ptr[1];
        buffer[19 + mi_Add] = ptr[0];
    }

    int mi_tempmin, mi_tempHour;
    long time;
    mi_tempmin = (df_set.mi_CONTROL_TIME / 60) % 60;
    mi_tempHour = df_set.mi_CONTROL_TIME / 3600;
    time = (df_set.mi_CONTROL_TIME % 60) * 1000 + mi_tempmin * 0x010000 + mi_tempHour * 0x01000000;
    buffer[20 + mi_Add] = int8_t((time & 0x00FF));
    buffer[21 + mi_Add] = int8_t((time & 0xFF00) >> 8);
    buffer[22 + mi_Add] = int8_t((time & 0xFF0000) >> 16);
    buffer[23 + mi_Add] = int8_t((time & 0xFF000000) >> 24);

    mi_tempmin = (df_set.mi_TEST_EndTime / 60) % 60;
    mi_tempHour = df_set.mi_TEST_EndTime / 3600;
    time = (df_set.mi_TEST_EndTime % 60) * 1000 + mi_tempmin * 0x010000 + mi_tempHour * 0x01000000;

    buffer[24 + mi_Add] = int8_t((time & 0x00FF));
    buffer[25 + mi_Add] = int8_t((time & 0xFF00) >> 8);
    buffer[26 + mi_Add] = int8_t((time & 0xFF0000) >> 16);
    buffer[27 + mi_Add] = int8_t((time & 0xFF000000) >> 24);

    buffer[28 + mi_Add] = int8_t((df_set.mui_IR_TEMP & 0x00FF));
    buffer[29 + mi_Add] = int8_t((df_set.mui_IR_TEMP & 0xFF00) >> 8);

    buffer[30 + mi_Add] = int8_t((df_set.mui_TEMP_TOLERANCE & 0x00FF));
    buffer[31 + mi_Add] = int8_t((df_set.mui_TEMP_TOLERANCE & 0xFF00) >> 8);

    buffer[32 + mi_Add] = int8_t((df_set.mui_MDR_FLAG & 0x00FF));
    buffer[33 + mi_Add] = int8_t((df_set.mui_MDR_FLAG & 0xFF00) >> 8);

    buffer[34 + mi_Add] = int8_t((df_set.mui_STABLE_TIME & 0x00FF));
    buffer[35 + mi_Add] = int8_t((df_set.mui_STABLE_TIME & 0xFF00) >> 8);

    calcSum(buffer);
    if (!socket->writeData(buffer)) {
        err = "writeData error " + socket->socketError();
        return false;
    }
    QByteArray recvData;
    if (!socket->readData(recvData)) {
        err = "readData error " + socket->socketError();
        return false;
    }
    if (!checkSum(recvData)) {
        err = "readData error ,checksum error";
        return false;
    }
    return true;
}


bool ControlTestCommHandler::startTest(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x05, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}

bool ControlTestCommHandler::home(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x30, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}


bool ControlTestCommHandler::stop(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x04, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}

bool ControlTestCommHandler::spin(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x1c, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}

bool ControlTestCommHandler::reSpin(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x1d, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}
