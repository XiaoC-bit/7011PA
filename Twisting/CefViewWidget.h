#pragma once

#include <QCefView.h>

class CefViewWidget  : public QCefView
{
	Q_OBJECT

public:
	CefViewWidget(const QString url, const QCefSetting* setting, QWidget* parent = 0);
	~CefViewWidget();

protected:
	void onNewDownloadItem(const QSharedPointer<QCefDownloadItem>& item, const QString& suggestedName) override;
	void onUpdateDownloadItem(const QSharedPointer<QCefDownloadItem>& item) override;

};
