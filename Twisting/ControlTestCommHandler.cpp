#include "ControlTestCommHandler.h"
#include "DataDefine.h"
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <QThread>
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
	else if (type == "move-up") {
		return spin(socket, obj, err);
	}
	else if (type == "move-down") {
		return reSpin(socket, obj, err);
	}
	else if (type == "start-test") {
		return startTest(socket, obj, err);
	}
	else if (type == "prepare-test") {
		return prepareTest(socket, obj, err);
	}
    else if (type == "end-test") {
		return stopTest(socket, obj, err);
    }
	else if (type == "transfer-method") {
        bool ret = false;
        for (int i = 0; i < 10; i++) {
            ret = transferMehod(socket, obj, err);
            qDebug() << "transfer-method ret:" << ret << err;
            if (ret) {
                break;
            }
        }

        qDebug() << "final transfer-method ret:" << ret << err;
        if (!ret) {
            QJsonObject responseObj;
            responseObj["__channel"] = obj["__channel"];
            responseObj["__type"] = obj["__type"];
			responseObj["status"] = "error";
            obj = responseObj;
            return true;
        }

        //写入PC KEY 0B，保存参数
		QByteArray buffer;
		buffer.fill(0x00, 528);
		packPC_KEY(0x0B, buffer);
		if (!socket->writeData(buffer)) {
			err = "writeData error " + socket->socketError();
			return false;
		}

        return ret;
	}
	else if (type == "zero") {
		return zero(socket, obj, err);
	}
    else if (type == "setYZDIR") {
        return setYZDIR(socket, obj, err);
    }
    else if (type == "setXDIR") {
        return setXDIR(socket, obj, err);
    }
    else if (type == "setAD1_DIR") {
        return setAD1_DIR(socket, obj, err);
    }
    else if (type == "setAD2_DIR") {
        return setAD2_DIR(socket, obj, err);
    }
	else if (type == "setAD1_UPDN") {
		return setAD1_UPDN(socket, obj, err);
	}
	else if (type == "setAD2_UPDN") {
		return setAD2_UPDN(socket, obj, err);
	}
    else if (type == "setXGAIN") {
        return setXGAIN(socket, obj, err);
    }
    else if (type == "setYZGAIN") {
        return setYZGAIN(socket, obj, err);
    }
    else if (type == "set-sampling-rate") {
		return setSamplingRate(socket, obj, err);
    }
	else if (type == "setADGAIN") {
		return setADGAIN(socket, obj, err);
	}
    else if (type == "setADCAP") {
		return setADCAP(socket, obj, err);
    }
	return false;
}


