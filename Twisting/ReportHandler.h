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

	//设置时间范围，按 start_time/end_time 过滤当前队列的 raw_data 并写回
	[[nodiscard]] bool setTimeRange(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//导出数据
	[[nodiscard]] bool exportData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//导出历史数据
	[[nodiscard]] bool exportHistoryData(const QSqlDatabase& configDb, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//删除历史数据
	[[nodiscard]] bool deleteHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//再计算
	[[nodiscard]] bool recalculateHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//查询报告数据
	[[nodiscard]] bool fetchReportData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//查询历史数据
	[[nodiscard]] bool fetchHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//查询测试详情
	[[nodiscard]] bool fetchTestHistoryDetail(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	//查询报告历史数据
	[[nodiscard]] bool fetchReportHistoryData(const QSqlDatabase& db, const QSqlDatabase& dataDb, const QJsonObject& obj, QString& response);

	void sumupQueue(const QSqlDatabase& db, const QSqlDatabase& dataDb);
};
 