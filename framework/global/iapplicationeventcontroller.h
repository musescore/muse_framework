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

#include <memory>
#include <vector>

#include "modularity/imoduleinterface.h"
#include "async/channel.h"

#ifndef NO_QT_SUPPORT
#include <QEvent>

class QObject;
#endif

namespace muse {
class IApplicationEventController : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IApplicationEventController)
public:

    virtual ~IApplicationEventController() = default;

#ifndef NO_QT_SUPPORT

    struct EventData {
        QObject* watched = nullptr;
        QEvent* event = nullptr;
    };

    virtual async::Channel<EventData> eventReceived() const = 0;
    virtual void setPendingEventTypes(const std::vector<QEvent::Type>& types) = 0;

    //! NOTE The watched object is deliberately not returned.
    //! By the time the events are taken it may already be destroyed.
    virtual std::vector<std::unique_ptr<QEvent> > takePendingEvents() = 0;
#endif
};
}