bool ControlTestCommHandler::setSamplingRate(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	int samplingRate = obj["samplingRate"].toInt();

    QByteArray buffer;
    buffer.fill(0x00, 528);
    packPC_KEY(0x9A, samplingRate, buffer);
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
    //把configForm转成字符串
	qDebug() << "configForm:" << QJsonDocument(configForm).toJson(QJsonDocument::Indented);
	qDebug() << "testModeConfig:" << QJsonDocument(testModeConfig).toJson(QJsonDocument::Indented);



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


    if (testModeConfig["direction"].isNull())
    {
        qDebug() << "direction error";
        return false;
    }
    QString direction = configForm["direction"].toString();

	if (testModeConfig["adDirection"].isNull())
	{
		qDebug() << "adDirection error";
		return false;
	}
	QString adDirection = configForm["adDirection"].toString();

	//是否回位
	if (configForm["specimenReturn"].isNull())
	{
		qDebug() << "specimenReturn error";
		return false;
	}
	int specimenReturn = configForm["specimenReturn"].toInt();

    //写入通用参数    

    //归零模式
	/*QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(0x02, buffer);
	setAddr(0x0ad4, buffer);

	
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
    }*/

    int16_t SYNC_ZERO_FLAG = 0;
	if (!readInt16(socket, 0x0a1a, SYNC_ZERO_FLAG, err))
	{
		err = "readInt16 error " + socket->socketError();
		return false;
	}

    int16_t mi_Add = 0;
    int zeroType = 0;
    //遍历zeroModeList
    for (int i = 0; i < zeroModeList.length(); i++)
    {
        if (zeroModeList[i] == "torqueZero")
        {
            //mi_Add的第4个bit写入1
            mi_Add |= 0x10;
            zeroType += 1;

            //将SYNC_ZERO_FLAG的BIT 2 写入 1
			SYNC_ZERO_FLAG |= 0x04;


        }
        else if (zeroModeList[i] == "angleZero")
        {
            //mi_Add的第1、2个bit写入1
            mi_Add |= 0x03;
            zeroType += 2;

			//将SYNC_ZERO_FLAG的BIT 4 写入 1
			SYNC_ZERO_FLAG |= 0x10;
        }
        else if (zeroModeList[i] == "angleZeroAndTorqueZero") {
            //mi_Add的第3个bit写入1

            mi_Add |= 0x08;
            zeroType += 3;


            SYNC_ZERO_FLAG |= 0x10;
            SYNC_ZERO_FLAG |= 0x04;
        }
    }

    if (!writeInt16(socket, 0x0a1a, SYNC_ZERO_FLAG, err))
    {
        err = "readInt16 error " + socket->socketError();
        return false;
    }

    if(zeroType > 0)
    {
        //ABS归零
        QByteArray buffer;

        buffer.clear();
        packPC_KEY(0x06, 0x1F, buffer);
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
    }
	int16_t mi_PC_TEST_3 = 0;
    if (!readInt16(socket, 0x0A7C, mi_PC_TEST_3, err)) {
		err = "readInt16 error " + socket->socketError();
		return false;
    }
    // 设置回位模式（0~5），你可以换成其他值如 2 = 暫停後回位，3 = 回極限 等
    uint8_t returnMode = 0;  // 預設不回位
    if (specimenReturn == 1) {
        returnMode = 1;  // 立刻回位
    }

	qDebug() << "0A7C写入前" << mi_PC_TEST_3;
    // 清除 Bit4~7，再設定
    mi_PC_TEST_3 = (mi_PC_TEST_3 & ~0xF0) | ((returnMode & 0x0F) << 4);

    //第4个bit置为1
    //mi_PC_TEST_3 |= 0x10;

	qDebug() << "0A7C写入后" << mi_PC_TEST_3;
    //写入回位
    if (!writeInt16(socket, 0x0A7C, mi_PC_TEST_3, err)) {
		err = "writeInt16 error " + socket->socketError();
		return false;
    }


    ////设置测试后的动作
    //int16_t PC_TEST_3 = 0;
    ////第4个bit置为1
    //PC_TEST_3 |= 0x10;
    //if (!writeInt16(socket, 0x0A7C, PC_TEST_3, err)) {
    //    err = "writeInt16 error " + socket->socketError();
    //    return false;
    //}



	if (!writeInt16(socket, 0x0ad4, mi_Add, err))
	{
		err = "writeInt16 error " + socket->socketError();
		return false;
	}
   
	//写入初始负载值
	if (!writeFloat(socket, 0x0a2c, initialLoadValue, err))
		return false;

    qDebug() << "startPoint" << startPoint;
    //写入起始点
	if (!writeFloat(socket, 0x0A44, startPoint, err))
		return false;

    qDebug() << "startPoint" << endCondition;
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


    for (int i = 0; i < zeroModeList.length(); i++)
    {
        if (zeroModeList[i] == "torqueZero")
        {
            //第0个bit写入1
            mi_PC_TEST_2 |= 0x03;
        }
        else if (zeroModeList[i] == "angleZero")
        {
            //第1个bit写入1
            mi_PC_TEST_2 |= 0x04;
        }
        else if (zeroModeList[i] == "angleZeroAndTorqueZero") {
            //第1 2个bit写入1

            mi_PC_TEST_2 |= 0x07;
        }
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
	//int32_t mi_SampleRate = 1;//5K的采样率
 //   if (!writeInt32(socket, 0x080c, mi_SampleRate, err)) {
 //       err = "writeInt32 error " + socket->socketError();
 //       return false;
 //   }



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



    QVector< DF_SET>  dfSets;
    // TODO
    // 把方法设定传至DF SET方法设定中
    if (mode == "destructive") {
        //写入移动速度
        if (!writeFloat(socket, 0x1408, torsionSpeed, err))
            return false;

        //写入力量上限
        if (!writeFloat(socket, 0x0A58, (float)maxTorque, err))
            return false;

        //写入角度上限
        if (!writeFloat(socket, 0x0A5C, (float)maxAngle, err))
            return false;

        //写入移动速度
        if (!writeFloat(socket, 0x1108, moveSpeed, err))
            return false;

        //不需要写入0A10
        if (0) {
            if (direction == "Forward") {
                int16_t value = 0;
                if (!readInt16(socket, 0x0a10, value, err)) {
                    return false;
                }
                //将BIT 0 设置为0
                value &= ~(1 << 0);
                //将BIT 9 设置为1
                value |= (1 << 9);
                //将BIT 10设置为0
                value &= ~(1 << 10);

                if (!writeInt16(socket, 0x0a10, value, err))
                    return false;
                if (!readInt16(socket, 0x0a10, value, err)) {
                    return false;
                }
                //回读检查一遍
                //检查BIT 0是否为0
                if ((value & (1 << 0)) != 0) {
                    err = "BIT 0 is not set";
                    return false;
                }
                //检查BIT 9是否为1
                if ((value & (1 << 9)) == 0) {
                    err = "BIT 9 is not set";
                    return false;
                }
                //检查BIT 10是否为0
                if ((value & (1 << 10)) != 0) {
                    err = "BIT 10 is not set";
                    return false;
                }


                if (0) {
                    //写0A16
                    value = 0;
                    if (!readInt16(socket, 0x0a16, value, err)) {
                        return false;
                    }
                    //将BIT 1 设置为1
                    value |= (1 << 1);

                    if (!writeInt16(socket, 0x0a16, value, err))
                        return false;

                    //检查回读一遍
                    if (!readInt16(socket, 0x0a16, value, err)) {
                        return false;
                    }
                    //检查BIT 1是否为1
                    if ((value & (1 << 1)) == 0) {
                        err = "BIT 1 is not set";
                        return false;
                    }
                }

            }
            else if (direction == "Backward") {
                int16_t value = 0;
                if (!readInt16(socket, 0x0a10, value, err)) {
                    return false;
                }

                //将BIT 0 设置为1
                value |= (1 << 0);
                //将BIT 9 设置为1
                value |= (1 << 9);
                //将BIT 10设置为1
                value |= (1 << 10);

                if (!writeInt16(socket, 0x0a10, value, err))
                    return false;

                //回读确认一遍
                if (!readInt16(socket, 0x0a10, value, err)) {
                    return false;
                }
                //检查BIT 0是否为1
                if ((value & (1 << 0)) == 0) {
                    err = "BIT 0 is not set";
                    return false;
                }
                //检查BIT 9是否为1
                if ((value & (1 << 9)) == 0) {
                    err = "BIT 9 is not set";
                    return false;
                }
                //检查BIT 10是否为1
                if ((value & (1 << 10)) == 0) {
                    err = "BIT 10 is not set";
                    return false;
                }

                if (0) {
                    //写0A16
                    value = 0;
                    if (!readInt16(socket, 0x0a16, value, err)) {
                        return false;
                    }
                    //将BIT 1 取反
                    value ^= (1 << 1);
                    //将BIT 9 取反
                    value ^= (1 << 9);
                    ////将BIT 1 设置为0
                    //value &= ~(1 << 1);

                    if (!writeInt16(socket, 0x0a16, value, err))
                        return false;

                    //检查回读一遍
                    if (!readInt16(socket, 0x0a16, value, err)) {
                        return false;
                    }
                    ////检查BIT 1是否为0
           //         if ((value & (1 << 1)) != 0) {
           //             err = "BIT 1 is not set";
           //             return false;
           //         }
                }

            }
        }
      

        //写0A16
        int16_t value = 0;
        if (!readInt16(socket, 0x0a16, value, err)) {
            return false;
        }
        if (adDirection == "Forward") {
			//将BIT 1 设置为1
			value |= (1 << 1);
		}
        else  if (adDirection == "Backward") {
			//将BIT 1 设置为0
			value &= ~(1 << 1);
        }
        if (direction == "Forward") {
            //将BIT 9 设置为1
            value |= (1 << 9);
        }
		else if (direction == "Backward") {
            //将BIT 9 设置为0
			value &= ~(1 << 9);
		}
		if (!writeInt16(socket, 0x0a16, value, err))
			return false;
    }
    else if (mode == "static") {

        //写入力量上限
        if (!writeFloat(socket, 0x0A58, (float)maxTorque, err))
            return false;

        //写入角度上限
        if (!writeFloat(socket, 0x0A5C, (float)maxAngle, err))
            return false;


        //写入移动速度
        if (!writeFloat(socket, 0x1C08, torsionSpeed, err))
            return false;
        //归零
        {
            DF_SET df_set;
            if (staticMode == "torque")
            {
                df_set.IR_TYPE = 2;
            }
            else if (staticMode == "angle")
            {
                df_set.IR_TYPE = 1;
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

            df_set.DF_IR = 0;
            df_set.DF_HZ = 10;


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

        if (staticMode == "torque")
        {
            df_set.DF_IR = constantTorque;
        }
        else if (staticMode == "angle")
        {
            df_set.DF_IR = constantAngle;
        }

        df_set.DF_HZ = torsionSpeed;
        //第一个步骤
		dfSets.push_back(df_set);

        //加入延迟
        if (delayTime != 0) {
            DF_SET df_set_delay;
            df_set_delay.IR_TYPE = 3;
            df_set_delay.DF_END_TIME = delayTime;
            dfSets.push_back(df_set_delay);
        }
       
        //归零
        {
            DF_SET df_set;
            if (staticMode == "torque")
            {
                df_set.IR_TYPE = 2;
            }
            else if (staticMode == "angle")
            {
                df_set.IR_TYPE = 1;
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

            df_set.DF_IR = 0;
            df_set.DF_HZ = torsionSpeed;


            //如果是第一个步骤，先位移0mm
            dfSets.push_back(df_set);
        }


        //加入延迟
        if (delayTime != 0) {
            DF_SET df_set_delay;
            df_set_delay.IR_TYPE = 3;
            df_set_delay.DF_END_TIME = delayTime;
            dfSets.push_back(df_set_delay);
        }

		//第二个步骤
		DF_SET df_set_second;
        df_set_second = df_set;

        if (staticMode == "torque")
        {
            df_set_second.DF_IR = -constantTorque;
        }
        else if (staticMode == "angle")
        {
            df_set_second.DF_IR = -constantAngle;
        }  
        dfSets.push_back(df_set_second);

        //延时
        if (delayTime != 0) {
            DF_SET df_set_delay;
            df_set_delay.IR_TYPE = 3;
            df_set_delay.DF_END_TIME = delayTime;
            dfSets.push_back(df_set_delay);
        }

        //归零
        {
            DF_SET df_set;
            if (staticMode == "torque")
            {
                df_set.IR_TYPE = 2;
            }
            else if (staticMode == "angle")
            {
                df_set.IR_TYPE = 1;
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

            df_set.DF_IR = 0;
            df_set.DF_HZ = torsionSpeed;


            //如果是第一个步骤，先位移0mm
            dfSets.push_back(df_set);
        }


        //加入延迟
        if (delayTime != 0) {
            DF_SET df_set_delay;
            df_set_delay.IR_TYPE = 3;
            df_set_delay.DF_END_TIME = delayTime;
            dfSets.push_back(df_set_delay);
        }

        DF_SET df_set_loop;
        df_set_loop.IR_TYPE = 4;
        df_set_loop.DF_END_CYCLE = cycleCount;
        df_set_loop.JUMP_NO = 1;
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


        if (staticMode == "torque")
        {
            df_set.DF_IR = constantTorque;
        }
        else if (staticMode == "angle")
        {
            df_set.DF_IR = constantAngle;
        }

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
    QByteArray buffer;
    QByteArray recvData;
	buffer.fill(0x00, 528);
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
	responseObj["status"] = "success";
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


bool ControlTestCommHandler::setAD1_DIR(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    bool onOrOff = obj["on"].toBool();


    int16_t value = 0;
    if (!readInt16(socket, 0x0a16, value, err)) {
        return false;
    }
    //将BIT 0 设置为1
    int bitNumber = 0;
    if (onOrOff) {
        value |= (1 << bitNumber);
    }
    else {
        value &= ~(1 << bitNumber);
    }

    if (!writeInt16(socket, 0x0a16, value, err))
        return false;

    return true;
}
bool ControlTestCommHandler::setAD2_DIR(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    bool onOrOff = obj["on"].toBool();


    int16_t value = 0;
    if (!readInt16(socket, 0x0a16, value, err)) {
        return false;
    }
    //将BIT 1 设置为1
    int bitNumber = 1;
    if (onOrOff) {
        value |= (1 << bitNumber);
    }
    else {
        value &= ~(1 << bitNumber);
    }

    if (!writeInt16(socket, 0x0a16, value, err))
        return false;

    return true;
}
bool ControlTestCommHandler::setAD1_UPDN(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    bool onOrOff = obj["on"].toBool();


    int16_t value = 0;
    if (!readInt16(socket, 0x0a16, value, err)) {
        return false;
    }
    //将BIT 8 设置为1
    int bitNumber = 8;
    if (onOrOff) {
        value |= (1 << bitNumber);
    }
    else {
        value &= ~(1 << bitNumber);
    }

    if (!writeInt16(socket, 0x0a16, value, err))
        return false;

    return true;
}
bool ControlTestCommHandler::setAD2_UPDN(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    bool onOrOff = obj["on"].toBool();


    int16_t value = 0;
    if (!readInt16(socket, 0x0a16, value, err)) {
        return false;
    }
    //将BIT 9 设置为1
    int bitNumber = 9;
    if (onOrOff) {
        value |= (1 << bitNumber);
    }
    else {
        value &= ~(1 << bitNumber);
    }

    if (!writeInt16(socket, 0x0a16, value, err))
        return false;

    return true;
}

//写入X DIR
bool ControlTestCommHandler::setXDIR(CommunicationThread* socket, QJsonObject& obj, QString& err) {

    bool onOrOff = obj["on"].toBool();


    int16_t value = 0;
    if (!readInt16(socket, 0x0a10, value, err)) {
        return false;
    }
    //将BIT 9 设置为1
    int bitNumber = 9;
    if (onOrOff) {
        value |= (1 << bitNumber);
    }
    else {
        value &= ~(1 << bitNumber);
    }

    if (!writeInt16(socket, 0x0a10, value, err))
        return false;

    return true;
}

//写入YZ DIR
bool ControlTestCommHandler::setYZDIR(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	bool onOrOff = obj["on"].toBool();
	int16_t value = 0;
	if (!readInt16(socket, 0x0a10, value, err)) {
		return false;
	}
	//将BIT 10 设置为1
	int bitNumber = 10;
	if (onOrOff) {
		value |= (1 << bitNumber);
	}
	else {
		value &= ~(1 << bitNumber);
	}
	if (!writeInt16(socket, 0x0a10, value, err))
		return false;
	return true;
}


//写入 XGAIN
bool ControlTestCommHandler::setXGAIN(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    float gain = obj["GAIN"].toDouble();
    if (!writeFloat(socket, 0x0a70, gain, err))
        return false;
    return true;
}
//写入YZ GAIN
bool ControlTestCommHandler::setYZGAIN(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    float gain = obj["GAIN"].toDouble();
    if (!writeFloat(socket, 0x0a74, gain, err))
        return false;
    return true;
}


bool ControlTestCommHandler::setADCAP(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    int ad_number = obj["AD"].toInt();
    float gain = obj["CAP"].toDouble();
    QByteArray buffer;
    buffer.fill(0x00, 528);
    packPC_KEY(0x0e, ad_number - 1, gain, buffer);

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

bool ControlTestCommHandler::setADGAIN(CommunicationThread* socket, QJsonObject& obj, QString& err) {
    int ad_number = obj["AD"].toInt();
	float gain = obj["GAIN"].toDouble();
	QByteArray buffer;
	buffer.fill(0x00, 528);
	packPC_KEY(0x0f, ad_number - 1, gain, buffer);

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
    packPC_KEY(0x06,0x1F, buffer);
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
	qDebug() << "stop";
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
    qDebug() << "stop true";
	return true;
}

bool ControlTestCommHandler::spin(CommunicationThread* socket, QJsonObject& obj, QString& err) {

	//writeInt32(socket, 0X1102, 0, err);
    if(0)
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

    if(0)
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

//启动测试前的准备：回读当前角度（0x0900块偏移0x34，即地址0x0934，float），
//与设定角度比较，偏小则 moveup(spin)，偏大则 movedown(reSpin)，
//每次只移动一点点，故循环"移动->回读->调整"，直到 |实际-设定| <= 阈值
bool ControlTestCommHandler::prepareTest(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	double targetAngle = obj.contains("targetAngle") ? obj["targetAngle"].toDouble() : 0.0143;// obj["targetAngle"].toDouble();
	double threshold = obj.contains("threshold") ? obj["threshold"].toDouble() : 0.00005;
	const int maxIterations = 5000; //安全上限，防止异常时死循环

	//移动速度自适应：远离设定角度时用高速(500)，接近时线性降到 minSpeed
	const double maxSpeed = 500.0;
	const double minSpeed = 10.0;
	double firstAbsDiff = -1.0; //首次回读的|diff|，作为满速基准

	for (int i = 0; i < maxIterations; i++) {
		//读取当前角度：4字节，地址 0x0934（0x0900 + 0x34），按 float 解析
		int32_t Hex32 = 0;
		if (!readInt32(socket, 0x0934, Hex32, err)) {
			return false;
		}
		float currentAngle = *reinterpret_cast<float*>(&Hex32);
		qDebug() << "prepareTest current:" << currentAngle << "target:" << targetAngle;

		double diff = currentAngle - targetAngle;
		double absDiff = qAbs(diff);
		if (absDiff <= threshold) {
			return true; //达到阈值，停止
		}

		//首次记录基准|diff|，用于按比例计算速度
		if (firstAbsDiff < 0) {
			firstAbsDiff = absDiff;
		}
		//速度随接近程度线性减小：远离时 maxSpeed，接近时降到 minSpeed
		double ratio = firstAbsDiff > 0 ? (absDiff / firstAbsDiff) : 1.0;
		double speed = minSpeed + (maxSpeed - minSpeed) * ratio;
		if (speed > maxSpeed) speed = maxSpeed;
		if (speed < minSpeed) speed = minSpeed;
		qDebug() << "prepareTest absDiff:" << absDiff << "speed:" << speed;

		//写入移动速度
		if (!writeFloat(socket, 0x1108, static_cast<float>(speed), err))
			return false;

		//当前角度比设定角度小 -> moveup(spin)；否则 -> movedown(reSpin)
		QJsonObject dummy;
		bool moveOk = false;
		if (diff < 0) {
			moveOk = spin(socket, dummy, err);
		}
		else {
			moveOk = reSpin(socket, dummy, err);
		}
		if (!moveOk) {
			return false;
		}

		QThread::msleep(100); //等待电机移动一小段后再回读
	}

	err = "prepareTest exceeded max iterations";
	return false;
}
