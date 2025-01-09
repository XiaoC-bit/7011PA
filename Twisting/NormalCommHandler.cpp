#include "NormalCommHandler.h"

NormalCommHandler::NormalCommHandler(ReadU65Struct& ref, QObject* parent)
	: CommHandler(ref,parent)
{}

NormalCommHandler::~NormalCommHandler()
{
}

bool NormalCommHandler::commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err) {

	QString type = obj["__type"].toString();
	if (type == "real-data") {
		return realData(socket, obj, err);
	}
	else if (type == "read-data") {
		return readData(socket, obj, err);
	}
	return false;
}

bool NormalCommHandler::realData(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	//
	/**
	 * STEP #1
	 * 读取0900 00FF个字节的数据
	 */
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Read, buffer);
	setLength(0xFF, buffer);
	setAddr(0x0900, buffer);
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
	if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0xFF) {
		err = "readData error ,data error";
		return false;
	}


	/**
	 * STEP #2
	 * 分析每段区间的字节含义
	 */

	 /**
	  * 2：待测
	  * 3：测试中
	  */
	U65Info_.U65_MODE = (recvData[12 + 0x18] & 0xff) + (recvData[12 + 0x19] & 0xff) * 0x100;
	U65Info_.U65_MSG = (recvData[12 + 0x1a] & 0xff) + (recvData[12 + 0x1b] & 0xff) * 0x100;
	U65Info_.U65_MSG2 = (recvData[12 + 0x1c] & 0xff) + (recvData[12 + 0x1d] & 0xff) * 0x100;
	U65Info_.U65_MSG3 = (recvData[12 + 0x20] & 0xff) + (recvData[12 + 0x21] & 0xff) * 0x100 + (recvData[12 + 0x22] & 0xff) * 0x10000 + (recvData[12 + 0x23] & 0xff) * 0x1000000;

	obj["U65_MODE"] = U65Info_.U65_MODE;
	obj["U65_MSG"] = U65Info_.U65_MSG;
	obj["U65_MSG2"] = U65Info_.U65_MSG2;
	obj["U65_MSG3"] = U65Info_.U65_MSG3;

	//读取DF SET的数据
	int ir_temp = (recvData[12 + 0xC8] & 0x00ff) + (recvData[12 + 0xC9] & 0x00ff) * 0x0100;
	obj["DF_SET_IR_TEMP"] = ir_temp;
	/*
	* 暂时没有使用
	int IO1_OUT = (recvData[12 + 0x2c] & 0xff) + (recvData[12 + 0x2d] & 0xff) * 0x100;
	*/
	U65Info_.IO1_IN = (recvData[12 + 0x2e] & 0xff) + (recvData[12 + 0x2f] & 0xff) * 0x100;
	obj["IO1_IN"] = U65Info_.IO1_IN;

	int32_t  Hex32;
	Hex32 = (recvData[12 + 0x38] & 0xff) +
		(recvData[12 + 0x39] & 0xff) * 0x100 +
		(recvData[12 + 0x3a] & 0xff) * 0x10000 +
		(recvData[12 + 0x3b] & 0xff) * 0x1000000;
	float* pHex32 = (float*)&Hex32;
	U65Info_.SITA = *pHex32;//S*  的θ 角	
	obj["SITA"] = U65Info_.SITA;

	Hex32 = (recvData[12 + 0x3c] & 0xff) +
		(recvData[12 + 0x3d] & 0xff) * 0x100 +
		(recvData[12 + 0x3e] & 0xff) * 0x10000 +
		(recvData[12 + 0x3f] & 0xff) * 0x1000000;
	U65Info_.sStar = *pHex32; //S*
	U65Info_.sStar = U65Info_.sStar / 100;
	obj["sStar"] = U65Info_.sStar;


	Hex32 = (recvData[12 + 0x40] & 0xff) +
		(recvData[12 + 0x41] & 0xff) * 0x100 +
		(recvData[12 + 0x42] & 0xff) * 0x10000 +
		(recvData[12 + 0x43] & 0xff) * 0x1000000;
	U65Info_.AD_2 = *pHex32; //发泡力
	obj["P"] = U65Info_.AD_2;


	U65Info_.sDoubleQuotation = (U65Info_.sStar) * sin(U65Info_.SITA * 3.14 / 180);
	obj["sDoubleQuotation"] = U65Info_.sDoubleQuotation;

	U65Info_.sQuotation = (U65Info_.sStar) * cos(U65Info_.SITA * 3.14 / 180);
	obj["sQuotation"] = U65Info_.sQuotation;


	Hex32 = (recvData[12 + 0x54] & 0xff) +
		(recvData[12 + 0x54 + 1] & 0xff) * 0x100 +
		(recvData[12 + 0x54 + 2] & 0xff) * 0x10000 +
		(recvData[12 + 0x54 + 3] & 0xff) * 0x1000000;
	U65Info_.TEST_TM_MS = *pHex32;


	U65Info_.TEST_TIMER = (double)((((recvData[12 + 0x58] & 0x00ff) +
		(recvData[12 + 0x59] & 0x00ff) * 0x0100)) / 1000.0 +
		(recvData[12 + 0x5a] & 0x00ff) * 60 +
		(recvData[12 + 0x5b] & 0x00ff) * 3600);
	obj["testTimer"] = U65Info_.TEST_TIMER;

	U65Info_.HOLD_TIMER = (recvData[12 + 0x5c] & 0xff) +
		(recvData[12 + 0x5d] & 0xff) * 0x100 +
		(recvData[12 + 0x5e] & 0xff) * 0x10000 +
		(recvData[12 + 0x5f] & 0xff) * 0x1000000;
	obj["holdTimer"] = U65Info_.HOLD_TIMER;



	U65Info_.REC_NO1 = (recvData[12 + 0xE0] & 0xff) +
		(recvData[12 + 0xE1] & 0xff) * 0x100 +
		(recvData[12 + 0xE2] & 0xff) * 0x10000 +
		(recvData[12 + 0xE3] & 0xff) * 0x1000000;


	U65Info_.REC_COMP = (recvData[12 + 0xE4] & 0xff) +
		(recvData[12 + 0xE5] & 0xff) * 0x100 +
		(recvData[12 + 0xE6] & 0xff) * 0x10000 +
		(recvData[12 + 0xE7] & 0xff) * 0x1000000;

	U65Info_.REC_NO2 = (recvData[12 + 0xE8] & 0xff) +
		(recvData[12 + 0xE9] & 0xff) * 0x100 +
		(recvData[12 + 0xEA] & 0xff) * 0x10000 +
		(recvData[12 + 0xEB] & 0xff) * 0x1000000;


	Hex32 = (recvData[12 + 0x80] & 0xff) + (recvData[12 + 0x81] & 0xff) * 0x100;//上模温度
	U65Info_.upperTemp = Hex32 / 100.0;
	obj["upperTemp"] = U65Info_.upperTemp;
	Hex32 = (recvData[12 + 0x82] & 0xff) + (recvData[12 + 0x83] & 0xff) * 0x100;//下模温度
	U65Info_.lowerTemp = Hex32 / 100.0;
	obj["lowerTemp"] = U65Info_.lowerTemp;


	int k;
	for (k = 0; k < 6; k++)
	{
		Hex32 = (recvData[12 + 0x3c + 4 * k] & 0xff) +
			(recvData[12 + 0x3d + 4 * k] & 0xff) * 0x100 +
			(recvData[(0x3e) + 12 + (4 * k)] & 0xff) * 0x10000 +
			(recvData[12 + 0x3f + 4 * k] & 0xff) * 0x1000000;
		U65Info_.Ad[k].Value = *pHex32;
		U65Info_.Ad[k].ABS_AD = (recvData[12 + 0x60 + k * 4] & 0xff) + (recvData[12 + 0x61 + k * 4] & 0xff) * 0x100 + (recvData[12 + 0x62 + k * 4] & 0xff) * 0x10000 + (recvData[12 + 0x63 + k * 4] & 0xff) * 0x1000000 /*-0x800000*/;
	}

	obj["ABS_AD_0"] =(int) U65Info_.Ad[0].ABS_AD;
	obj["ABS_AD_1"] = (int)U65Info_.Ad[1].ABS_AD;

	//当前扭力值
	obj["currentTorque"] = U65Info_.Ad[0].Value / 100.0;

	//当前压力值
	obj["currentPressure"] = U65Info_.Ad[1].Value;

	//当前门尼
	obj["currentMooney"] = U65Info_.Ad[0].Value / 83.0;


	Hex32 = (recvData[12 + 0x78] & 0xff) +
		(recvData[12 + 0x79] & 0xff) * 0x100 +
		(recvData[12 + 0x7a] & 0xff) * 0x10000 +
		(recvData[12 + 0x7b] & 0xff) * 0x1000000;
	U65Info_.X_SPEED = *pHex32;
	obj["X_SPEED"] = U65Info_.X_SPEED;


	{
		//读取卡片信息
		QByteArray buffer;
		buffer.fill(0x00, 528);
		setCmd(E_Mode::Read, buffer);
		setLength(0xFF, buffer);
		setAddr(0x0000, buffer);
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
		if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0xFF) {
			err = "readData error ,data error";
			return false;
		}

		for (int mi_temp = 0; mi_temp < 15; mi_temp++) {
			for (int mi_temp1 = 0; mi_temp1 < 16; mi_temp1++) {
				U65Info_.CartDetail[mi_temp][mi_temp1] = (recvData[12 + mi_temp1 + mi_temp * 16] & 0x00ff);
			}
		}
	}

	//读取0A00
	{
		QByteArray buffer;
		buffer.fill(0x00, 528);
		setCmd(E_Mode::Read, buffer);
		setLength(0xFB, buffer);
		setAddr(0x0A00, buffer);
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
		 if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0xFB) {
			//0A00地址不检查 旧版软件未检查，只检查0900
			 err = "0A00 readData error ,data error";
			 return false;
		 }

		U65Info_.Sys_Flag1 = (recvData[0x10 + 12] & 0xff) + (recvData[0x10 + 13] & 0xff) * 0x100;
		if (U65Info_.Sys_Flag1 == 0) {
			qDebug() << "Sys_Flag1 is zero";
		}
		obj["Sys_Flag1"] = (int)U65Info_.Sys_Flag1;

		Hex32 = (recvData[12 + 0x28] & 0x00ff) + (recvData[12 + 0x29] & 0x00ff) * 0x0100 + (recvData[12 + 0x2A] & 0x00ff) * 0x010000 + (recvData[12 + 0x2B] & 0x00ff) * 0x01000000;
		U65Info_.stru_ReadU65ext.md_S_RATE = *pHex32;
		obj["S_RATE"] = U65Info_.stru_ReadU65ext.md_S_RATE;


		Hex32 = (recvData[12 + 0x2C] & 0x00ff) + (recvData[12 + 0x2D] & 0x00ff) * 0x0100 + (recvData[12 + 0x2E] & 0x00ff) * 0x010000 + (recvData[12 + 0x2F] & 0x00ff) * 0x01000000;
		U65Info_.stru_ReadU65ext.md_SITA_COMP = *pHex32; //
		obj["SITA_COMP"] = U65Info_.stru_ReadU65ext.md_SITA_COMP;

		obj["friction"] = U65Info_.stru_ReadU65ext.md_SITA_COMP / 83.0;

		
		Hex32 = (recvData[12 + 0x70] & 0x00ff) + (recvData[12 + 0x71] & 0x00ff) * 0x0100 + (recvData[12 + 0x72] & 0x00ff) * 0x010000 + (recvData[12 + 0x73] & 0x00ff) * 0x01000000;
		U65Info_.stru_ReadU65ext.md_X_RATE = *pHex32; //
		obj["X_RATE"] = U65Info_.stru_ReadU65ext.md_X_RATE;


		Hex32 = (recvData[12 + 0x28] & 0x00ff) + (recvData[12 + 0x29] & 0x00ff) * 0x0100 + (recvData[12 + 0x2A] & 0x00ff) * 0x010000 + (recvData[12 + 0x2B] & 0x00ff) * 0x01000000;
		U65Info_.stru_ReadU65ext.md_S_RATE = *pHex32; //
		obj["S_RATE"] = U65Info_.stru_ReadU65ext.md_S_RATE;

		Hex32 = (recvData[12 + 0x44] & 0x00ff) +
			(recvData[12 + 0x45] & 0x00ff) * 0x0100 +
			(recvData[12 + 0x46] & 0x00ff) * 0x010000 +
			(recvData[12 + 0x47] & 0x00ff) * 0x01000000;
		U65Info_.stru_ReadU65ext.md_S_STANDARD = *pHex32;
		obj["S_STANDARD"] = U65Info_.stru_ReadU65ext.md_S_STANDARD;

		Hex32 = (recvData[12 + 0x30] & 0x00ff) +
			(recvData[12 + 0x31] & 0x00ff) * 0x0100 +
			(recvData[12 + 0x32] & 0x00ff) * 0x010000 +
			(recvData[12 + 0x33] & 0x00ff) * 0x01000000;
		U65Info_.stru_ReadU65ext.md_BREAK_LEVEL = Hex32;
		obj["BREAK_LEVEL"] = U65Info_.stru_ReadU65ext.md_BREAK_LEVEL;
	}
	auto machType = machineType();
	switch (machType)
	{
		case CommHandler::MACH_TYPE::Mooney: {
			obj["machType"] = "Mooney";
			break;
		}
		case CommHandler::MACH_TYPE::Speed_Mooney: {
			obj["machType"] = "Speed_Mooney";
			break;
		}
		case CommHandler::MACH_TYPE::Sulfur: {
			obj["machType"] = "Sulfur";
			break;
		}
		default: {
			obj["machType"] = "Unknown";
			break;
		}
	}
	return true;
}


