#pragma once

#include <QObject>

#include "CommHandler.h"

class NormalCommHandler  : public CommHandler
{
	Q_OBJECT

public:
	NormalCommHandler(ReadU65Struct& ref, QObject *parent);
	~NormalCommHandler();

	[[nodiscard]] bool commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err)  override;

private:
	//实时数据
	bool realData(CommunicationThread* socket, QJsonObject& obj, QString& err);
	//读取数据
	bool readData(CommunicationThread* socket, QJsonObject& obj, QString& err);
};
