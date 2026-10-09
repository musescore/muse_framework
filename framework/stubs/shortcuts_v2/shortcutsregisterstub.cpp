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
#include "shortcutsregisterstub.h"

using namespace muse;
using namespace muse::shortcuts;

const ShortcutList& ShortcutsRegisterStub::shortcuts() const
{
    static const ShortcutList list;
    return list;
}

Ret ShortcutsRegisterStub::updateShortcuts(const ShortcutList&)
{
    return make_ret(Ret::Code::NotImplemented);
}

void ShortcutsRegisterStub::resetShortcuts()
{
}

async::Notification ShortcutsRegisterStub::shortcutsChanged() const
{
    return async::Notification();
}

Ret ShortcutsRegisterStub::setAdditionalShortcuts(const std::string&, const ShortcutList&)
{
    return make_ret(Ret::Code::NotImplemented);
}

const Shortcut& ShortcutsRegisterStub::shortcut(const rcommand::Command&) const
{
    static Shortcut s;
    return s;
}

const Shortcut& ShortcutsRegisterStub::defaultShortcut(const rcommand::Command&) const
{
    static Shortcut s;
    return s;
}

ShortcutList ShortcutsRegisterStub::shortcutsForSequence(const std::string&) const
{
    return {};
}

Ret ShortcutsRegisterStub::importFromFile(const io::path_t&)
{
    return make_ret(Ret::Code::NotImplemented);
}

Ret ShortcutsRegisterStub::exportToFile(const io::path_t&) const
{
    return make_ret(Ret::Code::NotImplemented);
}

std::vector<std::string> ShortcutsRegisterStub::availablePresets() const
{
    return {};
}

std::string ShortcutsRegisterStub::currentPresetName() const
{
    return {};
}

void ShortcutsRegisterStub::setCurrentPresetName(const std::string&)
{
}

async::Channel<std::string> ShortcutsRegisterStub::currentPresetNameChanged() const
{
    return async::Channel<std::string>();
}

bool ShortcutsRegisterStub::isPresetEdited(const std::string&) const
{
    return false;
}

bool ShortcutsRegisterStub::canDeletePreset(const std::string&) const
{
    return false;
}

void ShortcutsRegisterStub::deletePreset(const std::string&)
{
}

void ShortcutsRegisterStub::reload(bool)
{
}
