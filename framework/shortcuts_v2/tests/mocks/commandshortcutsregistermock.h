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

#include <gmock/gmock.h>

#include "shortcuts_v2/icommandshortcutsregister.h"

namespace muse::shortcuts {
class CommandShortcutsRegisterMock : public ICommandShortcutsRegister
{
public:
    MOCK_METHOD(const ShortcutList&, shortcuts, (), (const, override));
    MOCK_METHOD(Ret, updateShortcuts, (const ShortcutList&), (override));
    MOCK_METHOD(void, resetShortcuts, (), (override));
    MOCK_METHOD(async::Notification, shortcutsChanged, (), (const, override));

    MOCK_METHOD(Ret, setAdditionalShortcuts, (const std::string&, const ShortcutList&), (override));

    MOCK_METHOD(ShortcutList, shortcutsForSequence, (const std::string&), (const, override));
    MOCK_METHOD(const Shortcut&, shortcut, (const rcommand::Command&), (const, override));
    MOCK_METHOD(const Shortcut&, defaultShortcut, (const rcommand::Command&), (const, override));

    MOCK_METHOD(Ret, importFromFile, (const io::path_t&), (override));
    MOCK_METHOD(Ret, exportToFile, (const io::path_t&), (const, override));

    MOCK_METHOD(std::vector<std::string>, availablePresets, (), (const, override));

    MOCK_METHOD(std::string, currentPresetName, (), (const, override));
    MOCK_METHOD(void, setCurrentPresetName, (const std::string&), (override));
    MOCK_METHOD(async::Channel<std::string>, currentPresetNameChanged, (), (const, override));

    MOCK_METHOD(bool, isPresetEdited, (const std::string&), (const, override));
    MOCK_METHOD(bool, canDeletePreset, (const std::string&), (const, override));
    MOCK_METHOD(void, deletePreset, (const std::string&), (override));

    MOCK_METHOD(void, reload, (bool), (override));
};
}
