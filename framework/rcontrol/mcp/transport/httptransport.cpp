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

#include "httptransport.h"

#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>

#include "log.h"

static const int HTTP_DEFAULT_PORT = 2212;
static const int HTTP_MAX_HEADER_BYTES = 16 * 1024;
static const int HTTP_MAX_BODY_BYTES = 1024 * 1024;
static const char* HTTP_PROTOCOL_VERSION = "2025-11-25";

using namespace muse::rcontrol::mcp;

static QByteArray headerValue(const QHash<QByteArray, QByteArray>& headers, const QByteArray& name)
{
    return headers.value(name.toLower());
}

static bool isJsonContentType(const QByteArray& value)
{
    return value.toLower().startsWith("application/json");
}

static bool isNotificationAck(const QByteArray& response)
{
    QByteArray compact;
    compact.reserve(response.size());
    for (const char ch : response) {
        if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
            compact.append(ch);
        }
    }
    return compact.isEmpty() || compact == "{}";
}

static bool isAllowedOrigin(const QByteArray& origin)
{
    const int schemeEnd = origin.indexOf("://");
    if (schemeEnd <= 0) {
        return false;
    }

    const QByteArray scheme = origin.left(schemeEnd).toLower();
    if (scheme != "http" && scheme != "https") {
        return false;
    }

    QByteArray host = origin.mid(schemeEnd + 3);
    if (host.isEmpty() || host.contains('/')) {
        return false;
    }

    if (host.startsWith('[')) {
        const int end = host.indexOf(']');
        if (end < 0) {
            return false;
        }
        const QByteArray rest = host.mid(end + 1);
        if (!rest.isEmpty() && !rest.startsWith(':')) {
            return false;
        }
        return host.mid(1, end - 1) == "::1";
    }

    const int colon = host.indexOf(':');
    if (colon >= 0) {
        host = host.left(colon);
    }
    return host == "127.0.0.1" || host.compare("localhost", Qt::CaseInsensitive) == 0;
}

HttpConnection::HttpConnection(QTcpSocket* socket, const ITransport::RequestHandler& onRequest, QObject* parent)
    : QObject(parent), m_socket(socket), m_onRequest(onRequest)
{
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &HttpConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
}

void HttpConnection::onReadyRead()
{
    m_buffer += m_socket->readAll();
    processAvailable();
}

void HttpConnection::processAvailable()
{
    while (!m_busy && m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        HttpMessage message;
        int errorStatus = 400;
        const char* errorReason = "Bad Request";
        const TakeStatus status = takeMessage(message, errorStatus, errorReason);
        if (status == TakeStatus::NeedMore) {
            return;
        }
        if (status == TakeStatus::Error) {
            fail(errorStatus, errorReason);
            return;
        }
        if (!dispatch(message)) {
            return;
        }
    }
}

