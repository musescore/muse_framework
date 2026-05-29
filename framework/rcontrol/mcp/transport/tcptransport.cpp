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

#include "tcptransport.h"

#include <QTcpServer>
#include <QTcpSocket>

#include "global/serialization/json.h"

#include "log.h"

static const int DEFAULT_PORT = 2212;

using namespace muse::rcontrol::mcp;

TcpConnection::TcpConnection(QTcpSocket* socket, const ITransport::RequestHandler& onRequest, QObject* parent)
    : QObject(parent), m_socket(socket), m_onRequest(onRequest)
{
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
}

void TcpConnection::onReadyRead()
{
    m_buffer += m_socket->readAll();

    int idx = -1;
    while ((idx = m_buffer.indexOf('\n')) != -1) {
        const QByteArray message = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);
        processMessage(message);
    }
}

void TcpConnection::processMessage(const QByteArray& request)
{
    LOGD() << "request: " << request;

    if (m_onRequest) {
        ByteArray req = ByteArray::fromQByteArrayNoCopy(request);
        m_onRequest(req, [this](const ByteArray& response) {
            QByteArray resp = response.toQByteArrayNoCopy();
            LOGD() << "response: " << resp;
            m_socket->write(resp);
            m_socket->write("\n");
            m_socket->flush();
        });
    } else {
        LOGE() << "No onResponse handler";
        m_socket->write("\n");
        m_socket->flush();
    }
}

TcpTransport::~TcpTransport()
{
    stop();
}

bool TcpTransport::start()
{
    if (!m_server) {
        m_server = new QTcpServer();
        QObject::connect(m_server, &QTcpServer::newConnection, [this]() {
            if (m_connection) {
                LOGE() << "Already connected";
                return;
            }

            QTcpSocket* socket = m_server->nextPendingConnection();
            m_connection = new TcpConnection(socket, m_onRequest);
        });
    }

    if (!m_server->listen(QHostAddress::Any, DEFAULT_PORT)) {
        return false;
    }

    LOGI() << "TcpTransport started on port " << DEFAULT_PORT;

    return true;
}

void TcpTransport::stop()
{
    if (m_server) {
        delete m_connection;
        m_connection = nullptr;

        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

void TcpTransport::onRequest(const RequestHandler& onRequest)
{
    m_onRequest = onRequest;
}
