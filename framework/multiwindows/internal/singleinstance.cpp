/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "../singleinstance.h"

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "multiprocess/ipc/ipc.h"
#include "multiprocess/ipc/ipclock.h"

#include "log.h"

namespace muse::mi {
static const QString SINGLE_INSTANCE_ACTIVATE("single_instance_activate");
constexpr int ACK_WAIT_MSEC = 5000;
}

bool muse::mi::activateExistingInstance(const QString& applicationId, const QStringList& args)
{
    if (applicationId.isEmpty()) {
        LOGE() << "empty applicationId";
        return false;
    }

    ipc::IpcLock lock(applicationId);
    ipc::IpcLockGuard guard(&lock);

    QLocalSocket socket;
    socket.connectToServer(applicationId);
    if (!socket.waitForConnected(ipc::TIMEOUT_MSEC)) {
        LOGD() << "no listener for " << applicationId << "; this is the first instance";
        return false;
    }

    ipc::Msg msg;
    msg.type = ipc::MsgType::Notify;
    msg.method = SINGLE_INSTANCE_ACTIVATE;
    msg.args = args;

    QByteArray data;
    ipc::serialize(msg, data);

    if (!ipc::writeToSocket(&socket, data)) {
        LOGE() << "failed to write activation message";
        return false;
    }

    if (!socket.waitForReadyRead(ACK_WAIT_MSEC)) {
        LOGW() << "no ack from existing instance within " << ACK_WAIT_MSEC << "ms";
        return false;
    }

    const QByteArray ack = socket.readAll();
    if (ack != ipc::ACK) {
        LOGW() << "unexpected ack payload: " << ack;
        return false;
    }

    socket.disconnectFromServer();
    LOGI() << "handoff acked by existing " << applicationId;
    return true;
}

namespace muse::mi {
SingleInstanceListener::SingleInstanceListener() = default;

SingleInstanceListener::~SingleInstanceListener()
{
    stop();
}

bool SingleInstanceListener::start(const QString& applicationId)
{
    if (m_server) {
        return true;
    }
    if (applicationId.isEmpty()) {
        LOGE() << "empty applicationId";
        return false;
    }

    m_server = new QLocalServer();

    bool ok = m_server->listen(applicationId);

    if (!ok && m_server->serverError() == QAbstractSocket::AddressInUseError) {
        LOGW() << "stale endpoint for " << applicationId << ", removing";
        QLocalServer::removeServer(applicationId);
        ok = m_server->listen(applicationId);
    }

    if (!ok) {
        LOGE() << "failed to listen on " << applicationId << ": " << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    QObject::connect(m_server, &QLocalServer::newConnection, m_server, [this]() {
        onNewConnection();
    });

    LOGI() << "single-instance listener bound: " << applicationId;
    return true;
}

void SingleInstanceListener::stop()
{
    if (!m_server) {
        return;
    }
    m_server->close();
    delete m_server;
    m_server = nullptr;
}

bool SingleInstanceListener::isListening() const
{
    return m_server && m_server->isListening();
}

async::Channel<QStringList> SingleInstanceListener::messageReceived() const
{
    return m_messageReceived;
}

void SingleInstanceListener::onNewConnection()
{
    QLocalSocket* socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }

    QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);

    auto channel = m_messageReceived;

    auto handleData = [socket, channel]() mutable {
        ipc::readFromSocket(socket, [socket, channel](const QByteArray& data) mutable {
            ipc::Msg msg;
            ipc::deserialize(data, msg);

            if (msg.method != SINGLE_INSTANCE_ACTIVATE) {
                LOGW() << "ignoring unexpected method: " << msg.method;
                return;
            }

            socket->write(ipc::ACK);
            socket->flush();

            LOGI() << "received activation from second instance";
            LOGD() << "args: " << msg.args;
            channel.send(msg.args);
        });
    };

    QObject::connect(socket, &QLocalSocket::readyRead, socket, handleData);

    if (socket->bytesAvailable() > 0) {
        handleData();
    }
}
}
