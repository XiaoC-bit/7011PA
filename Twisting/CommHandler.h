#pragma once

#include <QObject>
#include <qvector.h>
#include <qjsonobject.h>
#include "CommunicationThread.h"
class CommHandler  : public QObject
{
	Q_OBJECT

public:
	CommHandler(ReadU65Struct &ref,QObject *parent);
	~CommHandler();

	[[nodiscard]] virtual bool commFunc(CommunicationThread*socket,QJsonObject& obj, QString &err) = 0;

protected:

	bool readInt16(CommunicationThread* socket, int addr, int16_t& value, QString& err);
	bool readInt32(CommunicationThread* socket, int addr, int32_t& value, QString& err);

	bool writeInt16(CommunicationThread* socket, int addr, int16_t value, QString& err);
	bool writeInt32(CommunicationThread* socket, int addr, int32_t value, QString& err);


	bool writeFloat(CommunicationThread* socket, int addr, float value, QString& err);
	/*
	if (machType == "sulfur change instrument") {

	}
	else if (machType == "mooney machine") {

	}
	else if (machType == "variable speed mooney machine") {

	}
	*/

	enum MACH_TYPE {
		Unknow = 0,
		Sulfur = 1,//硫化
		Mooney,//门尼
		Speed_Mooney
	};
	MACH_TYPE machineType() const;

	friend class CommunicationThread;

	ReadU65Struct &U65Info_;

	void packPC_KEY(int pc_key, QByteArray& buffer);

	void packPC_KEY(int pc_key, int pc_addr, QByteArray& buffer);


	void packPC_KEY(int pc_key, int pc_addr, int16_t data, QByteArray& buffer);
	void packPC_KEY(int pc_key, int pc_addr, int32_t data, QByteArray& buffer);
	void packPC_KEY(int pc_key, int pc_addr, float data, QByteArray& buffer);
	/**
	 * 判断大小端，小端返回true.
	 * 
	 * @return 
	 */
	bool isLitteEndian();
	/**
	 * 计算校验和.
	 * 
	 * @param buffer
	 */
	void calcSum(QByteArray& buffer);

	/**
	 * 计算校验和是否正确，用于校验设备响应报文的合法性.
	 * 
	 * @param buffer
	 * @return 
	 */
	bool checkSum(QByteArray& buffer);

	enum E_Mode{
		Read,
		Write
	};

	/**
	 * 设置命令.
	 * 
	 * @param mode
	 * @param buffer
	 */
	void setCmd(E_Mode mode, QByteArray& buffer);

	/**
	 * 设置读写的长度.
	 * 
	 * @param 
	 * @param buffer
	 */
	void setLength(int16_t, QByteArray& buffer);

	/**
	 * 设置读写的地址.
	 * 
	 * @param addr
	 * @param buffer
	 */
	void setAddr(int32_t addr, QByteArray& buffer);
};
