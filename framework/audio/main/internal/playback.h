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

#include "../iplayback.h"
#include "global/async/asyncable.h"
#include "global/types/retval.h"

#include "modularity/ioc.h"
#include "common/rpc/irpcchannel.h"
#include "../istartaudiocontroller.h"

#include "../iplayer.h"

namespace muse::audio {
//! NOTE It may not use contextual services,
// but the playback itself is contextual,
// because each context (window) has its own playback.
class Playback : public IPlayback, public async::Asyncable, public Contextable
{
    GlobalInject<IStartAudioController> startAudioController;
    GlobalInject<rpc::IRpcChannel> channel;

public:
    Playback(const muse::modularity::ContextPtr& ctx)
        : Contextable(ctx) {}

    // Init
    async::Promise<Ret> init() override;
    bool isInited() const override;
    async::Channel<bool> initedChanged() const override;
    void deinit() override;

    // Resources
    async::Promise<AudioResourceMetaList> availableInputResources() const override;
    async::Promise<SoundPresetList> availableSoundPresets(const AudioResourceMeta& resourceMeta) const override;
    async::Promise<AudioResourceMetaList> availableOutputResources() const override;

    // Setup tracks
    async::Promise<TrackIdList> trackIdList() const override;
    async::Promise<RetVal<TrackName> > trackName(const TrackId trackId) const override;

    async::Promise<TrackId, TrackParams> addTrack(const TrackName& name, io::IODevice* data,
                                                  const TrackParams& params) override;
    async::Promise<TrackId, TrackParams> addTrack(const TrackName& name, const mpe::PlaybackData& data,
                                                  const TrackParams& params) override;
    async::Promise<TrackId, TrackParams> addAuxTrack(const TrackName& name, const TrackParams& params) override;

    void removeTrack(const TrackId trackId) override;
    void removeAllTracks() override;

    async::Channel<TrackId> trackAdded() const override;
    async::Channel<TrackId> trackRemoved() const override;

    // Params
    // Get all params
    async::Promise<TrackParams> params(const TrackId trackId) const override;

    // Set some params
    void setSourceParams(const TrackId trackId, const AudioSourceParams& params) override;
    void setControlParams(const TrackId trackId, const ControlParams& params) override;
    void setFxChainParams(const TrackId trackId, const AudioFxChain& params) override;
    void setAuxSendsParams(const TrackId trackId, const AuxSendsParams& params) override;

    // These parameters can be changed within the audio system.
    async::Channel<TrackId, AudioSourceParams> sourceParamsChanged() const override;
    async::Channel<TrackId, AudioFxChain> fxChainParamsChanged() const override;

    // Same for master
    async::Promise<TrackParams> masterParams() const override;
    void setMasterControlParams(const ControlParams& params) override;
    void setMasterFxChainParams(const AudioFxChain& params) override;
    void setMasterAuxSendsParams(const AuxSendsParams& params) override;
    async::Channel<AudioFxChain> masterFxChainParamsChanged() const override;

    // Input processing
    void processInput(const TrackId trackId) const override;
    async::Promise<InputProcessingProgress> inputProcessingProgress(const TrackId trackId) const override;

    // Clear cache
    void clearCache(const TrackId trackId) const override;
    void clearSources() override;
    void clearMasterOutputParams() override;
    void clearAllFx() override;

    // Play
    IPlayerPtr player() const override;

    // Signal changes
    async::Promise<AudioSignalChanges> signalChanges(const TrackId trackId) const override;
    async::Promise<AudioSignalChanges> masterSignalChanges() const override;

    // Export
    async::Promise<bool> saveSoundTrack(const SoundTrackFormat& format, io::IODevice& dstDevice) override;
    void abortSavingAllSoundTracks() override;
    SaveSoundTrackProgress saveSoundTrackProgressChanged() const override;

private:

    rpc::CtxId ctxId() const;

    ValCh<bool> m_inited;

    async::Channel<TrackId> m_trackAdded;
    async::Channel<TrackId> m_trackRemoved;
    async::Channel<TrackId, AudioSourceParams> m_sourceParamsChanged;
    async::Channel<TrackId, AudioFxChain> m_fxChainParamsChanged;
    async::Channel<AudioFxChain> m_masterFxChainParamsChanged;

    mutable bool m_saveSoundTrackProgressStreamInited = false;
    async::Channel<int64_t, int64_t, SaveSoundTrackStage> m_saveSoundTrackProgressStream;
    mutable rpc::StreamId m_saveSoundTrackProgressStreamId = 0;
};
}
