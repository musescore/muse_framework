/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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
#include "audiopluginsconfiguration.h"

#include "settings.h"

using namespace muse;
using namespace muse::audioplugins;

static const std::string module_name("audioplugins");
static const Settings::Key NEED_SCAN_FOR_PLUGINS_ON_START(module_name, "application/audioplugins/scanOnStart");

void AudioPluginsConfiguration::init()
{
    settings()->setDefaultValue(NEED_SCAN_FOR_PLUGINS_ON_START, Val(true));
    settings()->valueChanged(NEED_SCAN_FOR_PLUGINS_ON_START).onReceive(nullptr, [this](const Val& val) {
        m_needScanForPluginsOnStartChanged.send(val.toBool());
    });
}

io::path_t AudioPluginsConfiguration::knownAudioPluginsFilePath() const
{
    return globalConfiguration()->userAppDataPath() + "/known_audio_plugins.json";
}

bool AudioPluginsConfiguration::needScanForPluginsOnStart() const
{
    const Val val = settings()->value(NEED_SCAN_FOR_PLUGINS_ON_START);
    return val.isNull() ? true : val.toBool();
}

void AudioPluginsConfiguration::setNeedScanForPluginsOnStart(bool need)
{
    settings()->setSharedValue(NEED_SCAN_FOR_PLUGINS_ON_START, Val(need));
}

async::Channel<bool> AudioPluginsConfiguration::needScanForPluginsOnStartChanged() const
{
    return m_needScanForPluginsOnStartChanged;
}
