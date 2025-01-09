#include "Twisting.h"
#include <QtWidgets/QApplication>

#include <QMutex>
#include <QFileInfo>
#include <QDateTime>
#include <qprocess.h>
#include <windows.h>
#include <QCefContext.h>
#include <QtSql>
#include <qsettings.h>
#include <tlhelp32.h>
#include <tlhelp32.h>

#include "dumpfile.h"
#include "DataDefine.h"


bool terminateProcessByName(const std::string& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		qWarning() << "Failed to create snapshot";
		return false;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hSnapshot, &pe)) {
		do {
			if (processName == wstringToString(pe.szExeFile)) {
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
				if (hProcess != NULL) {
					if (TerminateProcess(hProcess, 0)) {
						//qDebug() << "Successfully terminated process:" << processName;
						CloseHandle(hProcess);
						CloseHandle(hSnapshot);
						return true;
					}
					else {
						//qWarning() << "Failed to terminate process:" << processName;
						CloseHandle(hProcess);
					}
				}
				else {
					//qWarning() << "Failed to open process:" << processName;
				}
			}
		} while (Process32Next(hSnapshot, &pe));
	}
	else {
		qWarning() << "Failed to retrieve process information";
	}

	CloseHandle(hSnapshot);
	return false;
}

void logMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	static QMutex mutex;
	mutex.lock();

	QString text;
	switch (type)
	{
	case QtDebugMsg:
		text = QString("Debug:");
		break;

	case QtWarningMsg:
		text = QString("Warning:");
		break;

	case QtCriticalMsg:
		text = QString("Critical:");
		break;

	case QtFatalMsg:
		text = QString("Fatal:");
	}


	QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
	QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
	QString current_date = QString("(%1)").arg(current_date_time);
	QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(msg).arg(current_date);
	QFile file;
	if (msg.indexOf("Message received") == -1) {
		file.setFileName("./log/operation_log-" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".txt");
	}
	else {
		file.setFileName("./log/system_log-" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".txt");
	}

	if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
		mutex.unlock();
		return;
	}

	QTextStream text_stream(&file);
	text_stream << message << "\r\n";
	file.flush();
	file.close();

	mutex.unlock();
}

#ifdef _DEBUG
bool gDebug = true;
#else
bool gDebug = false;
#endif

Config gConfig;
QString gSqlType = "SqlLite";

int main(int argc, char *argv[])
{
#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
	// For off-screen rendering, Qt::AA_EnableHighDpiScaling must be enabled. If not,
	// then all devicePixelRatio methods will always return 1.0,
	// so CEF will not scale the web content
	// NOET: There is bugs in Qt 6.2.4, the HighDpi doesn't work 
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
	setExceptionHandlers();

    QApplication a(argc, argv);
	qInstallMessageHandler(logMessage);


	QDir dir = QCoreApplication::applicationDirPath();

	QString iniFile = dir.filePath("config/config.ini");

	QSettings settings(iniFile, QSettings::IniFormat);
	if (!QFile::exists(iniFile)) {
		settings.setValue("System/DebugPort", 28001);
		settings.setValue("System/FrontExe", "gotechHttp.exe");
		settings.setValue("System/WebSocketPort", 20203);
		settings.setValue("System/FrontPort", 7779);
		settings.sync();
	}
	gConfig.debugPort = settings.value("System/DebugPort", 28001).toInt();
	gConfig.webSocketPort = settings.value("System/WebSocketPort", 20203).toInt();
	gConfig.frontPort = settings.value("System/FrontPort", 7779).toInt();	
	gConfig.frontExeName = settings.value("System/FrontExe", "gotechHttp.exe").toString();

	// build QCefConfig
	QCefConfig config;
	// set user agent 
	config.setUserAgent("QCefViewTest");
	// set log level
	config.setLogLevel(QCefConfig::LOGSEVERITY_DEFAULT);
	// set JSBridge object name (default value is QCefViewClient)
	config.setBridgeObjectName("CallBridge");
	// port for remote debugging (default is 0 and means to disable remote debugging)
	config.setRemoteDebuggingPort(gConfig.debugPort);
	// set background color for all browsers
	// (QCefSetting.setBackgroundColor will overwrite this value for specified browser instance)
	config.setBackgroundColor(Qt::lightGray);

	// WindowlessRenderingEnabled is set to true by default,
	// set to false to disable the OSR mode
	config.setWindowlessRenderingEnabled(true);

	// add command line args, you can any cef supported switches or parameters
	config.addCommandLineSwitch("use-mock-keychain");
	// config.addCommandLineSwitch("disable-gpu");
	// config.addCommandLineSwitch("enable-media-stream");
	// config.addCommandLineSwitch("allow-file-access-from-files");
	// config.addCommandLineSwitch("disable-spell-checking");
	// config.addCommandLineSwitch("disable-site-isolation-trials");
	// config.addCommandLineSwitch("enable-aggressive-domstorage-flushing");
	config.addCommandLineSwitchWithValue("renderer-process-limit", "1");
	// allow remote debugging
	config.addCommandLineSwitchWithValue("remote-allow-origins", "*");
	// config.addCommandLineSwitchWithValue("disable-features", "BlinkGenPropertyTrees,TranslateUI,site-per-process");

#if defined(Q_OS_MACOS) && defined(QT_DEBUG)
  // cef bugs on macOS debug build
	config.setCachePath(QDir::tempPath());
#endif

	// create QCefContext instance with config,
	// the lifecycle of cefContext must be the same as QApplication instance
	QCefContext cefContext(&a, argc, argv, &config);


	QProcess program;
	QStringList args;




	QString pyHttpExe = dir.filePath(QString("FrontEnv/%1").arg(gConfig.frontExeName));
	program.setWorkingDirectory(dir.filePath("FrontEnv/"));
	//args.append("-h");
	if (!gDebug) {
		while (1) {
			if (!terminateProcessByName(gConfig.frontExeName.toStdString()))
				break;
		}
		program.start(pyHttpExe, args);
		if (!program.waitForStarted()) {
			qDebug() << "";
			exit(0);
		}
		QObject::connect(&a, &QCoreApplication::aboutToQuit, [&program, pyHttpExe]() {
			if (program.state() != QProcess::NotRunning) {
				program.kill();
				program.waitForFinished();
				QString cmd = "taskkill /F /IM " + QFileInfo(pyHttpExe).fileName();
				QProcess::execute(cmd);
			}
			});
	}


    Twisting w;
    w.show();
    return a.exec();
}
