#include "CommHandler.h"

CommHandler::CommHandler(ReadU65Struct& ref, QObject* parent)
	: QObject(parent), U65Info_(ref)
{}

CommHandler::~CommHandler()
{}


//ReadU65Struct CommHandler::U65Info_;

void CommHandler::packPC_KEY(int pc_key, QByteArray& buffer) {
	buffer.resize(528);
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(2, buffer);
	setAddr(0x0902, buffer);
	buffer[12] = int8_t((pc_key & 0x000000FF));
	buffer[13] = int8_t((pc_key & 0x0000FF00) >> 8);
	calcSum(buffer);
}


void CommHandler::packPC_KEY(int pc_key, int pc_addr, QByteArray& buffer) {
	buffer.resize(528);
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(4, buffer);
	setAddr(0x0902, buffer);
	buffer[12] = int8_t((pc_key & 0x000000FF));
	buffer[13] = int8_t((pc_key & 0x0000FF00) >> 8);

	buffer[14] = int8_t((pc_addr & 0x000000FF));
	buffer[15] = int8_t((pc_addr & 0x0000FF00) >> 8);

	calcSum(buffer);
}


void CommHandler::packPC_KEY(int pc_key, int pc_addr, int16_t data, QByteArray& buffer) {
	buffer.resize(528);
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(8, buffer);
	setAddr(0x0902, buffer);
	buffer[12] = int8_t((pc_key & 0x000000FF));
	buffer[13] = int8_t((pc_key & 0x0000FF00) >> 8);

	buffer[14] = int8_t((pc_addr & 0x000000FF));
	buffer[15] = int8_t((pc_addr & 0x0000FF00) >> 8);


	buffer[16] = 0x02;
	buffer[17] = 0x00;

	buffer[18] = int8_t((data & 0x000000FF));
	buffer[19] = int8_t((data & 0x0000FF00) >> 8);

	calcSum(buffer);
}


void CommHandler::packPC_KEY(int pc_key, int pc_addr, float data, QByteArray& buffer) {
	buffer.resize(528);
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(10, buffer);
	setAddr(0x0902, buffer);
	buffer[12] = int8_t((pc_key & 0x000000FF));
	buffer[13] = int8_t((pc_key & 0x0000FF00) >> 8);

	buffer[14] = int8_t((pc_addr & 0x000000FF));
	buffer[15] = int8_t((pc_addr & 0x0000FF00) >> 8);


	buffer[16] = 0x04;
	buffer[17] = 0x00;

	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&data);
		buffer[18] = ptr[0];
		buffer[19] = ptr[1];
		buffer[20] = ptr[2];
		buffer[21] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&data);
		buffer[18] = ptr[3];
		buffer[19] = ptr[2];
		buffer[20] = ptr[1];
		buffer[21] = ptr[0];
	}



	calcSum(buffer);
}

void CommHandler::packPC_KEY(int pc_key, int pc_addr, int32_t data, QByteArray& buffer) {
	buffer.resize(528);
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(10, buffer);
	setAddr(0x0902, buffer);
	buffer[12] = int8_t((pc_key & 0x000000FF));
	buffer[13] = int8_t((pc_key & 0x0000FF00) >> 8);

	buffer[14] = int8_t((pc_addr & 0x000000FF));
	buffer[15] = int8_t((pc_addr & 0x0000FF00) >> 8);


	buffer[16] = 0x04;
	buffer[17] = 0x00;



	buffer[18] = int8_t((data & 0x000000FF));
	buffer[19] = int8_t((data & 0x0000FF00) >> 8);
	buffer[20] = int8_t((data & 0x00FF0000) >> 16);
	buffer[21] = int8_t((data & 0xFF000000) >> 24);

	calcSum(buffer);
}

bool CommHandler::isLitteEndian() {
	union tmp
	{
		short a;
		char b[2];
	};
	tmp x;
	x.b[0] = 0x01;
	x.b[1] = 0x00;
	return x.a == 1;
}

long P32_SUM(unsigned char* data, int length)
{
	unsigned int SUM = 0;
	unsigned int temp1, temp2, temp3, temp4;
	temp1 = (data[5] << 8) & 0xFF00;
	temp2 = data[6] << 16;
	temp3 = data[7] << 24;
	for (int mi_temp = 4;mi_temp < 4 + length;mi_temp += 4)
	{
		temp1 = data[mi_temp];
		temp2 = (data[mi_temp + 1] << 8) & 0xFF00;
		temp3 = (data[mi_temp + 2] << 16) & 0xFF0000;
		temp4 = (data[mi_temp + 3] << 24) & 0xFF000000;
		SUM += temp1 + temp2 + temp3 + temp4;
	}
	return SUM;
}

/**
 * 计算校验和
 * 注意：QByteArray内部是char，不能直接用于校验和。要先转成unsigned char
 * 
 * @param data
 * @param length
 * @return 
 */
