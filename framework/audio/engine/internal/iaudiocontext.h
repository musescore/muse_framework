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

#include "global/types/retval.h"
#include "global/async/promise.h"
#include "global/async/channel.h"
#include "audio/common/audiotypes.h"

namespace muse::audio::engine {
class IAudioContext
{
public:
    virtual ~IAudioContext() = default;

    virtual AudioCtxId id() const = 0;

    // Init
    virtual Ret init() = 0;
    virtual void deinit() = 0;

    // Resources
    virtual AudioResourceMetaList availableInputResources() const = 0;
    virtual SoundPresetList availableSoundPresets(const AudioResourceMeta& resourceMeta) const = 0;
    virtual AudioResourceMetaList availableOutputResources() const = 0;

    // Tracks
    virtual RetVal2<TrackId, TrackParams> addTrack(const TrackName& trackName, io::IODevice* playbackData, const TrackParams& params) = 0;
    virtual RetVal2<TrackId, TrackParams> addTrack(const TrackName& trackName, const mpe::PlaybackData& playbackData,
                                                   const TrackParams& params) = 0;
    virtual RetVal2<TrackId, TrackParams> addAuxTrack(const TrackName& trackName, const TrackParams& params) = 0;

    virtual void removeTrack(const TrackId trackId) = 0;
    virtual void removeAllTracks() = 0;

    virtual async::Channel<TrackId> trackAdded() const = 0;
    virtual async::Channel<TrackId> trackRemoved() const = 0;

    virtual RetVal<TrackIdList> trackIdList() const = 0;
    virtual RetVal<TrackName> trackName(const TrackId trackId) const = 0;

    // Params
    virtual RetVal<TrackParams> params(const TrackId trackId) const = 0;
    virtual void setSourceParams(const TrackId trackId, const AudioSourceParams& params) = 0;
    virtual void setControlParams(const TrackId trackId, const ControlParams& params) = 0;
    virtual void setFxChainParams(const TrackId trackId, const AudioFxChain& params) = 0;
    virtual void setAuxSendsParams(const TrackId trackId, const AuxSendsParams& params) = 0;

    virtual async::Channel<TrackId, AudioSourceParams> sourceParamsChanged() const = 0;
    virtual async::Channel<TrackId, AudioFxChain> fxChainParamsChanged() const = 0;

    // Input processing
    virtual void processInput(const TrackId trackId) const = 0;
    virtual RetVal<InputProcessingProgress> inputProcessingProgress(const TrackId trackId) const = 0;

    // Clear
    virtual void clearCache(const TrackId trackId) const = 0;
    virtual void clearSources() = 0;
    virtual void clearMasterOutputParams() = 0;
    virtual void clearAllFx() = 0;

    // Signals
    virtual RetVal<AudioSignalChanges> signalChanges(const TrackId trackId) const = 0;

    // Automation
    virtual RetVal<AutomatedControlParamsChanges> automatedControlParamsChanges(const TrackId trackId) const = 0;

    // Play
    virtual async::Promise<Ret> prepareToPlay() = 0;

    virtual void play(const secs_t delay = 0.0) = 0;
    virtual void seek(const secs_t newPosition, const bool flushSound = true) = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume(const secs_t delay = 0.0) = 0;

    virtual void setDuration(const secs_t duration) = 0;
    virtual Ret setLoop(const secs_t from, const secs_t to) = 0;
    virtual void resetLoop() = 0;

    virtual PlaybackStatus playbackStatus() const = 0;
    virtual async::Channel<PlaybackStatus> playbackStatusChanged() const = 0;
    virtual secs_t playbackPosition() const = 0;
    virtual async::Channel<secs_t> playbackPositionChanged() const = 0;

    // Export
    virtual async::Promise<Ret> saveSoundTrack(io::IODevice& dstDevice, const SoundTrackFormat& format) = 0;
    virtual SaveSoundTrackProgress saveSoundTrackProgressChanged() const = 0;
    virtual void abortSavingAllSoundTracks() = 0;
};
}
