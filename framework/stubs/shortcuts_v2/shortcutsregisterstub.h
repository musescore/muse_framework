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
#ifndef MUSE_SHORTCUTS_SHORTCUTSREGISTERSTUB_H
#define MUSE_SHORTCUTS_SHORTCUTSREGISTERSTUB_H

#include "shortcuts_v2/icommandshortcutsregister.h"

namespace muse::shortcuts {
class ShortcutsRegisterStub : public ICommandShortcutsRegister
{
public:
    const ShortcutList& shortcuts() const override;
    Ret updateShortcuts(const ShortcutList& shortcuts) override;
    void resetShortcuts() override;
    async::Notification shortcutsChanged() const override;

    Ret setAdditionalShortcuts(const std::string& context, const ShortcutList& shortcuts) override;

    ShortcutList shortcutsForSequence(const std::string& sequence) const override;
    const Shortcut& shortcut(const rcommand::Command& command) const override;
    const Shortcut& defaultShortcut(const rcommand::Command& command) const override;

    Ret importFromFile(const io::path_t& filePath) override;
    Ret exportToFile(const io::path_t& filePath) const override;

    std::vector<std::string> availablePresets() const override;

    std::string currentPresetName() const override;
    void setCurrentPresetName(const std::string& presetName) override;
    async::Channel<std::string> currentPresetNameChanged() const override;

    bool isPresetEdited(const std::string& presetName) const override;
    bool canDeletePreset(const std::string& presetName) const override;
    void deletePreset(const std::string& presetName) override;

    // for testflow tests
    void reload(bool onlyDef = false) override;
};
}

#endif // MUSE_SHORTCUTS_SHORTCUTSREGISTERSTUB_H
