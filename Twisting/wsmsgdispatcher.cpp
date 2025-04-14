/****************************************************************************
**
** Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebSockets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "wsmsgdispatcher.h"

#include <QtSql>
#include <QJsonObject>
#include <QApplication>
#include <qwebsocket.h>
#include <qmessagebox.h>
#include <QtCore/QDebug>
#include <QJsonDocument>
#include <qwebsocketserver.h>

QT_USE_NAMESPACE
WsMsgDispatcher::WsMsgDispatcher(quint16 port, bool debug, QObject* parent) :
	QObject(parent),
	m_pWebSocketServer(new QWebSocketServer(QStringLiteral("React WebSocket Server"),
		QWebSocketServer::NonSecureMode, this)),
	m_debug(debug)
{
	if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
		if (m_debug)
			qDebug() << "WSserver listening on port" << port;
		connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
			this, &WsMsgDispatcher::onNewConnection);
	}

	//外部使用者可以通过发送信号，进而发送WS消息。
	//通过信号槽的方式，确保接收发送都在一个线程
	connect(this, &WsMsgDispatcher::writeMsg, this, &WsMsgDispatcher::fireEvent);
	connect(this, &WsMsgDispatcher::closed, this, &WsMsgDispatcher::close);
	
}

WsMsgDispatcher::~WsMsgDispatcher()
{
	//由子线程（归属线程）进行关闭
	emit closed();
}


void WsMsgDispatcher::close() {
	m_pWebSocketServer->close();
	qDeleteAll(m_clients.begin(), m_clients.end());
}

void WsMsgDispatcher::onNewConnection()
{
	QWebSocket* pSocket = m_pWebSocketServer->nextPendingConnection();
	connect(pSocket, &QWebSocket::textMessageReceived, this, &WsMsgDispatcher::processTextMessage);
	connect(pSocket, &QWebSocket::disconnected, this, &WsMsgDispatcher::socketDisconnected);
	m_clients << pSocket;
}



void WsMsgDispatcher::fireEvent(const QString& msg, const QString& ip) {
	for (auto it : m_clients) {
		if (ip.length() && it->peerAddress().toIPv4Address() != QHostAddress( ip).toIPv4Address())
			continue;
		it->sendTextMessage(msg);
	}
}

void WsMsgDispatcher::responseToWs(const QJsonObject& recvObj) {
	/*auto it = handlers_.find(recvObj["__channel"].toString());
	if (it == handlers_.end())
		return;*/
	//设备通讯结果，直接发送至WS即可
	QJsonObject obj = recvObj;
	obj["__channel"] = obj["__channel"].toString() + "-" + obj["__type"].toString();
	QJsonDocument jsonDoc(obj);
	QString response = jsonDoc.toJson();
	fireEvent(response);
}

void WsMsgDispatcher::processTextMessage(QString message)
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	if (m_debug) {
		qDebug() << "Message received:" << message;
	}
	if (pClient == nullptr)
		return;
	QByteArray byteArray = message.toUtf8();
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(byteArray, &jsonError);
	if (jsonError.error != QJsonParseError::NoError) {
		if (m_debug)
			qDebug() << "Error parsing JSON:" << jsonError.errorString();
		return;
	}
	QJsonObject recvObj = jsonDoc.object();

	QString channel = recvObj["__channel"].toString();
	if (channel.left(7) == "control") {
		//控制类消息，转发至设备通讯线程
		emit requestMach(recvObj);
		return;
	}
	else if (channel == "pid-message") {
		//控制类消息，转发至设备通讯线程
		emit requestMach(recvObj);
		return;
	}
	else if(channel == "testing-message") {
		//测试类消息，统一转发给数据处理线程
		//因为有一些指令，需要先从数据库中获取，再跟设备通讯
		//所以先有数据处理线程获取数据后，再转发给设备通讯线程
		emit requestProcessor(recvObj);
	}
	else {
		//测试内容设定
		emit requestProcessor(recvObj);
	}
}

void WsMsgDispatcher::socketDisconnected()
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	if (m_debug)
		qDebug() << "socketDisconnected:" << pClient;
	if (pClient) {
		m_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}
