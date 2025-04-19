#include "ControlTestCommHandler.h"
#include "DataDefine.h"
#include <qjsonarray.h>
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
    else if (type == "end-test") {
		return stopTest(socket, obj, err);
    }
	else if (type == "transfer-method") {
		return transferMehod(socket, obj, err);
	}
	else if (type == "zero") {
		return zero(socket, obj, err);
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
    double delayTime = 0;

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

        // delayTime
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

    // 通用参数检查
    //initialMode
	if (configForm["initialMode"].isNull())
	{
		qDebug() << "initialMode error";
		return false;
	}
	auto initialMode = configForm["initialMode"].toString();
    //initialLoadValue
	if (configForm["initialLoadValue"].isNull())
	{
		qDebug() << "initialLoadValue error";
		return false;
	}
	double initialLoadValue = configForm["initialLoadValue"].toDouble();
	//unit
	if (configForm["unit"].isNull())
	{
		qDebug() << "unit error";
		return false;
	}
	QString unit = configForm["unit"].toString();
	//zeroMode
     //zeroMode是一个对象数组
    // 取第一个对象
    QJsonArray zeroModeArray = configForm["zeroMode"].toArray();
    if (zeroModeArray.isEmpty())
    {
        qDebug() << "zeroMode array is empty";
        return false;
    }
    //遍历取出所有对象，对象是字符串
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

    //写入通用参数    

    //归零模式
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(0x02, buffer);
	setAddr(0x0ad4, buffer);

	int16_t mi_Add = 0;
    //遍历zeroModeList
    for (int i = 0; i < zeroModeList.length(); i++)
    {
        if (zeroModeList[i] == "torqueZero")
        {
            //mi_Add的第4个bit写入1
            mi_Add |= 0x10;
        }
        else if (zeroModeList[i] == "angleZero")
        {
            //mi_Add的第1、2个bit写入1
            mi_Add |= 0x03;
        }
        else if (zeroModeList[i] == "angleZeroAndTorqueZero") {
            //mi_Add的第3个bit写入1

            mi_Add |= 0x08;
        }
    }

    buffer[12] = int8_t((mi_Add & 0x000000FF));
    buffer[13] = int8_t((mi_Add & 0x0000FF00) >> 8);
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
   
	//写入初始负载值
	if (!writeFloat(socket, 0x0a2c, initialLoadValue, err))
		return false;


    //写入起始点
	if (!writeFloat(socket, 0x0A44, startPoint, err))
		return false;

    //写入结束点
	if (!writeFloat(socket, 0x0A48, endCondition, err))
		return false;

	//写入最大扭力
	if (!writeFloat(socket, 0x0A60, maxTorque, err))
		return false;
	//写入最大角度
	if (!writeFloat(socket, 0x0A64, maxAngle, err))
		return false;
	//写入断裂敏感度
	if (!writeFloat(socket, 0x0A30, breakSensitivity, err))
		return false;
    //写入定速
	if (!writeInt16(socket, 0x1402, 0x0000, err))
		return false;
    //写入移动速度
    if (!writeFloat(socket, 0x1108, moveSpeed, err))
        return false;


    int16_t mi_PC_TEST_2 = 0;
    //写入单位
    if (unit == "N") {
        //第5个bit置为0
        mi_PC_TEST_2 &= 0xDF;
    }
    else if (unit == "mm") {
        //第5个bit写入1
        mi_PC_TEST_2 |= 0x20;
    }
    else {
        err = "unit error";
        return false;
    }
    if (!writeInt16(socket, 0x0A7A, mi_PC_TEST_2, err))
        return false;

    //指定缓冲区的记录来源
	int16_t REC_SOURCE = 0x0000;

	REC_SOURCE |= 0x0F;
    if (!writeInt16(socket, 0x0b42, REC_SOURCE, err)) {
		err = "writeInt16 error " + socket->socketError();
		return false;
    }
	//写入采样频率
	int32_t mi_SampleRate = 1;//5K的采样率
    if (!writeInt32(socket, 0x080c, mi_SampleRate, err)) {
        err = "writeInt32 error " + socket->socketError();
        return false;
    }



 //   buffer.clear();
	//packPC_KEY(0x14, buffer);
	//if (!socket->writeData(buffer)) {
	//	err = "writeData error " + socket->socketError();
	//	return false;
	//}
	//recvData.clear();
	//if (!socket->readData(recvData)) {
	//	err = "readData error " + socket->socketError();
	//	return false;
	//}
	//if (!checkSum(recvData)) {
	//	err = "readData error ,checksum error";
	//	return false;
	//}


	//设置测试后的动作
	int16_t PC_TEST_3 = 0;
	//第4个bit置为1
	PC_TEST_3 |= 0x10;
	if (!writeInt16(socket, 0x0A7C, PC_TEST_3, err)) {
		err = "writeInt16 error " + socket->socketError();
		return false;
	}

    if (mode == "static")
    {
        //写入力量上限
		if (!writeFloat(socket, 0x0A58, (float)maxTorque, err))
			return false;

        //写入角度上限
		if (!writeFloat(socket, 0x0A5C, (float)maxAngle, err))
			return false;
    }

    QVector< DF_SET>  dfSets;
    // TODO
    // 把方法设定传至DF SET方法设定中
    if (mode == "static") {
        DF_SET df_set;

		if (staticMode == "torque")
		{
			df_set.IR_TYPE = 2;
		}
		else if (staticMode == "angle")
		{
			df_set.IR_TYPE = 1;
		}
		else
		{
			err = "staticMode error";
			return false;
		}
        df_set.DF_SIGNAL = 4;

		if (torsionUnit == "degree_per_min")
		{
			df_set.DF_SP_UNIT = 2;
		}
		else if (torsionUnit == "n_per_min")
		{
			df_set.DF_SP_UNIT = 1;
		}
		else
		{
			err = "unit error";
			return false;
		}

		df_set.DF_IR = constantAngle;
		df_set.DF_HZ = torsionSpeed;

        //写入移动速度
        if (!writeFloat(socket, 0x1408, torsionSpeed, err))
            return false;

        //第一个步骤
		dfSets.push_back(df_set);

        //加入延迟
        DF_SET df_set_delay;
        df_set_delay.IR_TYPE = 3;
        df_set_delay.DF_END_TIME = delayTime;
        dfSets.push_back(df_set_delay);


		//第二个步骤
		DF_SET df_set_second;
        df_set_second = df_set;
        df_set_second.DF_IR = -constantAngle;        
        dfSets.push_back(df_set_second);

        dfSets.push_back(df_set_delay);


        DF_SET df_set_loop;
        df_set_loop.IR_TYPE = 4;
        df_set_loop.DF_END_CYCLE = cycleCount;
        df_set_loop.JUMP_NO = 0;
        dfSets.push_back(df_set_loop);
    }
    else if (mode == "dynamic")
    {
        if (dynamicMode == "sin") {
            DF_SET df_set;

            df_set.IR_TYPE = 1;
            df_set.DF_SIGNAL = 4;
            df_set.DF_SP_UNIT = 2;

            df_set.DF_IR = 0;
            df_set.DF_HZ = 30;

            
            //如果是第一个步骤，先位移0mm
            dfSets.push_back(df_set);
        }


        DF_SET df_set;

        if (staticMode == "torque")
        {
            df_set.IR_TYPE = 2;
        }
        else if (staticMode == "angle")
        {
            df_set.IR_TYPE = 1;
        }
        else
        {
            err = "staticMode error";
            return false;
        }

		if (dynamicMode == "triangle")
		{
            df_set.DF_SIGNAL = 1;
		}
		else if (dynamicMode == "sin")
		{
            df_set.DF_SIGNAL = 0;
		}

        df_set.DF_SP_UNIT = 2;

        df_set.DF_IR = constantAngle;
        df_set.DF_HZ = torsionFrequency;
        df_set.DF_END_CYCLE = cycleCount;
        df_set.JUMP_NO = 0;

        //第一个步骤
        dfSets.push_back(df_set);

        
    }
    /*
    *STEP#2
    * 遍历方法的每组设定信息
    * 最后一组信息需要设置为0，告知下位机方法设定信息结束
    */
    for (int i = 0; i <= dfSets.length(); i++) {
        if (i != dfSets.length()) {
			dfSets[i].mui_GroupNo = i;
            if (!perSetDfSet(socket, dfSets[i], err))
                return false;
            continue;
        }
        //写完最后一个DF SET，写入结束数据
        DF_SET tmp;
        tmp.mui_GroupNo = i;
        //tmp.IR_TYPE = 0;
        if (mode == "destructive") {

            tmp.IR_TYPE = 0;
        }
        else {
            tmp.IR_TYPE = 99;

        }
        if (!perSetDfSet(socket, tmp, err))
            return false;
    }

    /*
    文档解释:將 PC_ADD 指定的 動態組別 整組 搬到 及時顯示區  	DF_SET[PC_ADD]  0X09B0
    旧版代码中有此命令,搬运此段逻辑
    */

	buffer.clear();
    packPC_KEY(0x3a, 0, buffer);//这里，就是一直下发不了温度的原因
    if (!socket->writeData(buffer)) {
        err = "writeData error " + socket->socketError();
        return false;
    }
    recvData.clear();
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



    buffer[6 + mi_Add] = int8_t((df_set.ZERO_DEVICE & 0x00FF));
    buffer[7 + mi_Add] = int8_t((df_set.ZERO_DEVICE & 0xFF00) >> 8);

    buffer[8 + mi_Add] = int8_t((df_set.IR_TYPE & 0x00FF));
    buffer[9 + mi_Add] = int8_t((df_set.IR_TYPE & 0xFF00) >> 8);

    buffer[10 + mi_Add] = int8_t((df_set.DF_SIGNAL & 0x00FF));
    buffer[11 + mi_Add] = int8_t((df_set.DF_SIGNAL & 0xFF00) >> 8);
   

    if (isLitteEndian()) {
        char* ptr = reinterpret_cast<char*>(&df_set.DF_HZ);
        buffer[12 + mi_Add] = ptr[0];
        buffer[13 + mi_Add] = ptr[1];
        buffer[14 + mi_Add] = ptr[2];
        buffer[15 + mi_Add] = ptr[3];
    }
    else {
        char* ptr = reinterpret_cast<char*>(&df_set.DF_HZ);
        buffer[12 + mi_Add] = ptr[3];
        buffer[13 + mi_Add] = ptr[2];
        buffer[14 + mi_Add] = ptr[1];
        buffer[15 + mi_Add] = ptr[0];
    }


    if (isLitteEndian()) {
        char* ptr = reinterpret_cast<char*>(&df_set.DF_IR);
        buffer[16 + mi_Add] = ptr[0];
        buffer[17 + mi_Add] = ptr[1];
        buffer[18 + mi_Add] = ptr[2];
        buffer[19 + mi_Add] = ptr[3];
    }
    else {
        char* ptr = reinterpret_cast<char*>(&df_set.DF_IR);
        buffer[16 + mi_Add] = ptr[3];
        buffer[17 + mi_Add] = ptr[2];
        buffer[18 + mi_Add] = ptr[1];
        buffer[19 + mi_Add] = ptr[0];
    }
  

    buffer[20 + mi_Add] = int8_t((df_set.DF_END_CYCLE & 0x00FF));
    buffer[21 + mi_Add] = int8_t((df_set.DF_END_CYCLE & 0xFF00) >> 8);
    buffer[22 + mi_Add] = int8_t((df_set.DF_END_CYCLE & 0xFF0000) >> 16);
    buffer[23 + mi_Add] = int8_t((df_set.DF_END_CYCLE& 0xFF000000) >> 24);

    buffer[24 + mi_Add] = int8_t((df_set.DF_END_TIME & 0x00FF));
    buffer[25 + mi_Add] = int8_t((df_set.DF_END_TIME & 0xFF00) >> 8);
    buffer[26 + mi_Add] = int8_t((df_set.DF_END_TIME & 0xFF0000) >> 16);
    buffer[27 + mi_Add] = int8_t((df_set.DF_END_TIME & 0xFF000000) >> 24);

    buffer[28 + mi_Add] = int8_t((df_set.JUMP_NO & 0x00FF));
    buffer[29 + mi_Add] = int8_t((df_set.JUMP_NO & 0xFF00) >> 8);

    buffer[30 + mi_Add] = int8_t((df_set.DF_SP_UNIT & 0x00FF));
    buffer[31 + mi_Add] = int8_t((df_set.DF_SP_UNIT & 0xFF00) >> 8);

    buffer[32 + mi_Add] = int8_t((df_set.MEM_NO & 0x00FF));
    buffer[33 + mi_Add] = int8_t((df_set.MEM_NO & 0xFF00) >> 8);

    buffer[34 + mi_Add] = int8_t((0 & 0x00FF));
    buffer[35 + mi_Add] = int8_t((0& 0xFF00) >> 8);

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


bool ControlTestCommHandler::stopTest(CommunicationThread* socket, QJsonObject& obj, QString& err) {


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


bool ControlTestCommHandler::zero(CommunicationThread* socket, QJsonObject& obj, QString& err) {

    QByteArray buffer;

    buffer.clear();
    packPC_KEY(0x06,0x1E, buffer);
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



    if (!writeInt32(socket, 0X092E, 0X0800, err)) {
        return false;
    }
    return true;

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
	packPC_KEY(0x21, buffer);
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

	writeInt32(socket, 0X1102, 0, err);
    {
        QByteArray buffer;
        buffer.fill(0x00, 528);
        setCmd(E_Mode::Write, buffer);
        setLength(0x04, buffer);
        setAddr(0x1108, buffer);


        float speed = 10;

        if (isLitteEndian()) {
            char* ptr = reinterpret_cast<char*>(&speed);
            buffer[12] = ptr[0];
            buffer[13] = ptr[1];
            buffer[14] = ptr[2];
            buffer[15] = ptr[3];
        }
        else {
            char* ptr = reinterpret_cast<char*>(&speed);
            buffer[12] = ptr[3];
            buffer[13] = ptr[2];
            buffer[14] = ptr[1];
            buffer[15] = ptr[0];
        }

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
//        return true;
    }

    {
        QByteArray buffer;
        buffer.fill(0x00, 528);
        setCmd(E_Mode::Read, buffer);
        setLength(0x04, buffer);
        setAddr(0x1108, buffer);
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

        /**
         * 判断数据正确性
         *
         */
        if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0x04) {
            err = "readData error ,data error";
            return false;
        }

        char beginAddr = 0x01;
        int temp = (recvData[12 + beginAddr] & 0xff) +
            (recvData[12 + beginAddr+1] & 0xff) * 0x100 +
            (recvData[12 + beginAddr+2] & 0xff) * 0x10000 +
            (recvData[12 + beginAddr+3] & 0xff) * 0x1000000;
		qDebug() << "spin temp" << temp;
		if (temp == 0) {
			err = "spin error ,data error";
			return false;
		}
    }
  



	QByteArray buffer;
	packPC_KEY(0x22, buffer);
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
	packPC_KEY(0x23, buffer);
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
