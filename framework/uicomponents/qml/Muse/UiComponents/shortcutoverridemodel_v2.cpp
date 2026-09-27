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

 #include "shortcutoverridemodel_v2.h"

 #include <QKeySequence>

 #include "shortcuts_v2/shortcutstypes.h"
 #include "ui/navigationcommands.h"

using namespace muse::uicomponents;
using namespace muse::shortcuts;
using namespace muse::rcommand;
using namespace muse::ui;

ShortcutOverrideModel::ShortcutOverrideModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void ShortcutOverrideModel::init()
{
    shortcutsRegister()->shortcutsChanged().onNotify(this, [this](){
        loadDisallowedOverrides();
    });

    loadDisallowedOverrides();
}

bool ShortcutOverrideModel::isShortcutOverrideAllowed(Qt::Key key, Qt::KeyboardModifiers modifiers) const
{
    auto [newKey, newModifiers] = correctKeyInput(key, modifiers);

    if (needIgnoreKey(newKey)) {
        return true;
    }

    const Shortcut& shortcut = this->disallowedOverride(newKey, newModifiers);
    return !shortcut.isValid();
}

bool ShortcutOverrideModel::handleShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    auto [newKey, newModifiers] = correctKeyInput(key, modifiers);

    if (needIgnoreKey(newKey)) {
        return false;
    }

    const Shortcut& shortcut = this->disallowedOverride(newKey, newModifiers);
    bool found = shortcut.isValid();
    if (found) {
        dispatcher()->dispatch(shortcut.command);
    }

    return found;
}

ShortcutOverrideModel::DirectionKeys ShortcutOverrideModel::directionKeysForOverride() const
{
    return m_directionKeysForOverride;
}

void ShortcutOverrideModel::setDirectionKeysForOverride(const ShortcutOverrideModel::DirectionKeys& keys)
{
    if (m_directionKeysForOverride == keys) {
        return;
    }
    m_directionKeysForOverride = keys;
    emit directionKeysForOverrideChanged();

    loadDisallowedOverrides();
}

void ShortcutOverrideModel::loadDisallowedOverrides()
{
    m_notAllowedForOverrideShortcuts.clear();

    //! NOTE: navigation shortcuts cannot be overridden...
    static const std::vector<Command> commands {
        NEXT_SECTION_COMMAND,
        PREV_SECTION_COMMAND,
        NEXT_PANEL_COMMAND,
        PREV_PANEL_COMMAND,
        NEXT_TAB_COMMAND,
        PREV_TAB_COMMAND,
        TRIGGER_CONTROL_COMMAND,
        FIRST_CONTROL_COMMAND,
        LAST_CONTROL_COMMAND,
        NEXTROW_CONTROL_COMMAND,
        PREVROW_CONTROL_COMMAND
    };

    for (const Command& command : commands) {
        m_notAllowedForOverrideShortcuts.push_back(shortcutsRegister()->shortcut(command));
    }

    if (!m_directionKeysForOverride.testFlag(DirectionKey::LeftRight)) {
        // We don't want to override left/right - dispatch navigation instead...
        m_notAllowedForOverrideShortcuts.push_back(shortcutsRegister()->shortcut(LEFT_COMMAND));
        m_notAllowedForOverrideShortcuts.push_back(shortcutsRegister()->shortcut(RIGHT_COMMAND));
    }

    if (!m_directionKeysForOverride.testFlag(DirectionKey::UpDown)) {
        // We don't want to override up/down - dispatch navigation instead...
        m_notAllowedForOverrideShortcuts.push_back(shortcutsRegister()->shortcut(UP_COMMAND));
        m_notAllowedForOverrideShortcuts.push_back(shortcutsRegister()->shortcut(DOWN_COMMAND));
    }
}

Shortcut ShortcutOverrideModel::disallowedOverride(Qt::Key key, Qt::KeyboardModifiers modifiers) const
{
    QKeySequence keySequence(modifiers | key);
    for (const Shortcut& shortcut : m_notAllowedForOverrideShortcuts) {
        for (const std::string& seq : shortcut.sequences) {
            QKeySequence shortcutSequence(QString::fromStdString(seq));
            if (shortcutSequence == keySequence) {
                return shortcut;
            }
        }
    }

    return Shortcut();
}
