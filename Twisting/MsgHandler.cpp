#include "MsgHandler.h"
#include <qfile.h>
#include <qdebug.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qapplication.h>
MsgHandler::MsgHandler(QObject *parent)
	: QObject(parent)
{}

MsgHandler::~MsgHandler()
{}

bool MsgHandler::getConfigDB(QSqlDatabase& db){
	QString dbName = QString("sql_default_%1").arg(deviceId_);

	if (!QSqlDatabase::contains(dbName))
		db = QSqlDatabase::addDatabase("QSQLITE", dbName);
	else 
		db = QSqlDatabase::database(dbName);
	
	QString str = QCoreApplication::applicationDirPath() + "/database/config/";
	db.setDatabaseName(QCoreApplication::applicationDirPath() + "/database/config/" + "config.db");
	if (db.isOpen())
		return true;
	if (!db.open()) {
		qDebug() << "Failed to open database:" << db.lastError();
		return false;
	}
	return true;
}


void MsgHandler::setDeviceId(int id) {
	deviceId_ = id;
}

bool MsgHandler::getTestDataDB(const QString& project, const QString& method, QSqlDatabase& db) {
	/**
	 * 如果发现数据库不存在，说明是新的专案或者新的测试方法，需要从初始化数据库中拷贝新建数据库
	 * 
	 */
	QString dbFile = QCoreApplication::applicationDirPath() + "/database/" + project + "/" + method + ".db";
	QFile file(dbFile);
	if (!file.exists()) {
		//拷贝初始数据库
		QFile::copy(QCoreApplication::applicationDirPath() + "/database/init/data.db", dbFile);
	}

	QString dbName = QString("%1-%2-%3")
		.arg(project)
		.arg(method)
		.arg(deviceId_);
	if (!QSqlDatabase::contains(dbName))
		db = QSqlDatabase::addDatabase("QSQLITE", dbName);
	else
		db = QSqlDatabase::database(dbName);

	QString str = QCoreApplication::applicationDirPath() + "/database/" + project +"/";
	db.setDatabaseName(dbFile);
	if (db.isOpen())
		return true;
	if (!db.open()) {
		qDebug() << "Failed to open database:" << db.lastError();
		return false;
	}
	return true;
}

bool MsgHandler::getTestDataDB(QSqlDatabase& db) {
	QSqlDatabase configDb;
	if (!getConfigDB(configDb))
		return false;
	QString strSql = QString("select current_project,current_method from system_config where id=%1").arg(deviceId_);
	QString strProject, strMethod;
	QSqlQuery configQuery(configDb);
	if (!configQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << configQuery.lastError().text();
		return false;
	}
	if (configQuery.next()) {
		strProject = configQuery.value("current_project").toString();
		strMethod = configQuery.value("current_method").toString();
	}

	if (!getTestDataDB(strProject, strMethod, db)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << configQuery.lastError().text();
		return false;
	}
	return true;
}


bool MsgHandler::getTestDataDB(QSqlDatabase& db,int deviceId) {
	QSqlDatabase configDb;
	if (!getConfigDB(configDb))
		return false;
	QString strSql = QString("select current_project,current_method from system_config where id=%1").arg(deviceId);
	QString strProject, strMethod;
	QSqlQuery configQuery(configDb);
	if (!configQuery.exec(strSql)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << configQuery.lastError().text();
		return false;
	}
	if (configQuery.next()) {
		strProject = configQuery.value("current_project").toString();
		strMethod = configQuery.value("current_method").toString();
	}

	if (!getTestDataDB(strProject, strMethod, db)) {
		qDebug() << "Failed to fetch data:";
		qDebug() << configQuery.lastError().text();
		return false;
	}
	return true;
}



