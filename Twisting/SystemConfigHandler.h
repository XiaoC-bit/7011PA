#pragma once

#include "MsgHandler.h"
#include <qsqlquery.h>
#include <qsqldatabase.h>

class SystemConfigHandler : public MsgHandler
{
	Q_OBJECT

public:
	SystemConfigHandler(QObject *parent);
	~SystemConfigHandler();

	bool handleWsMsg(QJsonObject& obj, QString& response) override;

private:
	[[nodiscard]] bool fetchData(const QSqlDatabase& db, const QJsonObject& obj, QString& response);
	[[nodiscard]] bool changeLanguage(const QSqlDatabase& db, const QJsonObject& obj, QString& response);

	
	
};
