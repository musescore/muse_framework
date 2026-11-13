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

#include <gmock/gmock.h>

#include "audio/engine/internal/iaudiocontext.h"

namespace muse::audio::engine {
class AudioContextMock : public IAudioContext
{
public:
    MOCK_METHOD(AudioCtxId, id, (), (const, override));

    MOCK_METHOD(Ret, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));

    MOCK_METHOD(AudioResourceMetaList, availableInputResources, (), (const, override));
    MOCK_METHOD(SoundPresetList, availableSoundPresets, (const AudioResourceMeta&), (const, override));
    MOCK_METHOD(AudioResourceMetaList, availableOutputResources, (), (const, override));

    using AddTrackRet = RetVal2<TrackId, TrackParams>;
    MOCK_METHOD(AddTrackRet, addTrack, (const TrackName&, io::IODevice*, const TrackParams&), (override));
    MOCK_METHOD(AddTrackRet, addTrack, (const TrackName&, const mpe::PlaybackData&, const TrackParams&), (override));
    MOCK_METHOD(AddTrackRet, addAuxTrack, (const TrackName&, const TrackParams&), (override));

    MOCK_METHOD(void, removeTrack, (const TrackId), (override));
    MOCK_METHOD(void, removeAllTracks, (), (override));

    MOCK_METHOD(async::Channel<TrackId>, trackAdded, (), (const, override));
    MOCK_METHOD(async::Channel<TrackId>, trackRemoved, (), (const, override));

    MOCK_METHOD(RetVal<TrackIdList>, trackIdList, (), (const, override));
    MOCK_METHOD(RetVal<TrackName>, trackName, (const TrackId), (const, override));

    MOCK_METHOD(RetVal<TrackParams>, params, (const TrackId), (const, override));
    MOCK_METHOD(void, setSourceParams, (const TrackId, const AudioSourceParams&), (override));
    MOCK_METHOD(void, setControlParams, (const TrackId, const ControlParams&), (override));
    MOCK_METHOD(void, setFxChainParams, (const TrackId, const AudioFxChain&), (override));
    MOCK_METHOD(void, setAuxSendsParams, (const TrackId, const AuxSendsParams&), (override));

    using SourceParamsChannel = async::Channel<TrackId, AudioSourceParams>;
    using ControlParamsChannel = async::Channel<TrackId, ControlParams>;
    using FxChainChannel = async::Channel<TrackId, AudioFxChain>;
    using AuxSendsParamsChannel = async::Channel<TrackId, AuxSendsParams>;
    MOCK_METHOD(SourceParamsChannel, sourceParamsChanged, (), (const, override));
    MOCK_METHOD(ControlParamsChannel, controlParamsChanged, (), (const, override));
    MOCK_METHOD(FxChainChannel, fxChainParamsChanged, (), (const, override));
    MOCK_METHOD(AuxSendsParamsChannel, auxSendsParamsChanged, (), (const, override));

    MOCK_METHOD(void, processInput, (const TrackId), (const, override));
    MOCK_METHOD(RetVal<InputProcessingProgress>, inputProcessingProgress, (const TrackId), (const, override));

    MOCK_METHOD(void, clearCache, (const TrackId), (const, override));
    MOCK_METHOD(void, clearSources, (), (override));
    MOCK_METHOD(void, clearMasterOutputParams, (), (override));
    MOCK_METHOD(void, clearAllFx, (), (override));

    MOCK_METHOD(RetVal<AudioSignalChanges>, signalChanges, (const TrackId), (const, override));

    MOCK_METHOD(RetVal<AutomatedControlParamsChanges>, automatedControlParamsChanges, (const TrackId), (const, override));

    MOCK_METHOD(async::Promise<Ret>, prepareToPlay, (), (override));

    MOCK_METHOD(void, play, (const secs_t), (override));
    MOCK_METHOD(void, seek, (const secs_t, const bool), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, resume, (const secs_t), (override));

    MOCK_METHOD(void, setDuration, (const secs_t), (override));
    MOCK_METHOD(Ret, setLoop, (const secs_t, const secs_t), (override));
    MOCK_METHOD(void, resetLoop, (), (override));

    MOCK_METHOD(PlaybackStatus, playbackStatus, (), (const, override));
    MOCK_METHOD(async::Channel<PlaybackStatus>, playbackStatusChanged, (), (const, override));
    MOCK_METHOD(secs_t, playbackPosition, (), (const, override));
    MOCK_METHOD(async::Channel<secs_t>, playbackPositionChanged, (), (const, override));

    MOCK_METHOD(async::Promise<Ret>, saveSoundTrack, (io::IODevice&, const SoundTrackFormat&, const SoundTrackSaveOptions&), (override));
    MOCK_METHOD(SaveSoundTrackProgress, saveSoundTrackProgressChanged, (), (const, override));
    MOCK_METHOD(void, abortSavingAllSoundTracks, (), (override));
};
}
