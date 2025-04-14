#pragma once

#include "MsgHandler.h"
#include <qsqlquery.h>
#include <qsqldatabase.h>

class ReportHandler : public MsgHandler
{
	Q_OBJECT

public:
	ReportHandler(QObject *parent);
	~ReportHandler();

	bool handleWsMsg(QJsonObject& obj, QString& response) override;

private:
	/**
	 * 测试画面的当前数据.
	 * 
	 * \param db
	 * \param obj
	 * \param response
	 * \return 
	 */
	[[nodiscard]] bool liveTestingData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);
	
	//导出数据
	[[nodiscard]] bool exportData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//查询报告数据
	[[nodiscard]] bool fetchReportData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);
};
