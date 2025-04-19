#pragma once

#include <QObject>
#include <qjsonobject.h>
#include <qsqldatabase.h>
class MsgHandler  : public QObject
{
	Q_OBJECT

public:
	MsgHandler(QObject *parent);
	~MsgHandler();
	
	void setDeviceId(int id);

	/**
	 * 获取配置数据库实例.
	 * 
	 * @param db
	 * @return 
	 */
	[[nodiscard]] bool getConfigDB(QSqlDatabase& db);


	/**
	 * 根据专案名称、方法名称，获取测试数据的数据库实例
	 * 
	 * @param project
	 * @param method
	 * @param db
	 * @return 
	 */
	[[nodiscard]] bool getTestDataDB(const QString &project,const QString &method,QSqlDatabase& db);


	bool getTestDataDB(QSqlDatabase& db);


	bool getTestDataDB(QSqlDatabase& db,int deviceId);

	/**
	 * 对前端请求进行响应处理.
	 * 
	 * @param obj
	 * @param response  发送回前端的数据包，长度为空则不发送
	 * @return 
	 */
	[[nodiscard]] virtual bool handleWsMsg(QJsonObject& obj, QString& response) {
		return false;
	};

	/**
	 * 接收WS的消息，判断是否需要转发给设备通讯线程.
	 * 一些消息需要通过Processor中转处理
	 * 
	 * @param obj
	 * @return 
	 */
	[[nodiscard]] virtual bool isTransfer(const QJsonObject& obj) {
		return false;
	}

signals:
	void fileSelect(QString& str);

	void useRealTime(bool use);
protected:
	QString channel_;

	int deviceId_;//设备编号，用于实现一拖多台设备的功能
};
