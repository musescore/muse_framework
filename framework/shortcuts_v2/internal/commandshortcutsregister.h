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

#include <unordered_map>

#include "../icommandshortcutsregister.h"

#include "global/async/asyncable.h"

#include "modularity/ioc.h"
#include "ishortcutsconfiguration.h"
#include "rcommand/icommandsregister.h"
#include "multiwindows/imultiwindowsprovider.h"

namespace muse::shortcuts {
class CommandShortcutsRegister : public ICommandShortcutsRegister, public async::Asyncable
{
    GlobalInject<IShortcutsConfiguration> configuration;
    GlobalInject<mi::IMultiWindowsProvider> multiwindowsProvider;
    GlobalInject<rcommand::ICommandsRegister> commandsRegister;

public:
    CommandShortcutsRegister() = default;
    ~CommandShortcutsRegister() override = default;

    void init();

    const ShortcutList& shortcuts() const override;
    Ret setShortcuts(const ShortcutList& shortcuts) override;
    void resetShortcuts() override;
    async::Notification shortcutsChanged() const override;

    Ret setAdditionalShortcuts(const std::string& context, const ShortcutList& shortcuts) override;

    ShortcutList shortcutsForSequence(const std::string& sequence) const override;
    const Shortcut& defaultShortcut(const std::string& command) const override;

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

private:

    std::string activeShortcutsName() const;
    io::path_t userShortcutsPath() const;

    void applyShortcutsDiff(const std::string& shortcutsName, ShortcutList& shortcuts) const;
    ShortcutList makeDiff(const ShortcutList& shortcuts, const ShortcutList& defaultShortcuts) const;

    bool removeUserFile();

    bool readFromFile(ShortcutList& shortcuts, const io::path_t& path) const;
    bool writeToFile(const ShortcutList& shortcuts, const io::path_t& path) const;

    void mergeShortcuts(ShortcutList& shortcuts, const ShortcutList& defaultShortcuts) const;
    void mergeAdditionalShortcuts(ShortcutList& shortcuts);

    void makeUnique(ShortcutList& shortcuts);

    ShortcutList filterAndUpdateAdditionalShortcuts(const ShortcutList& shortcuts);

    ShortcutList m_shortcuts;
    ShortcutList m_defaultShortcuts;
    std::unordered_map<std::string, ShortcutList> m_additionalShortcutsMap;
    async::Notification m_shortcutsChanged;
};
}
