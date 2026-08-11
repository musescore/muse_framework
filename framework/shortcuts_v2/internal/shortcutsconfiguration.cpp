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
#include "shortcutsconfiguration.h"

#include "settings.h"
#include "io/path.h"

#include "global/configreader.h"

#include "log.h"

using namespace muse;
using namespace muse::shortcuts;

static const muse::Settings::Key CURRENT_SHORTCUTS_PRESET_KEY("shortcuts", "shortcuts/currentPreset");

void ShortcutsConfiguration::init()
{
    m_config = ConfigReader::read(":/configs/shortcuts.cfg");

    settings()->setDefaultValue(CURRENT_SHORTCUTS_PRESET_KEY, Val(""));
    settings()->valueChanged(CURRENT_SHORTCUTS_PRESET_KEY).onReceive(this, [this](const Val& name) {
        m_currentPresetNameChanged.send(name.toString());
    });

    fileSystem()->makePath(userShortcutsDirPath());
}

QString ShortcutsConfiguration::currentKeyboardLayout() const
{
    NOT_IMPLEMENTED;
    return "US-QWERTY";
}

void ShortcutsConfiguration::setCurrentKeyboardLayout(const QString& layout)
{
    UNUSED(layout);
    NOT_IMPLEMENTED;
    return;
}

io::path_t ShortcutsConfiguration::shortcutsUserAppDataPath() const
{
    return globalConfiguration()->userAppDataPath() + "/shortcuts.xml";
}

io::path_t ShortcutsConfiguration::shortcutsAppDataPath() const
{
#if defined(Q_OS_MACOS)
    return m_config.value("shortcuts_mac").toPath();
#else
    return m_config.value("shortcuts").toPath();
#endif
}

std::string ShortcutsConfiguration::defaultShortcutsName() const
{
#if defined(Q_OS_MACOS)
    return "shortcuts_mac";
#else
    return "shortcuts";
#endif
}

std::vector<std::string> ShortcutsConfiguration::availableShortcutsPresets() const
{
    std::vector<std::string> result;

    ValList presets = m_config.value("command_shortcuts_presets").toList();
    for (const Val& preset : presets) {
        result.push_back(preset.toString());
    }

    return result;
}

std::string ShortcutsConfiguration::currentShortcutsPresetName() const
{
    return settings()->value(CURRENT_SHORTCUTS_PRESET_KEY).toString();
}

void ShortcutsConfiguration::setCurrentShortcutsPresetName(const std::string& name)
{
    settings()->setSharedValue(CURRENT_SHORTCUTS_PRESET_KEY, Val(name));
}

async::Channel<std::string> ShortcutsConfiguration::currentShortcutsPresetNameChanged() const
{
    return m_currentPresetNameChanged;
}

io::path_t ShortcutsConfiguration::userShortcutsDirPath() const
{
    return globalConfiguration()->userAppDataPath() + "/shortcuts";
}

io::path_t ShortcutsConfiguration::commandShortcutsUserAppDataPath(const std::string& shortcutsName) const
{
    return userShortcutsDirPath() + "/" + shortcutsName + ".json";
}

io::path_t ShortcutsConfiguration::commandShortcutsAppDataPath(const std::string& shortcutsName) const
{
    return m_config.value("command_" + shortcutsName).toPath();
}
