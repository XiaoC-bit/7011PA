#pragma once

#include <qthread.h>
#include <qvector.h>

#include "MsgHandler.h"
#include "communicationthread.h"

class DataProcessor;
/**
 * 用于处理多设备消息，主要用于统一接收信号，分发到不同的槽函数.
 */
class MultiMachDispatcher  : public MsgHandler
{
	Q_OBJECT


public:
	MultiMachDispatcher(QObject *parent = nullptr);
	~MultiMachDispatcher();

	typedef struct {
		QString ip;
		bool enable;
	}DEVICE_INFO;

public slots:
	void handleWSMsg(const QJsonObject& obj);
	void handleWSMachReq(const QJsonObject& obj);
	
	void fileSelect(QString& str);
	//转发RESTFUL消息
	void handleRestFulMsg(const QJsonObject& recvObj, QJsonObject& obj);
private:
	int getDeviceId();
	QVector< DEVICE_INFO> devices();

signals:
	void forwardToDataProcessor1(const QJsonObject& obj);
	void forwardToDataProcessor2(const QJsonObject& obj);
	void forwardToDataProcessor3(const QJsonObject& obj);
	void forwardToDataProcessor4(const QJsonObject& obj);


	void forwardToMach1(const QJsonObject& obj);
	void forwardToMach2(const QJsonObject& obj);
	void forwardToMach3(const QJsonObject& obj);
	void forwardToMach4(const QJsonObject& obj);


	//回送数据消息给WS线程
	void fireEvent(const QString& msg, const QString& ip = "");
	//回送设备消息给WS线程
	void wsResponse(const QJsonObject& obj);
	//转发RESTFUL消息
	void forwardRestFulMsg1(const QJsonObject& recvObj, QJsonObject& obj);
	void forwardRestFulMsg2(const QJsonObject& recvObj, QJsonObject& obj);
	void forwardRestFulMsg3(const QJsonObject& recvObj, QJsonObject& obj);
	void forwardRestFulMsg4(const QJsonObject& recvObj, QJsonObject& obj);
private:
	DataProcessor* dataProcessor_[4];
	QThread* processThread_[4];
	CommunicationThread machThread[4];//设备通讯模块（线程封装在模块中）
};
