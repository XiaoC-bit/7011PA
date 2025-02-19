#pragma once

#include <QObject>

#include "CommHandler.h"

class ControlTestCommHandler  : public CommHandler
{
	Q_OBJECT

public:
	ControlTestCommHandler(ReadU65Struct& ref, QObject *parent);
	~ControlTestCommHandler();

	[[nodiscard]] bool commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err)  override;

private:
	bool startTest(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool home(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool stop(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool spin(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool reSpin(CommunicationThread* socket, QJsonObject& obj, QString& err);
};