HttpConnection::TakeStatus HttpConnection::takeMessage(HttpMessage& message, int& errorStatus, const char*& errorReason)
{
    const qsizetype headerEnd = m_buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        if (m_buffer.size() > HTTP_MAX_HEADER_BYTES) {
            m_buffer.clear();
            errorStatus = 431;
            errorReason = "Request Header Fields Too Large";
            return TakeStatus::Error;
        }
        return TakeStatus::NeedMore;
    }

    if (headerEnd > HTTP_MAX_HEADER_BYTES) {
        m_buffer.clear();
        errorStatus = 431;
        errorReason = "Request Header Fields Too Large";
        return TakeStatus::Error;
    }

    const QByteArray head = m_buffer.left(headerEnd);
    const QList<QByteArray> lines = head.split('\n');
    if (lines.isEmpty()) {
        m_buffer.clear();
        return TakeStatus::Error;
    }

    const QList<QByteArray> parts = QByteArray(lines.at(0)).trimmed().split(' ');
    if (parts.size() < 2) {
        m_buffer.clear();
        return TakeStatus::Error;
    }

    message.method = parts.at(0);
    message.version = parts.size() > 2 ? parts.at(2).trimmed() : QByteArray("HTTP/1.1");
    QByteArray target = parts.at(1);
    const qsizetype query = target.indexOf('?');
    if (query >= 0) {
        target = target.left(query);
    }
    message.path = target;
    message.headers.clear();
    message.body.clear();

    for (qsizetype i = 1; i < lines.size(); ++i) {
        const QByteArray line = QByteArray(lines.at(i)).trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const qsizetype colon = line.indexOf(':');
        if (colon <= 0) {
            m_buffer.clear();
            return TakeStatus::Error;
        }
        message.headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon + 1).trimmed());
    }

    int contentLength = 0;
    const QByteArray lengthText = headerValue(message.headers, "content-length");
    if (!lengthText.isEmpty()) {
        bool ok = false;
        contentLength = lengthText.toInt(&ok);
        if (!ok || contentLength < 0) {
            m_buffer.clear();
            return TakeStatus::Error;
        }
        if (contentLength > HTTP_MAX_BODY_BYTES) {
            m_buffer.clear();
            errorStatus = 413;
            errorReason = "Payload Too Large";
            return TakeStatus::Error;
        }
    } else if (headerValue(message.headers, "transfer-encoding").toLower().contains("chunked")) {
        m_buffer.clear();
        return TakeStatus::Error;
    }

    const qsizetype total = headerEnd + 4 + contentLength;
    if (m_buffer.size() < total) {
        return TakeStatus::NeedMore;
    }

    message.body = m_buffer.mid(headerEnd + 4, contentLength);
    m_buffer.remove(0, total);
    return TakeStatus::Ready;
}

bool HttpConnection::dispatch(const HttpMessage& message)
{
    m_origin.clear();
    m_sse = false;
    m_closeAfterResponse = false;

    if (!m_socket->peerAddress().isLoopback()) {
        fail(403, "Forbidden");
        return false;
    }

    const QByteArray origin = headerValue(message.headers, "origin");
    if (!origin.isEmpty() && !isAllowedOrigin(origin)) {
        fail(403, "Forbidden");
        return false;
    }
    m_origin = origin;

    const QByteArray connection = headerValue(message.headers, "connection").toLower();
    const bool http10 = message.version.startsWith("HTTP/1.0");
    m_closeAfterResponse = connection.contains("close") || (http10 && !connection.contains("keep-alive"));

    LOGD() << "request: " << message.method << " " << message.path << " " << message.body;

    if (message.path != "/mcp" && message.path != "/mcp/") {
        reply(404, "Not Found", QByteArray(), QByteArray(), m_closeAfterResponse);
        return !m_closeAfterResponse;
    }

    if (message.method == "OPTIONS") {
        reply(204, "No Content", QByteArray(), QByteArray(), m_closeAfterResponse);
        return !m_closeAfterResponse;
    }

    if (message.method == "GET" || message.method == "DELETE") {
        reply(405, "Method Not Allowed", QByteArray(), QByteArray(), m_closeAfterResponse);
        return !m_closeAfterResponse;
    }

    if (message.method != "POST") {
        reply(405, "Method Not Allowed", QByteArray(), QByteArray(), m_closeAfterResponse);
        return !m_closeAfterResponse;
    }

    if (!isJsonContentType(headerValue(message.headers, "content-type"))) {
        reply(415, "Unsupported Media Type", QByteArray(), QByteArray(), m_closeAfterResponse);
        return !m_closeAfterResponse;
    }

    const QByteArray accept = headerValue(message.headers, "accept").toLower();
    if (accept.isEmpty() || accept.contains("application/json") || accept.contains("*/*")) {
        m_sse = false;
    } else if (accept.contains("text/event-stream")) {
        m_sse = true;
    } else {
        reply(406, "Not Acceptable", QByteArray(), QByteArray(), m_closeAfterResponse);
        return !m_closeAfterResponse;
    }

    if (!m_onRequest) {
        LOGE() << "No onRequest handler";
        fail(500, "Internal Server Error");
        return false;
    }

    m_busy = true;
    const QPointer<HttpConnection> self(this);
    const bool closeAfterResponse = m_closeAfterResponse;
    m_onRequest(ByteArray::fromQByteArrayNoCopy(message.body), [self, closeAfterResponse](const ByteArray& response) {
        if (!self) {
            return;
        }
        self->m_closeAfterResponse = closeAfterResponse;
        self->finishRpc(response);
    });
    return true;
}

