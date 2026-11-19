/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include "unblock.h"

#ifndef NO_QT_SUPPORT
#include <QTimer>
#endif

#include "log.h"

namespace muse::async {
void Unblock::call(Asyncable* caller, const std::function<void()>& f)
{
#ifndef NO_QT_SUPPORT
    struct Connect : public Asyncable::IConnectable {};

    Connect* connect = nullptr;
    if (caller) {
        connect = new Connect();
        caller->async_connect(connect);
    }

    QTimer::singleShot(0, [connect, caller, f] () {
        if (!connect) {
            f();
            return;
        }

        if (connect->async_isConnected(caller)) {
            f();
            DO_ASSERT(caller);
            if (caller) {
                caller->async_disconnect(connect);
            }
        }

        delete connect;
    });
#else
    UNUSED(caller);
    f();
#endif
}
}
