/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include "modularity/imoduleinterface.h"
#include "global/types/retval.h"
#include "global/async/channel.h"
#include "global/async/promise.h"

#include "../common/audiotypes.h"

namespace muse::io {
class IODevice;
}

namespace muse::audio {
class ITracks;
class IPlayer;
class IAudioOutput;

class IPlayback : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IPlayback)

public:
    virtual ~IPlayback() = default;

    // Init
    virtual async::Promise<Ret> init() = 0;
    virtual bool isInited() const = 0;
    virtual async::Channel<bool> initedChanged() const = 0;
    virtual void deinit() = 0;

    // Resources
    virtual async::Promise<AudioResourceMetaList> availableInputResources() const = 0;
    virtual async::Promise<SoundPresetList> availableSoundPresets(const AudioResourceMeta& resourceMeta) const = 0;
    virtual async::Promise<AudioResourceMetaList> availableOutputResources() const = 0;

    // Setup tracks
    virtual async::Promise<TrackIdList> trackIdList() const = 0;
    virtual async::Promise<RetVal<TrackName> > trackName(const TrackId trackId) const = 0;

    virtual async::Promise<TrackId, TrackParams> addTrack(const TrackName& name, io::IODevice* data,
                                                          const TrackParams& params) = 0;
    virtual async::Promise<TrackId, TrackParams> addTrack(const TrackName& name, const mpe::PlaybackData& data,
                                                          const TrackParams& params) = 0;

    virtual async::Promise<TrackId, TrackParams> addAuxTrack(const TrackName& trackName, const TrackParams& params) = 0;

    virtual void removeTrack(const TrackId trackId) = 0;
    virtual void removeAllTracks() = 0;

    virtual async::Channel<TrackId> trackAdded() const = 0;
    virtual async::Channel<TrackId> trackRemoved() const = 0;

    // Params
    // Get all params
    virtual async::Promise<TrackParams> params(const TrackId trackId) const = 0;

    // Set some params
    virtual void setSourceParams(const TrackId trackId, const AudioSourceParams& params) = 0;
    virtual void setControlParams(const TrackId trackId, const ControlParams& params) = 0;
    virtual void setFxChainParams(const TrackId trackId, const AudioFxChain& params) = 0;
    virtual void setAuxSendsParams(const TrackId trackId, const AuxSendsParams& params) = 0;

    // These parameters can be changed within the audio system.
    virtual async::Channel<TrackId, AudioSourceParams> sourceParamsChanged() const = 0;
    virtual async::Channel<TrackId, AudioFxChain> fxChainParamsChanged() const = 0;

    // Same for master
    virtual async::Promise<TrackParams> masterParams() const = 0;
    virtual void setMasterControlParams(const ControlParams& params) = 0;
    virtual void setMasterFxChainParams(const AudioFxChain& params) = 0;
    virtual void setMasterAuxSendsParams(const AuxSendsParams& params) = 0;
    virtual async::Channel<AudioFxChain> masterFxChainParamsChanged() const = 0;

    // Input processing
    virtual void processInput(const TrackId trackId) const = 0;
    virtual async::Promise<InputProcessingProgress> inputProcessingProgress(const TrackId trackId) const = 0;

    virtual void clearCache(const TrackId trackId) const = 0;
    virtual void clearSources() = 0;
    virtual void clearMasterOutputParams() = 0;
    virtual void clearAllFx() = 0;

    // Play
    virtual std::shared_ptr<IPlayer> player() const = 0;

    // Signal changes
    virtual async::Promise<AudioSignalChanges> signalChanges(const TrackId trackId) const = 0;
    virtual async::Promise<AudioSignalChanges> masterSignalChanges() const = 0;

    // Export
    virtual async::Promise<bool> saveSoundTrack(const SoundTrackFormat& format, io::IODevice& dstDevice) = 0;
    virtual void abortSavingAllSoundTracks() = 0;
    virtual SaveSoundTrackProgress saveSoundTrackProgressChanged() const = 0;
};

using IPlaybackPtr = std::shared_ptr<IPlayback>;
}
