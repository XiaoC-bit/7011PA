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

	this->setWindowIcon(QIcon(":/Evolution/res/logo.ico"));
    this->setWindowTitle("Twisting");
    int minWidth = 1366;
    int minHeigth = 768;

	//启动前端通讯 websocket
	wsDispatcher_ = (new WsMsgDispatcher(gConfig.webSocketPort, true));
	wsThread_ = new QThread(this);
	wsDispatcher_->moveToThread(wsThread_);//放到后台线程，避免阻塞UI线程
	wsThread_->start();

	if (!gDebug) {
		QCefSetting setting;
		QString uri = QString("http://localhost:%1").arg(gConfig.frontPort);
		cefViewWidget_ = std::unique_ptr< QCefView>(new CefViewWidget(uri, &setting, this));
		this->setCentralWidget(cefViewWidget_.get());
		this->setMinimumWidth(minWidth);
		this->setMinimumHeight(minHeigth);
	}
	else {
		this->setMinimumWidth(10);
		this->setMinimumHeight(10);
		////测试使用
		QPushButton* button = new QPushButton("start");
		//QPushButton* button1 = new QPushButton("stop");
		//QPushButton* button2 = new QPushButton("send");
		//// 创建垂直布局
		QVBoxLayout* layout = new QVBoxLayout;
		//// 将按钮添加到布局中
		layout->addWidget(button);
		//layout->addWidget(button1);
		//layout->addWidget(button2);
		//// 创建一个 QWidget 作为布局的容器
		QWidget* widget = new QWidget;
		//// 将布局设置给 QWidget
		widget->setLayout(layout);
		//// 将 QWidget 设置为 QMainWindow 的中央窗口部件
		setCentralWidget(widget);
		//// 连接按钮的 clicked 信号到槽函数
	}

}

Twisting::~Twisting()
{
	cefViewWidget_ = nullptr;
}