bool NormalCommHandler::readData(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	int addr = obj["addr"].toInt();
	int length = obj["length"].toInt();
	/**
	 * STEP #1
	 * 读取0900 00FF个字节的数据
	 */
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Read, buffer);
	setLength(0xFF, buffer);
	setAddr(addr, buffer);
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
	if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0xFF) {
		err = "readData error ,data error";
		return false;
	}

	int mi_temp = 0;
	int temp1 = (recvData[12 + mi_temp * 32] & 0xff) + (recvData[12 + 0x01 + mi_temp * 32] & 0xff) * 0x0100;
	int temp2 = (recvData[12 + 0x02 + mi_temp * 32] & 0xff) + (recvData[12 + 0x03 + mi_temp * 32] & 0xff) * 0x0100;


	//qDebug() << "fake Info.upperTemp " << temp1;
	//qDebug() << "fake Info.lowerTemp " << temp2;

	obj["upperTemp"] = float(temp1) / 100;
	obj["lowerTemp"] = float(temp2 )/ 100;

	float sStar, SITA;

	int32_t  Hex32;
	Hex32 = (recvData[12 + 0x04 + mi_temp * 32] & 0xff) + 
		(recvData[12 + 0x05 + mi_temp * 32] & 0xff) * 0x100 +
		(recvData[12 + 0x06 + mi_temp * 32] & 0xff) * 0x10000 +
		(recvData[12 + 0x07 + mi_temp * 32] & 0xff) * 0x1000000;
	float* pHex32 = (float*)&Hex32;
	sStar = *pHex32; //S*
	

	auto machType = machineType();
	switch (machType)
	{
	case CommHandler::MACH_TYPE::Mooney: {
		//门尼机型的sStar就是门尼值
		sStar = sStar / 83.0;
		obj["machType"] = "Mooney";
		break;
	}
	case CommHandler::MACH_TYPE::Speed_Mooney: {
		sStar = sStar / 83.0;
		obj["machType"] = "Speed_Mooney";
		break;
	}
	case CommHandler::MACH_TYPE::Sulfur: {
		sStar = sStar / 100;
		obj["machType"] = "Sulfur";
		break;
	}
	default: {
		obj["machType"] = "Unknown";
		break;
	}
	}

	obj["sStar"] = sStar;




	Hex32 = (recvData[12 + 0x08 + mi_temp * 32] & 0xff) +
		(recvData[12 + 0x09 + mi_temp * 32] & 0xff) * 0x100 +
		(recvData[12 + 0x0A + mi_temp * 32] & 0xff) * 0x10000 +
		(recvData[12 + 0x0B + mi_temp * 32] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["SITA"] = *pHex32;
	SITA = *pHex32;

	Hex32 = (recvData[12 + 0x0C + mi_temp * 32] & 0xff) +
		(recvData[12 + 0x0D + mi_temp * 32] & 0xff) * 0x100 +
		(recvData[12 + 0x0E + mi_temp * 32] & 0xff) * 0x10000 +
		(recvData[12 + 0x0F + mi_temp * 32] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["P"] = *pHex32;

	double sDoubleQuotation = (sStar)*sin(SITA * 3.14 / 180);
	double sQuotation = (sStar)*cos(SITA * 3.14 / 180);
	obj["sDoubleQuotation"] = sDoubleQuotation;
	obj["sQuotation"] = sQuotation;

	obj["angle"] = SITA * 3.14 / 180;
	if (sQuotation == 0)
		obj["tanPA"] = 0;
	else
		obj["tanPA"] = sDoubleQuotation / sQuotation;// 自己计算tanPA
	
	obj["testTimer"] = (double)((((recvData[12 + 0x1C + mi_temp * 32] & 0x00ff) +
		(recvData[12 + 0x1D + mi_temp * 32] & 0x00ff) * 0x0100)) / 1000.0 +
		(recvData[mi_temp * 32 + 12 + 0x1E] & 0x00ff) * 60 +
		(recvData[12 + 0x1F + mi_temp * 32] & 0x00ff) * 3600);

	int StepNo = (recvData[12 + 0x16 + mi_temp * 32] & 0xff) + (recvData[12 + 0x17 + mi_temp * 32] & 0xff) * 0x100;
	obj["stepNo"] = StepNo;

	return true;
}
