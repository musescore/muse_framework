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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "../../../idlesleepblocker.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>

#include "log.h"

using namespace muse;

struct IdleSleepBlocker::Data {
    //! NOTE Holding the descriptor is what holds the inhibitor; closing it releases it
    QDBusUnixFileDescriptor descriptor;
};

IdleSleepBlocker::IdleSleepBlocker(const std::string& reason)
    : m_data(std::make_unique<Data>())
{
    QDBusInterface manager("org.freedesktop.login1",
                           "/org/freedesktop/login1",
                           "org.freedesktop.login1.Manager",
                           QDBusConnection::systemBus());

    if (!manager.isValid()) {
        LOGW() << "Failed to connect to login1, the system may go to sleep";
        return;
    }

    QDBusReply<QDBusUnixFileDescriptor> reply = manager.call("Inhibit",
                                                             QStringLiteral("idle"),
                                                             QCoreApplication::applicationName(),
                                                             QString::fromStdString(reason),
                                                             QStringLiteral("block"));

    if (!reply.isValid()) {
        LOGW() << "Failed to inhibit idle sleep: " << reply.error().message().toStdString()
               << ", the system may go to sleep";
        return;
    }

    m_data->descriptor = reply.value();
}

IdleSleepBlocker::~IdleSleepBlocker() = default;
