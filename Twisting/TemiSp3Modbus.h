#ifndef TEMISP3MODBUS_H
#define TEMISP3MODBUS_H

#include <QObject>
#include <QModbusRtuSerialMaster>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QSerialPort>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>

// TEMI SP3 位移/测长传感器 Modbus RTU 通讯封装
// 依据《TEMI_SP3功能说明》：
//   - 读速度：功能码 03（读保持寄存器），地址 1，返回值/100 = m/s
//   - 设置长度：功能码 06（写单个寄存器），地址 1，写入值/10 = mm
class TemiSp3Modbus : public QObject
{
    Q_OBJECT

public:
    explicit TemiSp3Modbus(QObject* parent = nullptr)
        : QObject(parent)
    {
        m_modbus = new QModbusRtuSerialMaster(this);

        connect(m_modbus, &QModbusClient::errorOccurred, this, [this](QModbusDevice::Error error) {
            if (error != QModbusDevice::NoError)
                qWarning() << "[Modbus] error:" << m_modbus->errorString();
            });

        connect(m_modbus, &QModbusClient::stateChanged, this, [](QModbusDevice::State state) {
            qDebug() << "[Modbus] state changed:" << state;
            });
    }

    ~TemiSp3Modbus()
    {
        if (m_modbus->state() != QModbusDevice::UnconnectedState)
            m_modbus->disconnectDevice();
    }

