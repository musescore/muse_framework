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
#include "earlyeventsbuffer.h"

#include "log.h"

using namespace muse::ui;

EarlyEventsBuffer::EarlyEventsBuffer(const std::set<QEvent::Type>& types, QObject* parent)
    : QObject(parent), m_types(types)
{
}

std::vector<std::unique_ptr<QEvent> > EarlyEventsBuffer::take(QEvent::Type type)
{
    IF_ASSERT_FAILED(m_types.count(type)) {
        return {};
    }

    std::vector<std::unique_ptr<QEvent> > taken;
    for (auto it = m_events.begin(); it != m_events.end();) {
        if ((*it)->type() == type) {
            taken.push_back(std::move(*it));
            it = m_events.erase(it);
        } else {
            ++it;
        }
    }

    return taken;
}

bool EarlyEventsBuffer::eventFilter(QObject* watched, QEvent* event)
{
    if (m_types.count(event->type())) {
        if (QEvent* cloned = event->clone()) {
            m_events.push_back(std::unique_ptr<QEvent>(cloned));
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}