void CommHandler::calcSum(QByteArray & buffer)
{
	unsigned char data[528];
	memset(data, 0x00, 528);
	//控制器返回的数据，可能长度后面还有一堆的非0数据，不能以528长度计算校验和
	unsigned int length = (unsigned char)buffer[6] + (unsigned char)buffer[7] * 256;
	for(int i=0;i<528;i++)
		data[i] = static_cast<unsigned char>(buffer[i]);
	uint32_t ml_sum = 0;
	uint32_t temp1, temp2, temp3, temp4;
	temp1 = (data[5] << 8) & 0xFF00;
	temp2 = data[6] << 16;
	temp3 = data[7] << 24;
	for (unsigned int mi_temp = 4;mi_temp < length+12;mi_temp += 4)
	{
		temp1 = data[mi_temp];
		temp2 = (data[mi_temp + 1] << 8) & 0xFF00;
		temp3 = (data[mi_temp + 2] << 16) & 0xFF0000;
		temp4 = (data[mi_temp + 3] << 24) & 0xFF000000;
		ml_sum += temp1 + temp2 + temp3 + temp4;
	}
	data[0] = ml_sum & 0xFF;
	data[1] = (ml_sum & 0xFF00) >> 8;
	data[2] = (ml_sum & 0xFF0000) >> 16;
	data[3] = (ml_sum & 0xFF000000) >> 24;
	for (int i = 0;i < 4;i++)
		buffer[i] = data[i];
	return;
}


bool CommHandler::checkSum(QByteArray& buffer) {
	if (buffer.length() != 528)
		return false;	
	QByteArray tmp = buffer;
	calcSum(tmp);
	for (int i = 0;i < 4;i++) {
		if (tmp[i] != buffer[i])
			return false;
	}
	return true;
}

void CommHandler::setAddr(int32_t addr, QByteArray& buffer) {
	buffer[8] = int8_t((addr & 0x000000FF));
	buffer[9] = int8_t((addr & 0x0000FF00) >> 8);
	buffer[10] = int8_t((addr & 0x00FF0000) >> 16);
	buffer[11] = int8_t((addr & 0xFF000000) >> 24);
}


void CommHandler::setLength(int16_t length, QByteArray& buffer) {
	buffer[6] = int8_t((length & 0x00FF));
	buffer[7] = int8_t((length & 0xFF00) >> 8);
}


void CommHandler::setCmd(E_Mode mode, QByteArray& buffer) {
	if (mode == E_Mode::Read) {
		buffer[4] = 0x52;
		buffer[5] = 0x44;
	}
	else if (mode == E_Mode::Write) {
		buffer[4] = 0x57;
		buffer[5] = 0x52;
	}
}


bool CommHandler::writeFloat(CommunicationThread* socket, int addr, float value, QString& err) {
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(0x04, buffer);
	setAddr(addr, buffer);
	if (isLitteEndian()) {
		char* ptr = reinterpret_cast<char*>(&value);
		buffer[12] = ptr[0];
		buffer[13] = ptr[1];
		buffer[14] = ptr[2];
		buffer[15] = ptr[3];
	}
	else {
		char* ptr = reinterpret_cast<char*>(&value);
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
	//判断数据正确性
	return true;
}


bool CommHandler::readInt16(CommunicationThread* socket, int addr, int16_t& value, QString& err) {
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Read, buffer);
	setLength(0x02, buffer);
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
	//判断数据正确性
	if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0x02) {
		err = "readData error ,data error";
		return false;
	}
	value = (recvData[12] & 0xff) + (recvData[13] & 0xff) * 0x0100;
	return true;
}

bool CommHandler::readInt32(CommunicationThread* socket, int addr, int32_t& value, QString& err) {
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Read, buffer);
	setLength(0x04, buffer);
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
	//判断数据正确性
	if ((recvData[4] & 0xff) != 0x52 || (recvData[5] & 0xff) != 0x44 || (recvData[6] & 0xff) != 0x04) {
		err = "readData error ,data error";
		return false;
	}

	value = (recvData[12] & 0xff) +
		(recvData[13] & 0xff) * 0x0100 +
		(recvData[14] & 0xff) * 0x010000 +
		(recvData[15] & 0xff) * 0x01000000;
	return true;
}


bool CommHandler::writeInt16(CommunicationThread* socket, int addr, int16_t value, QString& err) {
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(0x02, buffer);
	setAddr(addr, buffer);
	buffer[12] = int8_t((value & 0x000000FF));
	buffer[13] = int8_t((value & 0x0000FF00) >> 8);
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


bool CommHandler::writeInt32(CommunicationThread* socket, int addr, int32_t value, QString& err) {
	QByteArray buffer;
	buffer.fill(0x00, 528);
	setCmd(E_Mode::Write, buffer);
	setLength(0x04, buffer);
	setAddr(addr, buffer);

	buffer[12] = int8_t((value & 0x000000FF));
	buffer[13] = int8_t((value & 0x0000FF00) >> 8);
	buffer[14] = int8_t((value & 0x00FF0000) >> 16);
	buffer[15] = int8_t((value & 0xFF000000) >> 24);

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


CommHandler::MACH_TYPE CommHandler::machineType() const{
	MACH_TYPE type = MACH_TYPE::Unknow;
	int mi_MechineType = U65Info_.CartDetail[0][0] + U65Info_.CartDetail[0][1] * 0x100;
	switch (mi_MechineType)
	{
	case 5000:
		if (('M' == U65Info_.CartDetail[0][3]) && ('V' == U65Info_.CartDetail[0][2]))
		{
			type = MACH_TYPE::Mooney; // 门尼
			if (U65Info_.Sys_Flag1 & 256)
			{
				type = MACH_TYPE::Speed_Mooney; // 可变速门尼
			}
		}
		else if (('M' == U65Info_.CartDetail[0][3]) && ('-' == U65Info_.CartDetail[0][2]))
		{
			type = MACH_TYPE::Sulfur; // 硫化
		}
		else
		{
			type = MACH_TYPE::Sulfur; // 硫化
		}
		break;
	case 5001:
		type = MACH_TYPE::Mooney; // 门尼
		if (U65Info_.Sys_Flag1 & 256)
		{
			type = MACH_TYPE::Speed_Mooney; // 可变速门尼
		}
		break;
	}
	return type;
}
