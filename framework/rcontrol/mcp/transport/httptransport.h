/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) 2026 MuseScore/Audacity and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QByteArray>

#include "../itransport.h"

class QTcpServer;
class QTcpSocket;

//! NOTE Streamable HTTP endpoint: POST http://127.0.0.1:2212/mcp

namespace muse::rcontrol::mcp {
class HttpConnection : public QObject
{
    Q_OBJECT
public:
    explicit HttpConnection(QTcpSocket* socket, const ITransport::RequestHandler& onRequest, QObject* parent = nullptr);

private slots:
    void onReadyRead();

private:
    struct HttpMessage {
        QByteArray method;
        QByteArray path;
        QByteArray version;
        QHash<QByteArray, QByteArray> headers;
        QByteArray body;
    };

    enum class TakeStatus {
        NeedMore,
        Ready,
        Error
    };

    void processAvailable();
    TakeStatus takeMessage(HttpMessage& message, int& errorStatus, const char*& errorReason);
    bool dispatch(const HttpMessage& message);
    void reply(int status, const char* reason, const QByteArray& contentType, const QByteArray& body, bool closeConnection);
    void fail(int status, const char* reason);
    void finishRpc(const ByteArray& response);

    QTcpSocket* m_socket = nullptr;
    QByteArray m_buffer;
    QByteArray m_origin;
    bool m_busy = false;
    bool m_closeAfterResponse = false;
    bool m_sse = false;
    ITransport::RequestHandler m_onRequest = nullptr;
};

class HttpTransport : public ITransport
{
public:
    ~HttpTransport();

    bool start() override;
    void stop() override;

    void onRequest(const RequestHandler& onRequest) override;

private:
    QTcpServer* m_server = nullptr;
    RequestHandler m_onRequest = nullptr;
};
}
