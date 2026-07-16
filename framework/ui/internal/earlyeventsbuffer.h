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
#include <set>
#include <vector>

#include <QEvent>
#include <QObject>

namespace muse::ui {
//! Buffers selected event types delivered before the app installs its own consumers
// (e.g. on macOS the launch url arrives as a one-shot QFileOpenEvent during an early startup)
class EarlyEventsBuffer : public QObject
{
public:
    EarlyEventsBuffer(const std::set<QEvent::Type>& types, QObject* parent);

    std::vector<std::unique_ptr<QEvent> > take(QEvent::Type type);

private:
    bool eventFilter(QObject* watched, QEvent* event) override;

    std::set<QEvent::Type> m_types;
    std::vector<std::unique_ptr<QEvent> > m_events;
};
}
