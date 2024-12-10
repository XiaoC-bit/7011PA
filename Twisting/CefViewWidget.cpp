#include "CefViewWidget.h"

#include "DownloadManager.h"

CefViewWidget::CefViewWidget(const QString url, const QCefSetting* setting, QWidget* parent /* = 0*/)
: QCefView(url, setting, parent)
{}

CefViewWidget::~CefViewWidget()
{}

void
CefViewWidget::onNewDownloadItem(const QSharedPointer<QCefDownloadItem>& item, const QString& suggestedName)
{
	// keep the item into list or map, and call item->start() to allow the download

	DownloadManager::getInstance().AddNewDownloadItem(item);
}

void
CefViewWidget::onUpdateDownloadItem(const QSharedPointer<QCefDownloadItem>& item)
{

	// control the download by invoking item->pause(), item->resume(), item->cancel()

	DownloadManager::getInstance().UpdateDownloadItem(item);
}