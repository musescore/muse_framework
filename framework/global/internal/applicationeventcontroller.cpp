/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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
#include "applicationeventcontroller.h"

#ifndef NO_QT_SUPPORT

#include <QCoreApplication>

#include "containers.h"
#include "runtime.h"

#include "log.h"

using namespace muse;

ApplicationEventController::ApplicationEventController()
{
    DO_ASSERT(std::this_thread::get_id() == runtime::mainThreadId());
    qApp->installEventFilter(this);
}

ApplicationEventController::~ApplicationEventController()
{
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

async::Channel<IApplicationEventController::EventData> ApplicationEventController::eventReceived() const
{
    return m_eventReceived;
}

void ApplicationEventController::setPendingEventTypes(const std::vector<QEvent::Type>& types)
{
    m_pendingTypes = types;
}

std::vector< std::unique_ptr<QEvent> > ApplicationEventController::takePendingEvents()
{
    DO_ASSERT(std::this_thread::get_id() == runtime::mainThreadId());

    m_pendingTypes.clear();
    std::vector<std::unique_ptr<QEvent> > events = std::move(m_pendingEvents);
    m_pendingEvents.clear();

    return events;
}

bool ApplicationEventController::eventFilter(QObject* watched, QEvent* event)
{
    if (contains(m_pendingTypes, event->type())) {
        m_pendingEvents.emplace_back(event->clone());
    }

    m_eventReceived.send(EventData { watched, event });
    return false;
}

#endif // NO_QT_SUPPORT
