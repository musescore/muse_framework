/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include "iaudiocontext.h"
#include "nodes/audionode.h"

#include "global/async/asyncable.h"

#include "modularity/ioc.h"
#include "iaudiofactory.h"
#include "../iaudioengineconfiguration.h"

#include "iexecoperation.h"
#include "igettracksource.h"
#include "mixer.h"
#include "nodes/playheadnode.h"
#include "nodes/trackchain.h"

namespace muse::audio {
struct AudioContextTag
{
    static constexpr const char* name = "AudioContext";
};
}

namespace muse::audio::engine {
class ContextPlayer;
class AudioContext : public AudioNode<AudioContextTag>, public IAudioContext, public IGetTrackSource,
    public async::Asyncable
{
    GlobalInject<IAudioFactory> audioFactory;
    GlobalInject<IAudioEngineConfiguration> configuration;

public:
    AudioContext(const AudioCtxId& ctxId, IExecOperation* execOperation);

    AudioCtxId id() const override;

    Ret init() override;
    void deinit() override;

    // Resources
    AudioResourceMetaList availableInputResources() const override;
    SoundPresetList availableSoundPresets(const AudioResourceMeta& resourceMeta) const override;
    AudioResourceMetaList availableOutputResources() const override;

    // Tracks
    RetVal2<TrackId, TrackParams> addTrack(const std::string& trackName, io::IODevice* playbackData,
                                           const TrackParams& params) override;
    RetVal2<TrackId, TrackParams> addTrack(const std::string& trackName, const mpe::PlaybackData& playbackData,
                                           const TrackParams& params) override;
    RetVal2<TrackId, TrackParams> addAuxTrack(const std::string& trackName, const TrackParams& params) override;

    void removeTrack(const TrackId trackId) override;
    void removeAllTracks() override;

    async::Channel<TrackId> trackAdded() const override;
    async::Channel<TrackId> trackRemoved() const override;

    RetVal<TrackIdList> trackIdList() const override;
    RetVal<TrackName> trackName(const TrackId trackId) const override;

    // Params
    RetVal<TrackParams> params(const TrackId trackId) const override;

    void setSourceParams(const TrackId trackId, const AudioSourceParams& params) override;
    void setControlParams(const TrackId trackId, const ControlParams& params) override;
    void setFxChainParams(const TrackId trackId, const AudioFxChain& params) override;
    void setAuxSendsParams(const TrackId trackId, const AuxSendsParams& params) override;

    async::Channel<TrackId, AudioSourceParams> sourceParamsChanged() const override;
    async::Channel<TrackId, AudioFxChain> fxChainParamsChanged() const override;

    // Input processing
    void processInput(const TrackId trackId) const override;
    RetVal<InputProcessingProgress> inputProcessingProgress(const TrackId trackId) const override;

    // Clear
    void clearCache(const TrackId trackId) const override;
    void clearSources() override;
    void clearMasterOutputParams() override;
    void clearAllFx() override;

    // Signals
    RetVal<AudioSignalChanges> signalChanges(const TrackId trackId) const override;

    // Play
    async::Promise<Ret> prepareToPlay() override;

    void play(const secs_t delay = 0.0) override;
    void seek(const secs_t newPosition, const bool flushSound = true) override;
    void stop() override;
    void pause() override;
    void resume(const secs_t delay = 0.0) override;

    void setDuration(const secs_t duration) override;
    Ret setLoop(const secs_t from, const secs_t to) override;
    void resetLoop() override;

    PlaybackStatus playbackStatus() const override;
    async::Channel<PlaybackStatus> playbackStatusChanged() const override;
    secs_t playbackPosition() const override;
    async::Channel<secs_t> playbackPositionChanged() const override;

    // Export
    async::Promise<Ret> saveSoundTrack(io::IODevice& dstDevice, const SoundTrackFormat& format) override;
    SaveSoundTrackProgress saveSoundTrackProgressChanged() const override;
    void abortSavingAllSoundTracks() override;

private:

    enum class TrackType {
        Undefined = -1,
        Master_track,
        Event_track,
        Sound_track,
        Aux_track
    };

    struct Track
    {
        TrackId id = INVALID_TRACK_ID;
        TrackType type = TrackType::Undefined;
        TrackName name;
        TrackParams params;
        TrackChainPtr chain;
    };

    void onOutputSpecChanged(const OutputSpec& spec) override;
    void onModeChanged(const ProcessMode mode) override;
    void onSourceParamsChanged(Track& track, const AudioSourceParams& params);
    void onControlParamsChanged(Track& track, const ControlParams& params);
    void onFxChainParamsChanged(Track& track, const AudioFxChain& params);
    void onAuxSendsParamsChanged(Track& track, const AuxSendsParams& params);

    TrackId newTrackId() const;
    void doAddTrack(const Track& track);
    const Track* track(const TrackId id) const;
    Track* track(const TrackId id);

    // Processing
    void doSelfProcess(float* buffer, samples_t samplesPerChannel) override;

    // IGetTrackSource
    AudioSourceNodePtr trackSource(const TrackId trackId) const override;
    std::vector<AudioSourceNodePtr> allTracksSources() const override;
    // -----

    void updateNonMutedTrackCount();

    void listenInputProcessing(std::function<void(const Ret&)> completed);
    bool hasPendingChunks(const TrackId id) const;
    size_t tracksBeingProcessedCount() const;
    Ret doSaveSoundTrack(io::IODevice& dstDevice, const SoundTrackFormat& format);

    AudioCtxId m_ctxId = 0;
    IExecOperation* m_execOperation = nullptr;

    std::shared_ptr<ContextPlayer> m_player;
    PlayheadNodePtr m_playheadNode;
    std::shared_ptr<Mixer> m_mixer;

    Track m_masterTrack;
    std::vector<Track> m_tracks;
    async::Channel<TrackId> m_trackAdded;
    async::Channel<TrackId> m_trackRemoved;

    async::Channel<TrackId, AudioSourceParams> m_sourceParamsChanged;
    async::Channel<TrackId, AudioFxChain> m_fxChainParamsChanged;

    // -----
    void onShouldProcessDuringSilenceChanged(const TrackId trackId, bool shouldProcess);
    TrackId m_prevActiveTrackId = INVALID_TRACK_ID;
    std::unordered_set<TrackId> m_tracksToProcessWhenIdle;
    // -----

    struct SaveSoundTrackProgressData {
        SaveSoundTrackProgress progress;
        async::Notification aborted;
    };
    SaveSoundTrackProgressData m_saveSoundTracksProgress;
};
}