void HttpConnection::reply(int status, const char* reason, const QByteArray& contentType, const QByteArray& body,
                           bool closeConnection)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray msg;
    msg.reserve(body.size() + 256);
    msg += "HTTP/1.1 ";
    msg += QByteArray::number(status);
    msg += ' ';
    msg += reason;
    msg += "\r\n";
    if (!contentType.isEmpty()) {
        msg += "Content-Type: ";
        msg += contentType;
        msg += "\r\n";
    }
    if (status == 405) {
        msg += "Allow: POST\r\n";
    }
    if (status == 204) {
        msg += "Allow: POST\r\n";
        msg += "Access-Control-Allow-Methods: POST\r\n";
        msg += "Access-Control-Allow-Headers: Content-Type, Accept, Mcp-Protocol-Version, Mcp-Session-Id\r\n";
    }
    if (!m_origin.isEmpty()) {
        msg += "Access-Control-Allow-Origin: ";
        msg += m_origin;
        msg += "\r\n";
    }
    msg += "MCP-Protocol-Version: ";
    msg += HTTP_PROTOCOL_VERSION;
    msg += "\r\n";
    msg += "Content-Length: ";
    msg += QByteArray::number(body.size());
    msg += "\r\n";
    msg += "Connection: ";
    msg += closeConnection ? "close" : "keep-alive";
    msg += "\r\n\r\n";
    msg += body;

    m_socket->write(msg);
    m_socket->flush();
    LOGD() << "response: " << status << " " << reason << " (" << body.size() << " bytes)";

    if (closeConnection) {
        m_socket->disconnectFromHost();
    }
}

void HttpConnection::fail(int status, const char* reason)
{
    m_buffer.clear();
    reply(status, reason, QByteArray(), QByteArray(), true);
}

void HttpConnection::finishRpc(const ByteArray& response)
{
    const QByteArray bytes = response.toQByteArrayNoCopy();
    if (isNotificationAck(bytes)) {
        reply(202, "Accepted", QByteArray(), QByteArray(), m_closeAfterResponse);
    } else if (m_sse) {
        QByteArray event;
        event += "event: message\ndata: ";
        event += bytes;
        event += "\n\n";
        reply(200, "OK", "text/event-stream", event, m_closeAfterResponse);
    } else {
        reply(200, "OK", "application/json", bytes, m_closeAfterResponse);
    }

    m_busy = false;
    if (!m_closeAfterResponse) {
        processAvailable();
    }
}

HttpTransport::~HttpTransport()
{
    stop();
}

bool HttpTransport::start()
{
    if (!m_server) {
        m_server = new QTcpServer();
        QObject::connect(m_server, &QTcpServer::newConnection, [this]() {
            while (QTcpSocket* socket = m_server->nextPendingConnection()) {
                new HttpConnection(socket, m_onRequest, m_server);
            }
        });
    }

    if (m_server->isListening()) {
        return true;
    }

    if (!m_server->listen(QHostAddress::LocalHost, HTTP_DEFAULT_PORT)) {
        LOGE() << "HttpTransport failed to listen: " << m_server->errorString();
        return false;
    }

    LOGI() << "HttpTransport started on http://127.0.0.1:" << HTTP_DEFAULT_PORT << "/mcp";
    return true;
}

void HttpTransport::stop()
{
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

void HttpTransport::onRequest(const RequestHandler& onRequest)
{
    m_onRequest = onRequest;
}
