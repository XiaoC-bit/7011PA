#pragma once

#include <QObject>
#include <qmap.h>
#include <memory>
#include <qtimer.h>
#include <chrono>

#include "MsgHandler.h"
#include "DataDefine.h"

class QJsonObject;
class DataProcessor  : public QObject
{
	Q_OBJECT

public:
	DataProcessor(QObject *parent,int deviceId);
	~DataProcessor();

signals:
	void useRealTime(bool use);

	//回送消息给WS线程
	void fireEvent(const QString& msg, const QString& ip = "");

	//文件选择器
	void fileSelect(QString& str);
	/**
	 * 转发指令给设备通讯模块
	 *
	 * @param obj
	 */
	void requestMach(const QJsonObject& obj);
public slots:
	//处理WebSocket客户端的请求数据
	void handleWSMsg(const QJsonObject& obj);

	/**
	 * 从设备线程发送过来的数据，需要进行数据处理，随后保存到数据库.
	 * 
	 * @param data
	 */
	void handleRegularInfo(const QVariant& data);

private:

	bool transferMethodFunc(const int id);

	/**
	 * 辅助函数.
	 * 
	 * @param data
	 */
	void  handleRegularInfoFunc(const U65RawData& info);

	/**
	 * 上一个状态是测试中.
	 * 
	 * @param info
	 */
	void handleTesting(const U65RawData& info);


	/**
	 * 开始下一条测试队列
	 *
	 * @param info
	 */
	void beginTestQueue(const U65RawData& info);

	/**
	 * 尝试优化送料延时.
	 * 
	 * \param info
	 */
	void endTestQueueFunc(const U65RawData& info);

	/**
	 * 结束一条测试队列.
	 */
	void endTestQueue(const U65RawData& info);

	/**
	 * 判断是否需要输送下一个料.
	 * 
	 * @param info
	 * @return 
	 */
	bool checkAutoFeeding(const U65RawData& info);
	/**
	* 
	 * 通知设备线程,输送下一个料.
	 * 
	 */
	void sendNextItem();

	/**
	 * 通知设备线程,停止测试.
	 * 
	 */
	void stopTesting();

	/**
	 * 结算当前测试队列.
	 * 
	 */
	void sumupQueue();

	/**
	 * 记录原始数据.
	 * 
	 * @param info
	 */
	void recordDetail(const U65RawData& info);

	//开始内部测试(非队列模式)
	bool beginInternalTest();


	/**
	 * 获取下一个测试队列.
	 * 
	 * \param id
	 * \return 
	 */
	bool getNextQueue(int& id);

	/**
	 * 获取测试数据库实例.
	 * 
	 * @param db
	 * @return 
	 */
	bool getTestDataDB(QSqlDatabase& db);

	/**
	 * 获取配置数据库实例.
	 * 
	 * @param db
	 * @return 
	 */
	bool getConfigDb(QSqlDatabase& db);


	/**
	 * 测试数据点写入测试数据库
	 * 
	 * @param testDb
	 * @param items
	 * @return 
	 */
	bool recordItems(QSqlDatabase& testDb, const QVector<QPair<QString, QString>> & items,const QMap<QString, QPair<double, double>>&, bool &result);





private slots:
	void queueFunc();

private:
	QMap<QString, std::shared_ptr< MsgHandler>> handlers_;
	bool lastTesting_;// 上一次循环是否处于测试状态
	bool curTesting_;//本次循环是否处于测试状态
	enum DOOR_STATUS {
		OPEN,
		CLOSE
	};
	DOOR_STATUS lastDoorStatus_;// 上一次循环，门的状态
	DOOR_STATUS curDoorStatus_;// 当前循环，门的状态

	enum MOLD_STATUS {
		UP,
		DOWN,
		INIT
	};
	MOLD_STATUS lastMoldStatus_;// 上一次循环，上下模的状态
	MOLD_STATUS curMoldStatus_;// 当前循环，上下模的状态


	int queueId_;//当前正在进行的测试队列ID
	float startTime_;//当前正在测试的胶料，开始测试的时间戳
	float lastTime_;

	float step1StartTime_;
	float step2StartTime_;
	int lastStep_;

	unsigned __int64 lastAccountTime_;
	int accountTime_;

	/**
	 * 数据是否正常
	 * 异常情况如下:
	 * 1.胶料开始测试,内部模式,但是数据库内无有效的测试队列(没有下发方法)
	 * 2.胶料开始测试,外部模式,但是MES数据库无有效的测试队列(MES数据异常)
	 */
	bool flag_;

	/**
	 * 以下是外部测试队列相关的成员变量.
	 */
	bool usingQueue_;//是否使用队列测试
	int externalQueueId_;//外部队列的当前ID
	float externalTemp_;//外部队列传递的设置温度
	int externalEndTime_;//外部队列传递的测试时间
	QTimer* queueTimer_;//外部测试队列处理定时器

	int deviceId_;//设备ID，为了实现一拖多的功能


	/**
	 * 队列测试数据，用于快捷计算，暂时用于门尼机取平均值
	 */
	QVector<float> details_;
	double CVmd_Media_Mooney[10];
	const int CVmi_MediaCount = 5;


	std::chrono::steady_clock::time_point startTestingTime;



	DOOR_STATUS checklastDoorStatus_;// 上一次循环，门的状态
	DOOR_STATUS checkcurDoorStatus_;// 当前循环，门的状态
	MOLD_STATUS checklastMoldStatus_;// 上一次循环，上下模的状态
	MOLD_STATUS checkcurMoldStatus_;// 当前循环，上下模的状态
};
