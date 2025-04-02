#pragma once

#include <QObject>

#include "CommHandler.h"

class PIDCommHandler  : public CommHandler
{
	Q_OBJECT

public:
	PIDCommHandler(ReadU65Struct& ref, QObject *parent);
	~PIDCommHandler();

	[[nodiscard]] bool commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err)  override;

private:

	//读取数据
	bool readData(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入数据
	bool writeData(CommunicationThread* socket, QJsonObject& obj, QString& err);


private:
};
