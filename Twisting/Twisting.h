#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Twisting.h"
#include <memory>

class QCefView;
class WsMsgDispatcher;
class Twisting : public QMainWindow
{
    Q_OBJECT

public:
    Twisting(QWidget *parent = nullptr);
    ~Twisting();

private:
    Ui::TwistingClass ui;

    /**
     * 浏览器.
     */
    std::unique_ptr < QCefView> cefViewWidget_ = nullptr;
    

    /**
     * WebSocket前端通讯模块.
     */
    WsMsgDispatcher* wsDispatcher_ = nullptr;

    /**
     * Websocket线程，避免阻塞UI主线程.
     */
    QThread* wsThread_ = nullptr;
};
