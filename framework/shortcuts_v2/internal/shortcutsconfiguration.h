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
#pragma once

#include "async/asyncable.h"

#include "modularity/ioc.h"
#include "iglobalconfiguration.h"
#include "io/ifilesystem.h"

#include "ishortcutsconfiguration.h"

#include "global/types/config.h"

namespace muse::shortcuts {
class ShortcutsConfiguration : public IShortcutsConfiguration, public Contextable, public async::Asyncable
{
    GlobalInject<IGlobalConfiguration> globalConfiguration;
    GlobalInject<io::IFileSystem> fileSystem;

public:
    ShortcutsConfiguration(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();

    QString currentKeyboardLayout() const override;
    void setCurrentKeyboardLayout(const QString& layout) override;

    io::path_t shortcutsUserAppDataPath() const override;
    io::path_t shortcutsAppDataPath() const override;

    std::string defaultShortcutsName() const override;
    std::vector<std::string> availableShortcutsPresets() const override;

    std::string currentShortcutsPresetName() const override;
    void setCurrentShortcutsPresetName(const std::string& name) override;
    async::Channel<std::string> currentShortcutsPresetNameChanged() const override;

    io::path_t commandShortcutsUserAppDataPath(const std::string& shortcutsName) const override;
    io::path_t commandShortcutsAppDataPath(const std::string& shortcutsName) const override;

private:
    io::path_t userShortcutsDirPath() const;

    Config m_config;
    async::Channel<std::string> m_currentPresetNameChanged;
};
}