    // portName 例如 Windows: "COM3"，Linux: "/dev/ttyUSB0"
    // baudRate 需与设备拨码开关 1、2 脚设置一致（9600/19200/2400/4800）
    bool open(const QString& portName, int baudRate = 9600)
    {
        m_modbus->setConnectionParameter(QModbusDevice::SerialPortNameParameter, portName);
        m_modbus->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
        m_modbus->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudRate);
        m_modbus->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        m_modbus->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);

        m_modbus->setTimeout(1000);      // 超时 1s
        m_modbus->setNumberOfRetries(3); // 重试3次

        if (!m_modbus->connectDevice()) {
            qWarning() << "连接失败:" << m_modbus->errorString();
            return false;
        }
        return true;
    }

    void close()
    {
        m_modbus->disconnectDevice();
    }

    // 读速度：功能码03，地址1，从站地址 slaveAddr（对应设备5-8脚拨码，1~16）
    // 结果通过 speedRead(double speedMps) 信号返回
    void readSpeed(int slaveAddr)
    {
        QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, /*startAddr=*/1, /*count=*/1);

        if (auto* reply = m_modbus->sendReadRequest(readUnit, slaveAddr)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [this, reply]() {
                    if (reply->error() == QModbusDevice::NoError) {
                        const QModbusDataUnit unit = reply->result();
                        quint16 raw = unit.value(0);          // 例如 2345
                        double speed = raw / 100.0;           // => 23.45 m/s
                        emit speedRead(speed);
                    }
                    else {
                        qWarning() << "读速度失败:" << reply->errorString();
                        emit errorHappened(reply->errorString());
                    }
                    reply->deleteLater();
                    });
            }
            else {
                reply->deleteLater(); // 广播消息，立即完成，无需等待
            }
        }
        else {
            qWarning() << "发送读请求失败:" << m_modbus->errorString();
            emit errorHappened(m_modbus->errorString());
        }
    }

    // 设置长度：功能码06，地址1，单位0.1mm
    // 例如要设置 25.4mm，调用 setLength(slaveAddr, 25.4)
    void setLength(int slaveAddr, double lengthMm)
    {
        quint16 raw = static_cast<quint16>(qRound(lengthMm * 10)); // 25.4mm -> 254

        QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, /*startAddr=*/1, /*count=*/1);
        writeUnit.setValue(0, raw);

        if (auto* reply = m_modbus->sendWriteRequest(writeUnit, slaveAddr)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [this, reply]() {
                    if (reply->error() == QModbusDevice::NoError) {
                        emit lengthSetDone(true);
                    }
                    else {
                        qWarning() << "设置长度失败:" << reply->errorString();
                        emit lengthSetDone(false);
                        emit errorHappened(reply->errorString());
                    }
                    reply->deleteLater();
                    });
            }
            else {
                reply->deleteLater();
            }
        }
        else {
            qWarning() << "发送写请求失败:" << m_modbus->errorString();
            emit lengthSetDone(false);
            emit errorHappened(m_modbus->errorString());
        }
    }

    // 启动测试：功能码06，地址3，写入值1
    void startTest(int slaveAddr)
    {
        QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, /*startAddr=*/3, /*count=*/1);
        writeUnit.setValue(0, 1);

        if (auto* reply = m_modbus->sendWriteRequest(writeUnit, slaveAddr)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [this, reply]() {
                    if (reply->error() == QModbusDevice::NoError) {
                        emit startTestDone(true);
                    }
                    else {
                        qWarning() << "启动测试失败:" << reply->errorString();
                        emit startTestDone(false);
                        emit errorHappened(reply->errorString());
                    }
                    reply->deleteLater();
                    });
            }
            else {
                reply->deleteLater();
            }
        }
        else {
            qWarning() << "发送写请求失败:" << m_modbus->errorString();
            emit startTestDone(false);
            emit errorHappened(m_modbus->errorString());
        }
    }

    // ===== 同步（阻塞）版本 =====
    // 使用 QEventLoop 阻塞等待回复完成，嵌套事件循环会继续处理串口 I/O。
    // 注意：在主线程调用会阻塞 UI，适合在启动阶段或后台线程使用。

    // 启动测试（同步），返回是否成功
    bool startTestSync(int slaveAddr)
    {
        QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, /*startAddr=*/3, /*count=*/1);
        writeUnit.setValue(0, 1);

        bool ok = false;
        QEventLoop loop;
        if (auto* reply = m_modbus->sendWriteRequest(writeUnit, slaveAddr)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [this, reply, &ok, &loop]() {
                    ok = (reply->error() == QModbusDevice::NoError);
                    if (!ok) {
                        qWarning() << "启动测试失败:" << reply->errorString();
                        emit errorHappened(reply->errorString());
                    }
                    reply->deleteLater();
                    loop.quit();
                    });
                loop.exec();
            }
            else {
                reply->deleteLater();
            }
        }
        else {
            qWarning() << "发送写请求失败:" << m_modbus->errorString();
            emit errorHappened(m_modbus->errorString());
        }
        return ok;
    }

    // 读速度（同步），ok 指针用于接收是否成功
    double readSpeedSync(int slaveAddr, bool* ok = nullptr)
    {
        QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, /*startAddr=*/1, /*count=*/1);

        double speed = 0.0;
        bool success = false;
        QEventLoop loop;
        if (auto* reply = m_modbus->sendReadRequest(readUnit, slaveAddr)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [this, reply, &speed, &success, &loop]() {
                    if (reply->error() == QModbusDevice::NoError) {
                        const QModbusDataUnit unit = reply->result();
                        quint16 raw = unit.value(0);
                        speed = raw / 100.0;
                        success = true;
                    }
                    else {
                        qWarning() << "读速度失败:" << reply->errorString();
                        emit errorHappened(reply->errorString());
                    }
                    reply->deleteLater();
                    loop.quit();
                    });
                loop.exec();
            }
            else {
                reply->deleteLater();
            }
        }
        else {
            qWarning() << "发送读请求失败:" << m_modbus->errorString();
            emit errorHappened(m_modbus->errorString());
        }
        if (ok) *ok = success;
        return speed;
    }

signals:
    void speedRead(double speedMps);
    void lengthSetDone(bool ok);
    void startTestDone(bool ok);
    void errorHappened(const QString& msg);

private:
    QModbusRtuSerialMaster* m_modbus = nullptr;
};

#endif // TEMISP3MODBUS_H