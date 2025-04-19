#include "Twisting.h"
#include <qlayout.h>
#include <QPushButton>
#include <qthread.h>


#include "CefViewWidget.h"
#include "DataDefine.h"
#include "wsmsgdispatcher.h"

extern bool gDebug;
extern Config gConfig;

Twisting::Twisting(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	this->setWindowIcon(QIcon(":/Twisting/res/logo.ico"));
    this->setWindowTitle("Twisting");
    int minWidth = 1366;
    int minHeigth = 768;

	//启动前端通讯 websocket
	wsDispatcher_ = (new WsMsgDispatcher(gConfig.webSocketPort, true));
	wsThread_ = new QThread(this);
	wsDispatcher_->moveToThread(wsThread_);//放到后台线程，避免阻塞UI线程
	wsThread_->start();

	signalInit();

	if (gDebug) {
		QCefSetting setting;
		//QString uri = QString("http://localhost:%1").arg(gConfig.frontPort);
		QString uri = QString("https://www.baidu.com").arg(gConfig.frontPort);
		cefViewWidget_ = std::unique_ptr< QCefView>(new CefViewWidget(uri, &setting, this));
		this->setCentralWidget(cefViewWidget_.get());
		this->setMinimumWidth(10);
		this->setMinimumHeight(10);
	}
	else {
		QCefSetting setting;
		QString uri = QString("http://localhost:%1").arg(gConfig.frontPort);
		cefViewWidget_ = std::unique_ptr< QCefView>(new CefViewWidget(uri, &setting, this));
		this->setCentralWidget(cefViewWidget_.get());
		this->setMinimumWidth(minWidth);
		this->setMinimumHeight(minHeigth);
	}

}

Twisting::~Twisting()
{
	cefViewWidget_ = nullptr;
	wsDispatcher_->deleteLater();
	if (wsThread_) {
		wsThread_->quit();
		wsThread_->deleteLater();
	}
}


void Twisting::signalInit(){

	//WS消息，关联至设备通讯线程
	connect(wsDispatcher_, &WsMsgDispatcher::requestMach, &agent_, &MultiMachDispatcher::handleWSMachReq);
	connect(&agent_, &MultiMachDispatcher::wsResponse, wsDispatcher_, &WsMsgDispatcher::responseToWs);



	//WS消息，关联至数据处理器
	connect(wsDispatcher_, &WsMsgDispatcher::requestProcessor, &agent_, &MultiMachDispatcher::handleWSMsg);
	//数据处理器处理后，回送消息到WS发送出去
	connect(&agent_, &MultiMachDispatcher::fireEvent, wsDispatcher_, &WsMsgDispatcher::fireEvent);
}
