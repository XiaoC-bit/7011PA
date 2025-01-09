#pragma once

#include "MsgHandler.h"
#include <qsqlquery.h>
#include <qsqldatabase.h>

class ReportSettingHandler : public MsgHandler
{
	Q_OBJECT

public:
	ReportSettingHandler(QObject *parent);
	~ReportSettingHandler();

	bool handleWsMsg(QJsonObject& obj, QString& response) override;

private:
	[[nodiscard]] bool addData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);
	[[nodiscard]] bool fetchData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);
	
};
