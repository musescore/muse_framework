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

#include <atomic>

#include "global/modularity/ioc.h"
#include "global/iprocess.h"
#include "global/iglobalconfiguration.h"
#include "global/io/ifilesystem.h"
#include "interactive/iinteractive.h"
#include "global/async/asyncable.h"

#include "../iregisteraudiopluginsscenario.h"
#include "../iknownaudiopluginsregister.h"
#include "../iaudiopluginsscannerregister.h"
#include "../iaudiopluginmetareaderregister.h"
#include "../iaudiopluginsloadguard.h"

namespace muse::audioplugins {
class RegisterAudioPluginsScenario : public IRegisterAudioPluginsScenario, public Contextable, public async::Asyncable
{
public:
    GlobalInject<IGlobalConfiguration> globalConfiguration;
    GlobalInject<IProcess> process;
    GlobalInject<io::IFileSystem> fileSystem;
    GlobalInject<IKnownAudioPluginsRegister> knownPluginsRegister;
    GlobalInject<IAudioPluginsScannerRegister> scannerRegister;
    GlobalInject<IAudioPluginMetaReaderRegister> metaReaderRegister;
    GlobalInject<IAudioPluginsLoadGuard> loadGuard;
    ContextInject<IInteractive> interactive = { this };

public:
    RegisterAudioPluginsScenario(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();

    Ret markCrashedPluginsAsBroken();

    PluginScanResult scanPlugins(Progress* progress = nullptr) const override;

    Ret updatePluginsRegistry() override;
    Ret rescanAllPlugins() override;

    Ret registerNewPlugins(const io::paths_t& pluginPaths, bool validate) override;
    Ret unregisterRemovedPlugins(const PluginResourceIdList& pluginIds) override;

    Ret registerPlugin(const io::path_t& pluginPath) override;
    Ret validatePlugin(const io::path_t& pluginPath, const io::path_t& outputFile) override;

private:
    Ret persistDiscoveredPlaceholders(const io::paths_t& pluginPaths);
    void processPluginsRegistration(const io::paths_t& pluginPaths);
    AudioPluginInfoList scanResult(const io::path_t& pluginPath, const io::path_t& resultFile, int code) const;
    RetVal<AudioPluginInfoList> validatePluginInfo(const io::path_t& pluginPath) const;
    AudioPluginInfo makeFailedPluginInfo(const io::path_t& pluginPath, int failCode) const;
    io::path_t scanResultFilePath(int64_t index) const;
    IAudioPluginMetaReaderPtr metaReader(const io::path_t& pluginPath) const;
    PluginType metaType(const io::path_t& pluginPath) const;

    Progress m_progress;
    std::atomic_bool m_aborted = false;
};
}
