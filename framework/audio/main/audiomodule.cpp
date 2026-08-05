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
#include "audiomodule.h"

#include "common/iaudiotaskscheduler.h"
#include "global/modularity/ioc.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"

#include "audio/common/audiosanitizer.h"
#include "audio/common/audiothreadsecurer.h"
#ifdef Q_OS_WASM
#include "audio/common/rpc/platform/web/webrpcchannel.h"
#include "platform/web/websoundfontcontroller.h"
#else
#include "audio/common/rpc/platform/general/generalrpcchannel.h"
#include "platform/general/generalsoundfontcontroller.h"
#include "platform/general/generalsoundfontinstallscenario.h"
#endif

#include "internal/audioconfiguration.h"
#include "internal/audioactionscontroller.h"
#include "internal/audiocommandsregister.h"
#include "internal/audiocommandsstate.h"
#include "internal/transporteventscontroller.h"
#include "internal/startaudiocontroller.h"
#include "internal/playback.h"
#include "internal/audiodrivercontroller.h"

#include "audio/engine/enginesetup.h"

#include "diagnostics/idiagnosticspathsregister.h"

#include "log.h"

using namespace muse;
using namespace muse::modularity;
using namespace muse::audio;

AudioModule::AudioModule()
{
    AudioSanitizer::setupMainThread();
}

static const std::string mname("audio");

std::string AudioModule::moduleName() const
{
    return mname;
}

void AudioModule::registerExports()
{
    m_configuration = std::make_shared<AudioConfiguration>();
    m_audioDriverController = std::make_shared<AudioDriverController>();

    m_engineGlobalSetup = std::make_shared<engine::EngineGlobalSetup>();
    m_engineGlobalSetup->registerExports();

#ifdef Q_OS_WASM
    m_rpcChannel = std::make_shared<rpc::WebRpcChannel>();
    m_soundFontController = std::make_shared<WebSoundFontController>();
#else
    m_rpcChannel = std::make_shared<rpc::GeneralRpcChannel>();
    m_soundFontController = std::make_shared<GeneralSoundFontController>();
#endif

    m_startAudioController = std::make_shared<StartAudioController>(m_rpcChannel);

    globalIoc()->registerExport<IAudioConfiguration>(mname, m_configuration);
    globalIoc()->registerExport<IAudioThreadSecurer>(mname, std::make_shared<AudioThreadSecurer>());
    globalIoc()->registerExport<rpc::IRpcChannel>(mname, m_rpcChannel);
    globalIoc()->registerExport<IAudioDriverController>(mname, m_audioDriverController);
    globalIoc()->registerExport<IAudioTaskScheduler>(mname, m_audioDriverController->getAudioTaskScheduler());
    globalIoc()->registerExport<ISoundFontController>(mname, m_soundFontController);
    globalIoc()->registerExport<IStartAudioController>(mname, m_startAudioController);
}

void AudioModule::resolveImports()
{
    m_engineGlobalSetup->resolveImports();

    auto cr = globalIoc()->resolve<muse::rcommand::ICommandsRegister>(mname);
    if (cr) {
        cr->reg(std::make_shared<AudioCommandsRegister>());
    }
}

void AudioModule::onInit(const IApplication::RunMode& mode)
{
    m_configuration->init();

    if (mode == IApplication::RunMode::AudioPluginRegistration) {
        return;
    }

    // rpc
    m_rpcChannel->setupOnMain();
#ifndef Q_OS_WASM
    m_rpcTicker.start(1, [this]() {
        m_rpcChannel->process();
    }, Ticker::Mode::Repeat);
#endif

    m_startAudioController->init();

#ifndef Q_OS_WASM
    m_startAudioController->startAudioProcessing(mode);
#endif

    //! --- Diagnostics ---
    auto pr = globalIoc()->resolve<muse::diagnostics::IDiagnosticsPathsRegister>(mname);
    if (pr) {
        std::vector<io::path_t> paths = m_configuration->soundFontDirectories();
        for (const io::path_t& p : paths) {
            pr->reg("soundfonts", p);
        }
    }
}

void AudioModule::onDeinit()
{
    m_rpcTicker.stop();
    m_startAudioController->stopAudioProcessing();
    m_engineGlobalSetup->onDeinit();
}

modularity::IContextSetup* AudioModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new AudioContext(ctx);
}

// Context

void AudioContext::registerExports()
{
    m_actionsController = std::make_shared<AudioActionsController>(iocContext());
    m_transportEventsController = std::make_shared<TransportEventsController>(iocContext());

#ifndef Q_OS_WASM
    ioc()->registerExport<ISoundFontInstallScenario>(mname, new GeneralSoundFontInstallScenario(iocContext()));
#endif

    ioc()->registerExport<IPlayback>(mname, new Playback(iocContext()));
}

void AudioContext::resolveImports()
{
    auto cs = ioc()->resolve<muse::rcommand::ICommandsState>(mname);
    if (cs) {
        cs->reg(std::make_shared<AudioCommandsState>(iocContext(), m_actionsController));
    }
}

void AudioContext::onInit(const IApplication::RunMode& mode)
{
    if (mode == IApplication::RunMode::AudioPluginRegistration) {
        return;
    }

    m_audioInited = true;

    m_actionsController->init();
    m_transportEventsController->init();
}

void AudioContext::onDeinit()
{
    m_transportEventsController->deinit();
}
