#pragma once

#include "MsgHandler.h"
#include <qsqlquery.h>
#include <qsqldatabase.h>

class TestingHandler : public MsgHandler
{
	Q_OBJECT

public:
	TestingHandler(QObject *parent);
	~TestingHandler();

	bool handleWsMsg(QJsonObject& obj, QString& response) override;

	bool isTransfer(const QJsonObject& obj) override;

private:
	[[nodiscard]] bool transferMethodPreHandle(const QSqlDatabase& configDb, const QSqlDatabase& testDb, QJsonObject& obj, QString& response);

	[[nodiscard]] bool startTest(const QSqlDatabase& configDb, const QSqlDatabase& testDb, QJsonObject& obj, QString& response);

};
