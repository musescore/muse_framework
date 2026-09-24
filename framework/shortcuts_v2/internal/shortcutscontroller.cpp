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
#include "shortcutscontroller.h"

#include "log.h"

#define SHORTCUTS_DEBUG 1

#if SHORTCUTS_DEBUG
#define SC_LOG() LOGDA() << "[SC] "
#else
#define SC_LOG() LOGN()
#endif

using namespace muse::shortcuts;
using namespace muse::rcommand;

void ShortcutsController::init()
{
    interactive()->currentUri().ch.onReceive(this, [this](const Uri&) {
        //! NOTE: enable process shortcuts only for non-widget objects
        setActive(!interactive()->topWindowIsWidget());
    });
}

void ShortcutsController::activate(const std::string& sequence)
{
    if (!m_active) {
        return;
    }

    LOGD() << sequence;

    ShortcutList allowedShortcuts;
    const ShortcutList& commandShortcuts = commandShortcutsRegister()->shortcutsForSequence(sequence);
    SC_LOG() << "commandShortcuts: " << commandShortcuts.size();
    for (const Shortcut& sc : commandShortcuts) {
        const Command& command = Command(sc.command);
        if (commandsState()->commandState(command).enabled) {
            allowedShortcuts.push_back(sc);
        }
    }
    SC_LOG() << "allowedShortcuts: " << allowedShortcuts.size();

    Shortcut selectedShortcut;
    if (allowedShortcuts.size() == 1) {
        selectedShortcut = allowedShortcuts.front();
    } else if (allowedShortcuts.size() > 1) {
        if (shortcutsResolver()) {
            selectedShortcut = shortcutsResolver()->selectOne(allowedShortcuts);
        } else {
            selectedShortcut = allowedShortcuts.front();
        }
    }
    SC_LOG() << "selectedShortcut: " << selectedShortcut.command;
    if (selectedShortcut.isValid()) {
        commandDispatcher()->dispatch(Command(selectedShortcut.command));
    }
}

bool ShortcutsController::active()
{
    return m_active;
}

void ShortcutsController::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    m_activeChanged.notify();
}

muse::async::Notification ShortcutsController::activeChanged() const
{
    return m_activeChanged;
}
