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

#include "global/modularity/imoduleinterface.h"
#include "global/types/retval.h"
#include "mpe/events.h"
#include "../isynthesizer.h"
#include "nodes/audiosourcenode.h"
#include "nodes/fxchain.h"

namespace muse::audio::engine {
class IAudioFactory : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IAudioFactory)

public:
    virtual ~IAudioFactory() = default;

    // Available resources
    virtual AudioResourceMetaList availableInputResources() const = 0;
    virtual SoundPresetList availableSoundPresets(const AudioResourceMeta& resourceMeta) const = 0;
    virtual AudioResourceMetaList availableOutputResources() const = 0;

    // Make audio sources
    virtual RetVal<synth::ISynthesizerPtr> makeSynth(const TrackId trackId, const AudioInputParams& params,
                                                     const PlaybackSetupData& setupData) const = 0;

    virtual RetVal<synth::ISynthesizerPtr> makeDefaultSynth(const TrackId trackId) const = 0;

    //! NOTE For internal purposes,
    // created synth instances are registered in an internal registry (see VST).
    // This method clears this registry.
    virtual void clearSynthSources() = 0;

    virtual RetVal<AudioSourceNodePtr> makeEventSource(const TrackId trackId, const std::string& trackName,
                                                       const mpe::PlaybackData& playbackData, const AudioInputParams& params,
                                                       const std::function<void()> onOffStreamReceived = nullptr) const = 0;
    // Make FX
    virtual FxChainPtr makeMasterFxChain(const AudioFxChain& fxChain) const = 0;
    virtual FxChainPtr makeTrackFxChain(const TrackId trackId, const AudioFxChain& fxChain) const = 0;

    //! NOTE For internal purposes,
    // created effect instances are registered in an internal registry (see VST).
    // This method clears this registry.
    virtual void clearAllFx() = 0;
};
}
