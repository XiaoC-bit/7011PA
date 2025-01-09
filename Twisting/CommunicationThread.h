// communicationthread.h
#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <qmap.h>
#include <QTimer>
#include <qqueue.h>
#include <qmutex.h>
#include <QThread>
#include <QObject>
#include <QTcpSocket>

#include "DataDefine.h"

class CommHandler;
class CommunicationThread : public QThread
{
    Q_OBJECT

public:
    explicit CommunicationThread(QObject* parent = nullptr);
    ~CommunicationThread();

    void setDeviceId(int id);

    //连接至设备，并启动收发线程，开始通讯
    void init(const QString& deviceAddress, quint16 port);
    /**
     * 获取通讯线程负责的设备IP
     * 
     * @return 
     */
    QString ip();

    /**
     * 套接字写入函数
     *
     * @param data
     * @return
     */
    bool writeData(const QByteArray& data);

    bool readData(QByteArray& data,int timeout = 2000);

    QString socketError() {
        return m_socket->errorString();
    }
public slots:
    void handleWsRequest(const QJsonObject& obj);

    void useRealTime(bool use);
private:
    /**
     * 缓慢发送实时数据.
     * 
     * \param obj
     */
    void looseFireRealData(const QJsonObject& obj);

    unsigned __int64 lastSendRealData_;
    //处理通讯控制消息
    void handleCommCtrlMsg(const QJsonObject& obj);

    //更换ip地址
    void setDeviceAddress(const QString& ip);
    /**
     * 线程执行函数.
     *
     */
    void run() override;
    /**
     * 定时器执行函数.
     */
    void timerFunc();

    /**
     * 普通的定时器任务.
     * 需要不断与设备通讯的日常通讯任务
     * 
     */
    void normalTimerFunc();
   
    /**
     * 其他定时器任务，目前用于软件主动下发的通讯任务.
     * 
     */
    void otherTimerFunc();

    /**
     * 从资料区获取数据,欺骗数据处理线程.
     * 
     * \param info
     */
    void fakeData(const U65RawData&info);

    /**
     * 获取资料区数据.
     * 
     * \param info
     */
    bool readRecData(const U65RawData& info, U65RawData&fakeInfo,bool readRec2 = false);
signals:
    /**
     * 由主线程调用，通知线程启动定时器.
     */
    void startTimer();
    /**
   * 由主线程调用，通知线程停止定时器.
   */
    void stopTimer();

    void wsResponse(const QJsonObject& obj);

    void fireRegularInfo(const QVariant& data);
private:
    void log(const QString& str);
    void log(const QString& ip, const QString& str);
private:
    QMap<QString, CommHandler*> commHandlers_;//设备通讯协议处理器

    quint16 m_port;
    QString m_deviceAddress;
    QTcpSocket* m_socket;
    bool powerOn_;//是否需要跟设备通讯，如果用户点击了断开连接，就不通讯，默认是true

    QTimer* m_sendTimer;//数据发送定时器


    QQueue<QJsonObject> writeQueue_;//主动发送数据的队列，待发送状态
    QMutex writeMtx_;//保护发送队列

    QQueue<QJsonObject> commCtrlQueue_;//系统处理消息
    QMutex ctrlMtx_;//保护系统队列

    int recNum1_;//软件已经记录的数据
    int recNum2_;//软件已经记录的数据

    bool useRealTime_;//是否使用实时数据

    int deviceId_;

    ReadU65Struct u65Info;
};

#endif // COMMUNICATIONTHREAD_H
