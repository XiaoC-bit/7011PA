#pragma once

#include <QObject>
#include <vector>
#include "DataDefine.h"

#include "CommHandler.h"
#include "TemiSp3Modbus.h"

class ControlTestCommHandler  : public CommHandler
{
	Q_OBJECT

public:
	ControlTestCommHandler(ReadU65Struct& ref, std::vector<TestingRawData>&datas, QObject *parent);
	~ControlTestCommHandler();

	[[nodiscard]] bool commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err)  override;

	std::vector<TestingRawData> &testingRawDatas_;
private:
	bool transferMehod(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool startTest(CommunicationThread* socket, QJsonObject& obj, QString& err);
	bool stopTest(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool home(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool stop(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool spin(CommunicationThread* socket, QJsonObject& obj, QString& err);

	bool reSpin(CommunicationThread* socket, QJsonObject& obj, QString& err);

	
	//启动测试前的准备：循环回读角度并 moveup/movedown，直至接近设定角度
	bool prepareTest(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//夹持/松开
	bool grip(CommunicationThread* socket, QJsonObject& obj, QString& err);
	bool release(CommunicationThread* socket, QJsonObject& obj, QString& err);


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
	bool setADCAP(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入采样率
	bool setSamplingRate(CommunicationThread* socket, QJsonObject& obj, QString& err);

	//写入DIR_FLAG
	bool setAD1_DIR(CommunicationThread* socket, QJsonObject& obj, QString& err);
	bool setAD2_DIR(CommunicationThread* socket, QJsonObject& obj, QString& err);
	bool setAD1_UPDN(CommunicationThread* socket, QJsonObject& obj, QString& err);
	bool setAD2_UPDN(CommunicationThread* socket, QJsonObject& obj, QString& err);
private:

	bool readData(CommunicationThread* socket, QString& err);

	

	TestingRawData testingRawdata_;

	//单个写入DF SET
	bool perSetDfSet(CommunicationThread* socket, DF_SET& df_set, QString& err);

};
