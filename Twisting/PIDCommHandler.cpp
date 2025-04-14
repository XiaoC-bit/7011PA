#include "PIDCommHandler.h"
#include "DataDefine.h"
PIDCommHandler::PIDCommHandler(ReadU65Struct& ref, QObject* parent)
	: CommHandler(ref,parent)
{}

PIDCommHandler::~PIDCommHandler()
{
}

bool PIDCommHandler::commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err) {

	QString type = obj["__type"].toString();
	
	if (type == "read-data") {
		return readData(socket, obj, err);
	}
	else if (type == "write-data") {
		return writeData(socket, obj, err);
	}
	return false;
}

/**
 * 1.从obj中提取PID组数
 * 2.第一组PID的地址为0x2000，第二组为0x2100，第三组为0x2200,以此类推
 * 
 * \param socket
 * \param obj
 * \param err
 * \return 
 */
bool PIDCommHandler::readData(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	//从obj中提取PID组数,类型为int
	int group = obj["group"].toInt();
	if (group < 0 || group > 32) {
		err = "group error";
		return false;
	}

	//计算地址
	int addr = 0x1000 + group * 0x100;
	int length = 0xff;



	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Read, buffer);
	setLength(length, buffer);
	setAddr(addr, buffer);
	calcSum(buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		qDebug() << "123xxx";
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		qDebug() << "123xxx";
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		qDebug() << "123xxx";
		return false;
	}


	 //PID_BANK
	obj["PID_BANK"] = (recvData[12 + 0x00] & 0xff) + (recvData[12 + 0x01] & 0xff) * 0x100;
	//PID MODE
	obj["PID_MODE"] = (recvData[12 + 0x02] & 0xff) + (recvData[12 + 0x03] & 0xff) * 0x100;
	//PID COMD
	obj["PID_COMD"] = (recvData[12 + 0x04] & 0xff) + (recvData[12 + 0x05] & 0xff) * 0x100;
	//PID MSG
	obj["PID_MSG"] = (recvData[12 + 0x06] & 0xff) + (recvData[12 + 0x07] & 0xff) * 0x100;
	//IR_SPEED
	int32_t Hex32 = (recvData[12 + 0x08] & 0xff) +
		(recvData[12 + 0x09] & 0xff) * 0x100 +
		(recvData[12 + 0x0A] & 0xff) * 0x10000 +
		(recvData[12 + 0x0B] & 0xff) * 0x1000000;
	float* pHex32 = (float*)&Hex32;
	obj["IR_SPEED"] = *pHex32;
	//IR_POS
	Hex32 = (recvData[12 + 0x0C] & 0xff) +
		(recvData[12 + 0x0D] & 0xff) * 0x100 +
		(recvData[12 + 0x0E] & 0xff) * 0x10000 +
		(recvData[12 + 0x0F] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["IR_POS"] = *pHex32;
	//KP
	Hex32 = (recvData[12 + 0x10] & 0xff) +
		(recvData[12 + 0x11] & 0xff) * 0x100 +
		(recvData[12 + 0x12] & 0xff) * 0x10000 +
		(recvData[12 + 0x13] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["KP"] = *pHex32;
	//KI
	Hex32 = (recvData[12 + 0x14] & 0xff) +
		(recvData[12 + 0x15] & 0xff) * 0x100 +
		(recvData[12 + 0x16] & 0xff) * 0x10000 +
		(recvData[12 + 0x17] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["KI"] = *pHex32;
	//KD
	Hex32 = (recvData[12 + 0x18] & 0xff) +
		(recvData[12 + 0x19] & 0xff) * 0x100 +
		(recvData[12 + 0x1A] & 0xff) * 0x10000 +
		(recvData[12 + 0x1B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["KD"] = *pHex32;
	//S_I_LIM +
	Hex32 = (recvData[12 + 0x1C] & 0xff) +
		(recvData[12 + 0x1D] & 0xff) * 0x100 +
		(recvData[12 + 0x1E] & 0xff) * 0x10000 +
		(recvData[12 + 0x1F] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["S_I_LIM_PLUS"] = *pHex32;
	//S_I_LIM -
	Hex32 = (recvData[12 + 0x20] & 0xff) +
		(recvData[12 + 0x21] & 0xff) * 0x100 +
		(recvData[12 + 0x22] & 0xff) * 0x10000 +
		(recvData[12 + 0x23] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["S_I_LIM_MINUS"] = *pHex32;
	//PERIOD
	obj["PERIOD"] = (recvData[12 + 0x24] & 0xff) + (recvData[12 + 0x25] & 0xff) * 0x100;
	//IR_TARGET
	obj["IR_TARGET"] = (recvData[12 + 0x28] & 0xff) + (recvData[12 + 0x29] & 0xff) * 0x100;
	//FB_SOURCE
	obj["FB_SOURCE"] = (recvData[12 + 0x2A] & 0xff) + (recvData[12 + 0x2B] & 0xff) * 0x100;
	//REV_STOP
	obj["REV_STOP"] = (recvData[12 + 0x2C] & 0xff) + (recvData[12 + 0x2D] & 0xff) * 0x100;
	//SV OFF DELAY
	obj["SV_OFF_DELAY"] = (recvData[12 + 0x2E] & 0xff) + (recvData[12 + 0x2F] & 0xff) * 0x100;
	//F_SOURCE
	obj["F_SOURCE"] = (recvData[12 + 0x30] & 0xff) + (recvData[12 + 0x31] & 0xff) * 0x100;
	//IMPACT_TIMES
	obj["IMPACT_TIMES"] = (recvData[12 + 0x32] & 0xff) + (recvData[12 + 0x33] & 0xff) * 0x100;
	



	//IMPACT_LEVEL
	Hex32 = (recvData[12 + 0x34] & 0xff) +
		(recvData[12 + 0x35] & 0xff) * 0x100 +
		(recvData[12 + 0x36] & 0xff) * 0x10000 +
		(recvData[12 + 0x37] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["IMPACT_LEVEL"] = *pHex32;

	//F_HOLD_UP
	Hex32 = (recvData[12 + 0x38] & 0xff) +
		(recvData[12 + 0x39] & 0xff) * 0x100 +
		(recvData[12 + 0x3A] & 0xff) * 0x10000 +
		(recvData[12 + 0x3B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["F_HOLD_UP"] = *pHex32;

	//PS_ACC_TIME
	obj["PS_ACC_TIME"] = (recvData[12 + 0x40] & 0xff) + (recvData[12 + 0x41] & 0xff) * 0x100;

	//PS_DEC_TIME
	obj["PS_DEC_TIME"] = (recvData[12 + 0x42] & 0xff) + (recvData[12 + 0x43] & 0xff) * 0x100;

	//PS MODE
	obj["PS_MODE"] = (recvData[12 + 0x44] & 0xff) + (recvData[12 + 0x45] & 0xff) * 0x100;

	//DA_TYPE
	obj["DA_TYPE"] = (recvData[12 + 0x46] & 0xff) + (recvData[12 + 0x47] & 0xff) * 0x100;

	//PS_PID_MIN
	Hex32 = (recvData[12 + 0x48] & 0xff) +
		(recvData[12 + 0x49] & 0xff) * 0x100 +
		(recvData[12 + 0x4A] & 0xff) * 0x10000 +
		(recvData[12 + 0x4B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["PS_PID_MIN"] = *pHex32;

	//PS_SP_LIM
	Hex32 = (recvData[12 + 0x4C] & 0xff) +
		(recvData[12 + 0x4D] & 0xff) * 0x100 +
		(recvData[12 + 0x4E] & 0xff) * 0x10000 +
		(recvData[12 + 0x4F] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["PS_SP_LIM"] = *pHex32;

	//DA_OFFSET+
	Hex32 = (recvData[12 + 0x50] & 0xff) +
		(recvData[12 + 0x51] & 0xff) * 0x100 +
		(recvData[12 + 0x52] & 0xff) * 0x10000 +
		(recvData[12 + 0x53] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["DA_OFFSET_PLUS"] = *pHex32;

	//DA_OFFSET-
	Hex32 = (recvData[12 + 0x54] & 0xff) +
		(recvData[12 + 0x55] & 0xff) * 0x100 +
		(recvData[12 + 0x56] & 0xff) * 0x10000 +
		(recvData[12 + 0x57] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["DA_OFFSET_MINUS"] = *pHex32;

	//DA 滿刻度
	Hex32 = (recvData[12 + 0x58] & 0xff) +
		(recvData[12 + 0x59] & 0xff) * 0x100 +
		(recvData[12 + 0x5A] & 0xff) * 0x10000 +
		(recvData[12 + 0x5B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["DA_FULL_SCALE"] = *pHex32;

	//DA_LIM_+
	obj["DA_LIM_PLUS"] = (recvData[12 + 0x5C] & 0xff) + (recvData[12 + 0x5D] & 0xff) * 0x100;

	//DA_LIM_-
	obj["DA_LIM_MINUS"] = (recvData[12 + 0x5E] & 0xff) + (recvData[12 + 0x5F] & 0xff) * 0x100;

	//DA_ACC_TIME
	obj["DA_ACC_TIME"] = (recvData[12 + 0x60] & 0xff) + (recvData[12 + 0x61] & 0xff) * 0x100;

	//DA_DEC_TIME
	obj["DA_DEC_TIME"] = (recvData[12 + 0x62] & 0xff) + (recvData[12 + 0x63] & 0xff) * 0x100;

	//DA MODE
	obj["DA_MODE"] = (recvData[12 + 0x64] & 0xff) + (recvData[12 + 0x65] & 0xff) * 0x100;

	//KF
	Hex32 = (recvData[12 + 0x68] & 0xff) +
		(recvData[12 + 0x69] & 0xff) * 0x100 +
		(recvData[12 + 0x6A] & 0xff) * 0x10000 +
		(recvData[12 + 0x6B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["KF"] = *pHex32;

	//DF_SIGNAL
	obj["DF_SIGNAL"] = (recvData[12 + 0x6C] & 0xff) + (recvData[12 + 0x6D] & 0xff) * 0x100;

	//STOP_MODE
	obj["STOP_MODE"] = (recvData[12 + 0x6E] & 0xff) + (recvData[12 + 0x6F] & 0xff) * 0x100;

	//CYCLE_KI
	Hex32 = (recvData[12 + 0x70] & 0xff) +
		(recvData[12 + 0x71] & 0xff) * 0x100 +
		(recvData[12 + 0x72] & 0xff) * 0x10000 +
		(recvData[12 + 0x73] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["CYCLE_KI"] = *pHex32;

	//CYCLE_RATE_MAX
	Hex32 = (recvData[12 + 0x74] & 0xff) +
		(recvData[12 + 0x75] & 0xff) * 0x100 +
		(recvData[12 + 0x76] & 0xff) * 0x10000 +
		(recvData[12 + 0x77] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["CYCLE_RATE_MAX"] = *pHex32;

	//CYCLE_RATE_MIN
	Hex32 = (recvData[12 + 0x78] & 0xff) +
		(recvData[12 + 0x79] & 0xff) * 0x100 +
		(recvData[12 + 0x7A] & 0xff) * 0x10000 +
		(recvData[12 + 0x7B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["CYCLE_RATE_MIN"] = *pHex32;

	//SPEED_SIDE_KP
	Hex32 = (recvData[12 + 0x7C] & 0xff) +
		(recvData[12 + 0x7D] & 0xff) * 0x100 +
		(recvData[12 + 0x7E] & 0xff) * 0x10000 +
		(recvData[12 + 0x7F] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["SPEED_SIDE_KP"] = *pHex32;

	//DF_START_RATE
	Hex32 = (recvData[12 + 0x80] & 0xff) +
		(recvData[12 + 0x81] & 0xff) * 0x100 +
		(recvData[12 + 0x82] & 0xff) * 0x10000 +
		(recvData[12 + 0x83] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["DF_START_RATE"] = *pHex32;

	//AUTO_SIZE_KP
	Hex32 = (recvData[12 + 0x84] & 0xff) +
		(recvData[12 + 0x85] & 0xff) * 0x100 +
		(recvData[12 + 0x86] & 0xff) * 0x10000 +
		(recvData[12 + 0x87] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["AUTO_SIZE_KP"] = *pHex32;

	//AUTO_SIDE_MAX
	Hex32 = (recvData[12 + 0x88] & 0xff) +
		(recvData[12 + 0x89] & 0xff) * 0x100 +
		(recvData[12 + 0x8A] & 0xff) * 0x10000 +
		(recvData[12 + 0x8B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["AUTO_SIDE_MAX"] = *pHex32;

	//AUTO_SIDE_MIN
	Hex32 = (recvData[12 + 0x8C] & 0xff) +
		(recvData[12 + 0x8D] & 0xff) * 0x100 +
		(recvData[12 + 0x8E] & 0xff) * 0x10000 +
		(recvData[12 + 0x8F] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["AUTO_SIDE_MIN"] = *pHex32;

	//KP_N
	Hex32 = (recvData[12 + 0x90] & 0xff) +
		(recvData[12 + 0x91] & 0xff) * 0x100 +
		(recvData[12 + 0x92] & 0xff) * 0x10000 +
		(recvData[12 + 0x93] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["KP_N"] = *pHex32;

	//IR_DC
	Hex32 = (recvData[12 + 0x94] & 0xff) +
		(recvData[12 + 0x95] & 0xff) * 0x100 +
		(recvData[12 + 0x96] & 0xff) * 0x10000 +
		(recvData[12 + 0x97] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["IR_DC"] = *pHex32;

	//DA_LIM_P_V
	Hex32 = (recvData[12 + 0x98] & 0xff) +
		(recvData[12 + 0x99] & 0xff) * 0x100 +
		(recvData[12 + 0x9A] & 0xff) * 0x10000 +
		(recvData[12 + 0x9B] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["DA_LIM_P_V"] = *pHex32;

	//DA_LIM_N_V
	Hex32 = (recvData[12 + 0x9C] & 0xff) +
		(recvData[12 + 0x9D] & 0xff) * 0x100 +
		(recvData[12 + 0x9E] & 0xff) * 0x10000 +
		(recvData[12 + 0x9F] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["DA_LIM_N_V"] = *pHex32;

	//LOCK_RATE
	Hex32 = (recvData[12 + 0xA0] & 0xff) +
		(recvData[12 + 0xA1] & 0xff) * 0x100 +
		(recvData[12 + 0xA2] & 0xff) * 0x10000 +
		(recvData[12 + 0xA3] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["LOCK_RATE"] = *pHex32;

	//PULSE_RATE
	Hex32 = (recvData[12 + 0xA4] & 0xff) +
		(recvData[12 + 0xA5] & 0xff) * 0x100 +
		(recvData[12 + 0xA6] & 0xff) * 0x10000 +
		(recvData[12 + 0xA7] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["PULSE_RATE"] = *pHex32;

	//HALF_WAVE_RATIO
	Hex32 = (recvData[12 + 0xA8] & 0xff) +
		(recvData[12 + 0xA9] & 0xff) * 0x100 +
		(recvData[12 + 0xAA] & 0xff) * 0x10000 +
		(recvData[12 + 0xAB] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["HALF_WAVE_RATIO"] = *pHex32;

	//MDR2_IR_F
	Hex32 = (recvData[12 + 0xAC] & 0xff) +
		(recvData[12 + 0xAD] & 0xff) * 0x100 +
		(recvData[12 + 0xAE] & 0xff) * 0x10000 +
		(recvData[12 + 0xAF] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["MDR2_IR_F"] = *pHex32;

	//MDR2_KI
	Hex32 = (recvData[12 + 0xB0] & 0xff) +
		(recvData[12 + 0xB1] & 0xff) * 0x100 +
		(recvData[12 + 0xB2] & 0xff) * 0x10000 +
		(recvData[12 + 0xB3] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["MDR2_KI"] = *pHex32;

	//SVON_HOLD_TIME
	Hex32 = (recvData[12 + 0xB4] & 0xff) +
		(recvData[12 + 0xB5] & 0xff) * 0x100 +
		(recvData[12 + 0xB6] & 0xff) * 0x10000 +
		(recvData[12 + 0xB7] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["SVON_HOLD_TIME"] = *pHex32;

	//RUN_OUT_LIM_OFF
	Hex32 = (recvData[12 + 0xB8] & 0xff) +
		(recvData[12 + 0xB9] & 0xff) * 0x100 +
		(recvData[12 + 0xBA] & 0xff) * 0x10000 +
		(recvData[12 + 0xBB] & 0xff) * 0x1000000;
	pHex32 = (float*)&Hex32;
	obj["RUN_OUT_LIM_OFF"] = *pHex32;



	return true;
	
}


bool PIDCommHandler::writeData(CommunicationThread* socket, QJsonObject& recvObj, QString& err) {
	//从obj中提取PID组数,类型为int
	int group = recvObj["group"].toInt();
	if (group < 0 || group > 32) {
		err = "group error";
		return false;
	}

	QJsonObject obj = recvObj["pid"].toObject();
	//计算地址
	int addr = 0x1000 + group * 0x100;
	int length = 0xff;
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(length, buffer);
	setAddr(addr, buffer);
	
	//从obj中提取数据，填充到buffer中
	// 2-byte integers
	// 2-byte integers
	/*buffer[12 + 0x00] = int8_t(obj["PID_BANK"].toInt() & 0x00FF);
	buffer[12 + 0x01] = int8_t((obj["PID_BANK"].toInt() & 0xFF00) >> 8);*/
	buffer[12 + 0x00] = int8_t(group & 0x00FF);
	buffer[12 + 0x01] = int8_t((group & 0xFF00) >> 8);
	buffer[12 + 0x02] = int8_t(obj["PID_MODE"].toInt() & 0x00FF);
	buffer[12 + 0x03] = int8_t((obj["PID_MODE"].toInt() & 0xFF00) >> 8);
	buffer[12 + 0x04] = int8_t(obj["PID_COMD"].toInt() & 0x00FF);
	buffer[12 + 0x05] = int8_t((obj["PID_COMD"].toInt() & 0xFF00) >> 8);
	buffer[12 + 0x06] = int8_t(obj["PID_MSG"].toInt() & 0x00FF);
	buffer[12 + 0x07] = int8_t((obj["PID_MSG"].toInt() & 0xFF00) >> 8);

	// 4-byte floating point: IR_SPEED
	float fValue = obj["IR_SPEED"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x08] = ptr[0];
		buffer[12 + 0x09] = ptr[1];
		buffer[12 + 0x0A] = ptr[2];
		buffer[12 + 0x0B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x08] = ptr[3];
		buffer[12 + 0x09] = ptr[2];
		buffer[12 + 0x0A] = ptr[1];
		buffer[12 + 0x0B] = ptr[0];
	}

	// 4-byte floating point: IR_POS
	fValue = obj["IR_POS"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x0C] = ptr[0];
		buffer[12 + 0x0D] = ptr[1];
		buffer[12 + 0x0E] = ptr[2];
		buffer[12 + 0x0F] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x0C] = ptr[3];
		buffer[12 + 0x0D] = ptr[2];
		buffer[12 + 0x0E] = ptr[1];
		buffer[12 + 0x0F] = ptr[0];
	}

	// 4-byte floating point: KP
	fValue = obj["KP"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x10] = ptr[0];
		buffer[12 + 0x11] = ptr[1];
		buffer[12 + 0x12] = ptr[2];
		buffer[12 + 0x13] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x10] = ptr[3];
		buffer[12 + 0x11] = ptr[2];
		buffer[12 + 0x12] = ptr[1];
		buffer[12 + 0x13] = ptr[0];
	}

	// 4-byte floating point: KI
	fValue = obj["KI"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x14] = ptr[0];
		buffer[12 + 0x15] = ptr[1];
		buffer[12 + 0x16] = ptr[2];
		buffer[12 + 0x17] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x14] = ptr[3];
		buffer[12 + 0x15] = ptr[2];
		buffer[12 + 0x16] = ptr[1];
		buffer[12 + 0x17] = ptr[0];
	}

	// 4-byte floating point: KD
	fValue = obj["KD"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x18] = ptr[0];
		buffer[12 + 0x19] = ptr[1];
		buffer[12 + 0x1A] = ptr[2];
		buffer[12 + 0x1B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x18] = ptr[3];
		buffer[12 + 0x19] = ptr[2];
		buffer[12 + 0x1A] = ptr[1];
		buffer[12 + 0x1B] = ptr[0];
	}

	// 4-byte floating point: S_I_LIM+
	fValue = obj["S_I_LIM_PLUS"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x1C] = ptr[0];
		buffer[12 + 0x1D] = ptr[1];
		buffer[12 + 0x1E] = ptr[2];
		buffer[12 + 0x1F] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x1C] = ptr[3];
		buffer[12 + 0x1D] = ptr[2];
		buffer[12 + 0x1E] = ptr[1];
		buffer[12 + 0x1F] = ptr[0];
	}

	// 4-byte floating point: S_I_LIM-
	fValue = obj["S_I_LIM_MINUS"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x20] = ptr[0];
		buffer[12 + 0x21] = ptr[1];
		buffer[12 + 0x22] = ptr[2];
		buffer[12 + 0x23] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x20] = ptr[3];
		buffer[12 + 0x21] = ptr[2];
		buffer[12 + 0x22] = ptr[1];
		buffer[12 + 0x23] = ptr[0];
	}

	// 2-byte integer: PERIOD
	buffer[12 + 0x24] = int8_t(obj["PERIOD"].toInt() & 0x00FF);
	buffer[12 + 0x25] = int8_t((obj["PERIOD"].toInt() & 0xFF00) >> 8);

	// 0x26-0x27 is empty or reserved

	// 2-byte integer: IR_TARGET
	buffer[12 + 0x28] = int8_t(obj["IR_TARGET"].toInt() & 0x00FF);
	buffer[12 + 0x29] = int8_t((obj["IR_TARGET"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: FB_SOURCE
	buffer[12 + 0x2A] = int8_t(obj["FB_SOURCE"].toInt() & 0x00FF);
	buffer[12 + 0x2B] = int8_t((obj["FB_SOURCE"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: REV_STOP
	buffer[12 + 0x2C] = int8_t(obj["REV_STOP"].toInt() & 0x00FF);
	buffer[12 + 0x2D] = int8_t((obj["REV_STOP"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: SV_OFF_DELAY
	buffer[12 + 0x2E] = int8_t(obj["SV_OFF_DELAY"].toInt() & 0x00FF);
	buffer[12 + 0x2F] = int8_t((obj["SV_OFF_DELAY"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: F_SOURCE
	buffer[12 + 0x30] = int8_t(obj["F_SOURCE"].toInt() & 0x00FF);
	buffer[12 + 0x31] = int8_t((obj["F_SOURCE"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: IMPACT_TIMES
	buffer[12 + 0x32] = int8_t(obj["IMPACT_TIMES"].toInt() & 0x00FF);
	buffer[12 + 0x33] = int8_t((obj["IMPACT_TIMES"].toInt() & 0xFF00) >> 8);

	// 4-byte floating point: IMPACT_LEVEL
	fValue = obj["IMPACT_LEVEL"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x34] = ptr[0];
		buffer[12 + 0x35] = ptr[1];
		buffer[12 + 0x36] = ptr[2];
		buffer[12 + 0x37] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x34] = ptr[3];
		buffer[12 + 0x35] = ptr[2];
		buffer[12 + 0x36] = ptr[1];
		buffer[12 + 0x37] = ptr[0];
	}

	// 4-byte floating point: F_HOLD_UP
	fValue = obj["F_HOLD_UP"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x38] = ptr[0];
		buffer[12 + 0x39] = ptr[1];
		buffer[12 + 0x3A] = ptr[2];
		buffer[12 + 0x3B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x38] = ptr[3];
		buffer[12 + 0x39] = ptr[2];
		buffer[12 + 0x3A] = ptr[1];
		buffer[12 + 0x3B] = ptr[0];
	}

	// 0x3C-0x3F appears to be empty or reserved

	// 2-byte integer: PS_ACC_TIME
	buffer[12 + 0x40] = int8_t(obj["PS_ACC_TIME"].toInt() & 0x00FF);
	buffer[12 + 0x41] = int8_t((obj["PS_ACC_TIME"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: PS_DEC_TIME
	buffer[12 + 0x42] = int8_t(obj["PS_DEC_TIME"].toInt() & 0x00FF);
	buffer[12 + 0x43] = int8_t((obj["PS_DEC_TIME"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: PS_MODE
	buffer[12 + 0x44] = int8_t(obj["PS_MODE"].toInt() & 0x00FF);
	buffer[12 + 0x45] = int8_t((obj["PS_MODE"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: DA_TYPE
	buffer[12 + 0x46] = int8_t(obj["DA_TYPE"].toInt() & 0x00FF);
	buffer[12 + 0x47] = int8_t((obj["DA_TYPE"].toInt() & 0xFF00) >> 8);

	// 4-byte floating point: PS_PID_MIN
	fValue = obj["PS_PID_MIN"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x48] = ptr[0];
		buffer[12 + 0x49] = ptr[1];
		buffer[12 + 0x4A] = ptr[2];
		buffer[12 + 0x4B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x48] = ptr[3];
		buffer[12 + 0x49] = ptr[2];
		buffer[12 + 0x4A] = ptr[1];
		buffer[12 + 0x4B] = ptr[0];
	}

	// 4-byte floating point: PS_SP_LIM
	fValue = obj["PS_SP_LIM"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x4C] = ptr[0];
		buffer[12 + 0x4D] = ptr[1];
		buffer[12 + 0x4E] = ptr[2];
		buffer[12 + 0x4F] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x4C] = ptr[3];
		buffer[12 + 0x4D] = ptr[2];
		buffer[12 + 0x4E] = ptr[1];
		buffer[12 + 0x4F] = ptr[0];
	}

	// 4-byte floating point: DA_OFFSET+
	fValue = obj["DA_OFFSET_PLUS"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x50] = ptr[0];
		buffer[12 + 0x51] = ptr[1];
		buffer[12 + 0x52] = ptr[2];
		buffer[12 + 0x53] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x50] = ptr[3];
		buffer[12 + 0x51] = ptr[2];
		buffer[12 + 0x52] = ptr[1];
		buffer[12 + 0x53] = ptr[0];
	}

	// 4-byte floating point: DA_OFFSET-
	fValue = obj["DA_OFFSET_MINUS"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x54] = ptr[0];
		buffer[12 + 0x55] = ptr[1];
		buffer[12 + 0x56] = ptr[2];
		buffer[12 + 0x57] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x54] = ptr[3];
		buffer[12 + 0x55] = ptr[2];
		buffer[12 + 0x56] = ptr[1];
		buffer[12 + 0x57] = ptr[0];
	}

	// 4-byte floating point: DA_滿刻度
	fValue = obj["DA_FULL_SCALE"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x58] = ptr[0];
		buffer[12 + 0x59] = ptr[1];
		buffer[12 + 0x5A] = ptr[2];
		buffer[12 + 0x5B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x58] = ptr[3];
		buffer[12 + 0x59] = ptr[2];
		buffer[12 + 0x5A] = ptr[1];
		buffer[12 + 0x5B] = ptr[0];
	}

	// 2-byte integer: DA_LIM_+
	buffer[12 + 0x5C] = int8_t(obj["DA_LIM_PLUS"].toInt() & 0x00FF);
	buffer[12 + 0x5D] = int8_t((obj["DA_LIM_MINUS"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: DA_LIM_-
	buffer[12 + 0x5E] = int8_t(obj["DA_LIM_PLUS"].toInt() & 0x00FF);
	buffer[12 + 0x5F] = int8_t((obj["DA_LIM_MINUS"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: DA_ACC_TIME
	buffer[12 + 0x60] = int8_t(obj["DA_ACC_TIME"].toInt() & 0x00FF);
	buffer[12 + 0x61] = int8_t((obj["DA_ACC_TIME"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: DA_DEC_TIME
	buffer[12 + 0x62] = int8_t(obj["DA_DEC_TIME"].toInt() & 0x00FF);
	buffer[12 + 0x63] = int8_t((obj["DA_DEC_TIME"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: DA_MODE
	buffer[12 + 0x64] = int8_t(obj["DA_MODE"].toInt() & 0x00FF);
	buffer[12 + 0x65] = int8_t((obj["DA_MODE"].toInt() & 0xFF00) >> 8);

	// 0x66-0x67 appears to be empty or reserved

	// 4-byte floating point: KF
	fValue = obj["KF"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x68] = ptr[0];
		buffer[12 + 0x69] = ptr[1];
		buffer[12 + 0x6A] = ptr[2];
		buffer[12 + 0x6B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x68] = ptr[3];
		buffer[12 + 0x69] = ptr[2];
		buffer[12 + 0x6A] = ptr[1];
		buffer[12 + 0x6B] = ptr[0];
	}

	// 2-byte integer: DF_SIGNAL
	buffer[12 + 0x6C] = int8_t(obj["DF_SIGNAL"].toInt() & 0x00FF);
	buffer[12 + 0x6D] = int8_t((obj["DF_SIGNAL"].toInt() & 0xFF00) >> 8);

	// 2-byte integer: STOP_MODE
	buffer[12 + 0x6E] = int8_t(obj["STOP_MODE"].toInt() & 0x00FF);
	buffer[12 + 0x6F] = int8_t((obj["STOP_MODE"].toInt() & 0xFF00) >> 8);

	// 4-byte floating point: CYCLE_KI
	fValue = obj["CYCLE_KI"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x70] = ptr[0];
		buffer[12 + 0x71] = ptr[1];
		buffer[12 + 0x72] = ptr[2];
		buffer[12 + 0x73] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x70] = ptr[3];
		buffer[12 + 0x71] = ptr[2];
		buffer[12 + 0x72] = ptr[1];
		buffer[12 + 0x73] = ptr[0];
	}

	// 4-byte floating point: CYCLE_RATE_MAX
	fValue = obj["CYCLE_RATE_MAX"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x74] = ptr[0];
		buffer[12 + 0x75] = ptr[1];
		buffer[12 + 0x76] = ptr[2];
		buffer[12 + 0x77] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x74] = ptr[3];
		buffer[12 + 0x75] = ptr[2];
		buffer[12 + 0x76] = ptr[1];
		buffer[12 + 0x77] = ptr[0];
	}

	// 4-byte floating point: CYCLE_RATE_MIN
	fValue = obj["CYCLE_RATE_MIN"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x78] = ptr[0];
		buffer[12 + 0x79] = ptr[1];
		buffer[12 + 0x7A] = ptr[2];
		buffer[12 + 0x7B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x78] = ptr[3];
		buffer[12 + 0x79] = ptr[2];
		buffer[12 + 0x7A] = ptr[1];
		buffer[12 + 0x7B] = ptr[0];
	}

	// 4-byte floating point: SPEED_SIDE_KP
	fValue = obj["SPEED_SIDE_KP"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x7C] = ptr[0];
		buffer[12 + 0x7D] = ptr[1];
		buffer[12 + 0x7E] = ptr[2];
		buffer[12 + 0x7F] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x7C] = ptr[3];
		buffer[12 + 0x7D] = ptr[2];
		buffer[12 + 0x7E] = ptr[1];
		buffer[12 + 0x7F] = ptr[0];
	}

	// 4-byte floating point: DF_START_RATE
	fValue = obj["DF_START_RATE"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x80] = ptr[0];
		buffer[12 + 0x81] = ptr[1];
		buffer[12 + 0x82] = ptr[2];
		buffer[12 + 0x83] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x80] = ptr[3];
		buffer[12 + 0x81] = ptr[2];
		buffer[12 + 0x82] = ptr[1];
		buffer[12 + 0x83] = ptr[0];
	}

	// 4-byte floating point: AUTO_SIZE_KP
	fValue = obj["AUTO_SIZE_KP"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x84] = ptr[0];
		buffer[12 + 0x85] = ptr[1];
		buffer[12 + 0x86] = ptr[2];
		buffer[12 + 0x87] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x84] = ptr[3];
		buffer[12 + 0x85] = ptr[2];
		buffer[12 + 0x86] = ptr[1];
		buffer[12 + 0x87] = ptr[0];
	}

	// 4-byte floating point: AUTO_SIDE_MAX
	fValue = obj["AUTO_SIDE_MAX"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x88] = ptr[0];
		buffer[12 + 0x89] = ptr[1];
		buffer[12 + 0x8A] = ptr[2];
		buffer[12 + 0x8B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x88] = ptr[3];
		buffer[12 + 0x89] = ptr[2];
		buffer[12 + 0x8A] = ptr[1];
		buffer[12 + 0x8B] = ptr[0];
	}

	// 4-byte floating point: AUTO_SIDE_MIN
	fValue = obj["AUTO_SIDE_MIN"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x8C] = ptr[0];
		buffer[12 + 0x8D] = ptr[1];
		buffer[12 + 0x8E] = ptr[2];
		buffer[12 + 0x8F] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x8C] = ptr[3];
		buffer[12 + 0x8D] = ptr[2];
		buffer[12 + 0x8E] = ptr[1];
		buffer[12 + 0x8F] = ptr[0];
	}

	// 4-byte floating point: KP_N
	fValue = obj["KP_N"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x90] = ptr[0];
		buffer[12 + 0x91] = ptr[1];
		buffer[12 + 0x92] = ptr[2];
		buffer[12 + 0x93] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x90] = ptr[3];
		buffer[12 + 0x91] = ptr[2];
		buffer[12 + 0x92] = ptr[1];
		buffer[12 + 0x93] = ptr[0];
	}

	// 4-byte floating point: IR_DC
	fValue = obj["IR_DC"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x94] = ptr[0];
		buffer[12 + 0x95] = ptr[1];
		buffer[12 + 0x96] = ptr[2];
		buffer[12 + 0x97] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x94] = ptr[3];
		buffer[12 + 0x95] = ptr[2];
		buffer[12 + 0x96] = ptr[1];
		buffer[12 + 0x97] = ptr[0];
	}

	// 4-byte floating point: DA_LIM_P_V
	fValue = obj["DA_LIM_P_V"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x98] = ptr[0];
		buffer[12 + 0x99] = ptr[1];
		buffer[12 + 0x9A] = ptr[2];
		buffer[12 + 0x9B] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x98] = ptr[3];
		buffer[12 + 0x99] = ptr[2];
		buffer[12 + 0x9A] = ptr[1];
		buffer[12 + 0x9B] = ptr[0];
	}

	// 4-byte floating point: DA_LIM_N_V
	fValue = obj["DA_LIM_N_V"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x9C] = ptr[0];
		buffer[12 + 0x9D] = ptr[1];
		buffer[12 + 0x9E] = ptr[2];
		buffer[12 + 0x9F] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0x9C] = ptr[3];
		buffer[12 + 0x9D] = ptr[2];
		buffer[12 + 0x9E] = ptr[1];
		buffer[12 + 0x9F] = ptr[0];
	}


	fValue = obj["LOCK_RATE"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xA0] = ptr[0];
		buffer[12 + 0xA1] = ptr[1];
		buffer[12 + 0xA2] = ptr[2];
		buffer[12 + 0xA3] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xA0] = ptr[3];
		buffer[12 + 0xA1] = ptr[2];
		buffer[12 + 0xA2] = ptr[1];
		buffer[12 + 0xA3] = ptr[0];
	}

	// 4-byte floating point: PULSE_RATE
	fValue = obj["PULSE_RATE"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xA4] = ptr[0];
		buffer[12 + 0xA5] = ptr[1];
		buffer[12 + 0xA6] = ptr[2];
		buffer[12 + 0xA7] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xA4] = ptr[3];
		buffer[12 + 0xA5] = ptr[2];
		buffer[12 + 0xA6] = ptr[1];
		buffer[12 + 0xA7] = ptr[0];
	}

	// 4-byte floating point: HALF_WAVE_RATIO
	fValue = obj["HALF_WAVE_RATIO"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xA8] = ptr[0];
		buffer[12 + 0xA9] = ptr[1];
		buffer[12 + 0xAA] = ptr[2];
		buffer[12 + 0xAB] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xA8] = ptr[3];
		buffer[12 + 0xA9] = ptr[2];
		buffer[12 + 0xAA] = ptr[1];
		buffer[12 + 0xAB] = ptr[0];
	}

	// 4-byte floating point: MDR2_IR_F
	fValue = obj["MDR2_IR_F"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xAC] = ptr[0];
		buffer[12 + 0xAD] = ptr[1];
		buffer[12 + 0xAE] = ptr[2];
		buffer[12 + 0xAF] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xAC] = ptr[3];
		buffer[12 + 0xAD] = ptr[2];
		buffer[12 + 0xAE] = ptr[1];
		buffer[12 + 0xAF] = ptr[0];
	}

	// 4-byte floating point: MDR2_KI
	fValue = obj["MDR2_KI"].toDouble();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xB0] = ptr[0];
		buffer[12 + 0xB1] = ptr[1];
		buffer[12 + 0xB2] = ptr[2];
		buffer[12 + 0xB3] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&fValue);
		buffer[12 + 0xB0] = ptr[3];
		buffer[12 + 0xB1] = ptr[2];
		buffer[12 + 0xB2] = ptr[1];
		buffer[12 + 0xB3] = ptr[0];
	}

	// 4-byte integer: SVON_HOLD_TIME
	int32_t iValue = obj["SVON_HOLD_TIME"].toInt();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&iValue);
		buffer[12 + 0xB4] = ptr[0];
		buffer[12 + 0xB5] = ptr[1];
		buffer[12 + 0xB6] = ptr[2];
		buffer[12 + 0xB7] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&iValue);
		buffer[12 + 0xB4] = ptr[3];
		buffer[12 + 0xB5] = ptr[2];
		buffer[12 + 0xB6] = ptr[1];
		buffer[12 + 0xB7] = ptr[0];
	}

	// 4-byte integer: RUN_OUT_LIM_OFF
	iValue = obj["RUN_OUT_LIM_OFF"].toInt();
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&iValue);
		buffer[12 + 0xB8] = ptr[0];
		buffer[12 + 0xB9] = ptr[1];
		buffer[12 + 0xBA] = ptr[2];
		buffer[12 + 0xBB] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&iValue);
		buffer[12 + 0xB8] = ptr[3];
		buffer[12 + 0xB9] = ptr[2];
		buffer[12 + 0xBA] = ptr[1];
		buffer[12 + 0xBB] = ptr[0];
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
	recvObj["status"] = "success";

	return true;
}
