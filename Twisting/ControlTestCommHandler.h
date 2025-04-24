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
	bool transferMehod(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool startTest(CommunicationThread* socket, QJsonObject& obj, QString& err);
	bool stopTest(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool home(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool stop(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool spin(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool reSpin(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//归零
	bool zero(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入X DIR
	bool setXDIR(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入YZ DIR
	bool setYZDIR(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入 XGAIN
	bool setXGAIN(CommunicationThread* socket, QJsonObject& obj, QString& err);
	//写入YZ GAIN
	bool setYZGAIN(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool setADGAIN(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入采样率
	bool setSamplingRate(CommunicationThread* socket, QJsonObject& obj, QString& err);
private:

	//单个写入DF SET
	bool perSetDfSet(CommunicationThread* socket, DF_SET& df_set, QString& err);
};
