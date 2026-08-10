#pragma once

#include "MsgHandler.h"
#include <qsqlquery.h>
#include <qsqldatabase.h>

class MethodHandler : public MsgHandler
{
	Q_OBJECT

public:
	MethodHandler(QObject *parent);
	~MethodHandler();

	bool handleWsMsg(QJsonObject& obj, QString& response) override;

private:
	[[nodiscard]] bool addData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);
	[[nodiscard]] bool fetchData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);
	[[nodiscard]] bool deleteData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);
	[[nodiscard]] bool fetchDetail(const QSqlDatabase& db, const QJsonObject& obj, QString& response);

	//修改当前方法的参数
	[[nodiscard]] bool modifyData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);

	//获取系统中的串口列表
	[[nodiscard]] bool fetchSerialPorts(const QJsonObject& obj, QString& response);

	//更新当前方法的modbus串口号
	[[nodiscard]] bool updateModbusSerialPort(const QSqlDatabase& db, QJsonObject& recvObj, QString& response);
};
 