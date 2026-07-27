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
#pragma once

#ifndef NO_QT_SUPPORT

#include <memory>
#include <vector>

#include <QObject>

#include "../iapplicationeventcontroller.h"

namespace muse {
class ApplicationEventController : public QObject, public IApplicationEventController
{
public:
    ApplicationEventController();
    ~ApplicationEventController() override;

    async::Channel<EventData> eventReceived() const override;

    void setPendingEventTypes(const std::vector<QEvent::Type>& types) override;
    std::vector<std::unique_ptr<QEvent> > takePendingEvents() override;

private:

    bool eventFilter(QObject* watched, QEvent* event) override;

    std::vector<QEvent::Type> m_pendingTypes;
    std::vector<std::unique_ptr<QEvent> > m_pendingEvents;

    async::Channel<EventData> m_eventReceived = async::Channel<EventData>(async::makeOpt()
                                                                          .name("muse::ApplicationEventController::m_eventReceived")
                                                                          .threads(1));
};
}

#endif // NO_QT_SUPPORT
