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

#include "shortcuts/ishortcutsregister.h"

namespace muse::shortcuts {
class ShortcutsRegisterMock : public IShortcutsRegister
{
public:
    MOCK_METHOD(const ShortcutList&, shortcuts, (), (const, override));
    MOCK_METHOD(Ret, setShortcuts, (const ShortcutList&), (override));
    MOCK_METHOD(void, resetShortcuts, (), (override));
    MOCK_METHOD(async::Notification, shortcutsChanged, (), (const, override));

    MOCK_METHOD(Ret, setAdditionalShortcuts, (const std::string&, const ShortcutList&), (override));

    MOCK_METHOD(const Shortcut&, shortcut, (const std::string&), (const, override));
    MOCK_METHOD(const Shortcut&, defaultShortcut, (const std::string&), (const, override));

    MOCK_METHOD(bool, isRegistered, (const std::string&), (const, override));
    MOCK_METHOD(ShortcutList, shortcutsForSequence, (const std::string&), (const, override));

    MOCK_METHOD(Ret, importFromFile, (const io::path_t&), (override));
    MOCK_METHOD(Ret, exportToFile, (const io::path_t&), (const, override));

    MOCK_METHOD(bool, active, (), (override));
    MOCK_METHOD(void, setActive, (bool), (override));
    MOCK_METHOD(async::Notification, activeChanged, (), (const, override));

    MOCK_METHOD(void, reload, (bool), (override));
};
}
