#include "ControlTestCommHandler.h"

ControlTestCommHandler::ControlTestCommHandler(ReadU65Struct& ref, QObject* parent)
	: CommHandler(ref,parent)
{}

ControlTestCommHandler::~ControlTestCommHandler()
{
}

bool ControlTestCommHandler::commFunc(CommunicationThread* socket, QJsonObject& obj, QString& err) {

	QString type = obj["__type"].toString();
	if (type == "home") {
		return home(socket, obj, err);
	}
	else if (type == "stop") {
		return stop(socket, obj, err);
	}
	else if (type == "spin") {
		return spin(socket, obj, err);
	}
	else if (type == "re-spin") {
		return reSpin(socket, obj, err);
	}
	else if (type == "start-test") {
		return startTest(socket, obj, err);
	}
	return false;
}

bool ControlTestCommHandler::startTest(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x05, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}

bool ControlTestCommHandler::home(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x30, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}


bool ControlTestCommHandler::stop(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x04, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}

bool ControlTestCommHandler::spin(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x1c, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}

bool ControlTestCommHandler::reSpin(CommunicationThread* socket, QJsonObject& obj, QString& err) {
	QByteArray buffer;
	packPC_KEY(0x1d, buffer);
	if (!socket->writeData(buffer)) {
		err = "writeData error " + socket->socketError();
		return false;
	}
	QByteArray recvData;
	if (!socket->readData(recvData)) {
		err = "readData error " + socket->socketError();
		return false;
	}
	if (!checkSum(recvData)) {
		err = "readData error ,checksum error";
		return false;
	}
	return true;
}
