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
#include "audiocontext.h"

#include "audio/common/audiosanitizer.h"
#include "audio/common/audioerrors.h"
#include "audio/common/audioutils.h"

#include "nodes/trackchain.h"

#include "contextplayer.h"

#include "muse_framework_config.h"
#ifdef MUSE_MODULE_AUDIO_EXPORT
#include "export/soundtrackwriter.h"
#endif

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;

AudioContext::AudioContext(const AudioCtxId& ctxId, IExecOperation* execOperation)
    : m_ctxId(ctxId)
    , m_execOperation(execOperation)
{
    m_player = std::make_shared<ContextPlayer>(this, execOperation);

    m_playheadNode = std::make_shared<PlayheadNode>(std::static_pointer_cast<IPlayhead>(m_player));
    m_mixer = std::make_shared<Mixer>();

    m_masterTrack.id = MASTER_TRACK_ID;
    m_masterTrack.type = TrackType::Master_track;
    m_masterTrack.name = "Master";
    m_masterTrack.chain = std::make_shared<TrackChain>(MASTER_TRACK_ID, "Master");
}

AudioCtxId AudioContext::id() const
{
    return m_ctxId;
}

Ret AudioContext::init()
{
    m_mixer->init();

    // Make the chain: audiocontext <- playheadnode
    // <- mastertrackchain <- mastersignalnode <- mastercontrolnode <- masterfxchain
    // <- mixer
    AutomationControlNodePtr masterControlNode = std::make_shared<AutomationControlNode>();
    masterControlNode->setPlayheadPosition(std::static_pointer_cast<IPlayheadPosition>(m_player));

    m_masterTrack.chain->setSource(m_mixer);
    m_masterTrack.chain->setFxChain(nullptr); //!< NOTE Master fx chain is not exists yet
    m_masterTrack.chain->setControl(masterControlNode);
    m_masterTrack.chain->setSignal(std::make_shared<SignalNode>());
    m_masterTrack.chain->rebuild();

    m_masterTrack.chain->connect(m_playheadNode);

    LOGD() << "Master track chain: " << m_playheadNode->dump();

    m_player->isActiveChanged().onReceive(this, [this](bool isActive) {
        setMode(isActive ? ProcessMode::Playing : ProcessMode::Idle);
    });

    return make_ret(Ret::Code::Ok);
}

void AudioContext::deinit()
{
    ONLY_AUDIO_ENGINE_THREAD;

    removeAllTracks();

    m_player.reset();

    // Explicitly disconnect and clear all channel members before
    // async_disconnectAll() and before the destructor runs. This ensures
    // subscribers are disconnected while they're still alive, not during IoC
    // teardown when some may already be destroyed.
    m_trackRemoved = async::Channel<TrackId>();
    m_trackAdded = async::Channel<TrackId>();

    m_saveSoundTracksProgress = SaveSoundTrackProgressData();
    m_sourceParamsChanged = async::Channel<TrackId, AudioSourceParams>();
    m_fxChainParamsChanged = async::Channel<TrackId, AudioFxChain>();

    async_disconnectAll();
}

// Config
void AudioContext::onModeChanged(const ProcessMode mode)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (mode == ProcessMode::Idle) {
        m_mixer->setTracksToProcessWhenIdle(m_tracksToProcessWhenIdle);
    }

    //! NOTE will be set on all nodes in the chain
    m_playheadNode->setMode(mode);
}

void AudioContext::onOutputSpecChanged(const OutputSpec& outputSpec)
{
    ONLY_AUDIO_ENGINE_THREAD;

    //! NOTE will be set on all nodes in the chain
    m_playheadNode->setOutputSpec(outputSpec);

    TimePosition currentPosition = m_player->currentPosition();
    if (currentPosition.isValid()) {
        m_player->seek(TimePosition::fromTime(currentPosition.time(), outputSpec.sampleRate));
    } else {
        m_player->seek(TimePosition::zero(outputSpec.sampleRate));
    }
}

// Setup tracks
TrackId AudioContext::newTrackId() const
{
    static TrackId lastId = MASTER_TRACK_ID;
    ++lastId;
    return lastId;
}

RetVal2<TrackId, TrackParams> AudioContext::addTrack(const std::string& trackName,
                                                     io::IODevice* playbackData,
                                                     const TrackParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;

    using RetType = RetVal2<TrackId, TrackParams>;

    if (!playbackData) {
        return RetType::make_ret(Err::InvalidAudioFilePath);
    }

    TrackId trackId = newTrackId();
    Track track;
    track.type = TrackType::Sound_track;
    track.id = trackId;
    track.name = trackName;
    track.params = params;
    //! NOT IMPLEMENTED YET
    // track.chain = ...

    doAddTrack(track);

    return RetType::make_ok(trackId, { });
}

RetVal2<TrackId, TrackParams> AudioContext::addTrack(const std::string& trackName,
                                                     const mpe::PlaybackData& playbackData,
                                                     const TrackParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;

    using RetType = RetVal2<TrackId, TrackParams>;

    if (!playbackData.setupData.isValid()) {
        return RetType::make_ret(Err::InvalidSetupData);
    }

    TrackId trackId = newTrackId();

    auto onOffStreamReceived = [this, trackId]() {
        std::unordered_set<TrackId> tracksToProcess = m_tracksToProcessWhenIdle;

        if (m_prevActiveTrackId == INVALID_TRACK_ID) {
            tracksToProcess.insert(trackId);
        } else {
            tracksToProcess.insert({ m_prevActiveTrackId, trackId });
        }

        m_mixer->setTracksToProcessWhenIdle(tracksToProcess);
        m_prevActiveTrackId = trackId;
    };

    // Make source
    RetVal<AudioSourceNodePtr> source = audioFactory()->makeEventSource(trackId, playbackData, params.source, onOffStreamReceived);
    if (!source.ret) {
        return RetType::make_ret(source.ret);
    }

    AutomationControlNodePtr controlNode = std::make_shared<AutomationControlNode>();
    controlNode->setPlayheadPosition(std::static_pointer_cast<IPlayheadPosition>(m_player));

    TrackChainPtr trackChain = std::make_shared<TrackChain>(trackId, trackName);
    trackChain->setOutputSpec(outputSpec());
    trackChain->setMode(mode());
    trackChain->setSource(source.val);
    trackChain->setFxChain(nullptr); // will be added later
    trackChain->setControl(controlNode);
    trackChain->setSignal(std::make_shared<SignalNode>());
    trackChain->rebuild();

    Ret ret = m_mixer->addTrack(trackChain, params.auxSends);
    if (!ret) {
        return RetType::make_ret(ret);
    }

    // Make track info
    Track track;
    track.type = TrackType::Event_track;
    track.id = trackId;
    track.name = trackName;
    track.params = params;
    track.chain = trackChain;

    track.params.source = source.val->inputParams();

    onFxChainParamsChanged(track, params.fxChain);
    onControlParamsChanged(track, params.control);

    doAddTrack(track);

    return RetType::make_ok(trackId, track.params);
}

RetVal2<TrackId, TrackParams> AudioContext::addAuxTrack(const std::string& trackName,
                                                        const TrackParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;

    using RetType = RetVal2<TrackId, TrackParams>;

    TrackId trackId = newTrackId();

    AutomationControlNodePtr controlNode = std::make_shared<AutomationControlNode>();
    controlNode->setPlayheadPosition(std::static_pointer_cast<IPlayheadPosition>(m_player));

    TrackChainPtr trackChain = std::make_shared<TrackChain>(trackId, trackName);
    trackChain->setOutputSpec(outputSpec());
    trackChain->setMode(mode());
    trackChain->setFxChain(nullptr); // will be added later
    trackChain->setControl(controlNode);
    trackChain->setSignal(std::make_shared<SignalNode>());
    trackChain->rebuild();

    Ret ret = m_mixer->addAuxTrack(trackChain);
    if (!ret) {
        return RetType::make_ret(ret);
    }

    // Make track info
    Track track;
    track.type = TrackType::Aux_track;
    track.id = trackId;
    track.name = trackName;
    track.params = params;
    track.chain = trackChain;

    onFxChainParamsChanged(track, params.fxChain);
    onControlParamsChanged(track, params.control);

    doAddTrack(track);

    return RetType::make_ok(trackId, track.params);
}

void AudioContext::doAddTrack(const Track& track)
{
    ONLY_AUDIO_ENGINE_THREAD;
    const TrackId trackId = track.id;

    if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(track.chain->source())) {
        source->seek(m_player->currentPosition());
        source->inputParamsChanged().onReceive(this, [this, trackId](const AudioInputParams& params) {
            m_sourceParamsChanged.send(trackId, params);
        });
    }

    m_tracks.push_back(track);
    m_trackAdded.send(trackId);

    updateNonMutedTrackCount();
}

void AudioContext::onSourceParamsChanged(Track& track, const AudioSourceParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(track.chain->source())) {
        source->applyInputParams(params);
    }

    track.params.source = params;
}

void AudioContext::onControlParamsChanged(Track& track, const ControlParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (auto control = track.chain->control()) {
        const bool mutedChanged = control->muted() != params.muted;

        control->setVolume(params.volume);
        control->setPan(params.balance);
        control->setMuted(params.muted);

        //! NOTE For regular tracks
        if (mutedChanged && !params.muted && track.chain->source()) {
            if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(track.chain->source())) {
                source->seek(m_player->currentPosition());
            }
        }
    }

    track.params.control = params;

    updateNonMutedTrackCount();
}

void AudioContext::updateNonMutedTrackCount()
{
    ONLY_AUDIO_ENGINE_THREAD;
    size_t count = 0;
    for (const Track& t : m_tracks) {
        if (auto control = t.chain->control()) {
            if (!control->muted()) {
                count++;
            }
        }
    }
    m_mixer->setNonMutedTrackCount(count);
}

void AudioContext::onFxChainParamsChanged(Track& track, const AudioFxChain& params)
{
    ONLY_AUDIO_ENGINE_THREAD;

    const TrackId trackId = track.id;
    std::shared_ptr<IPlayheadPosition> playheadPosition = std::static_pointer_cast<IPlayheadPosition>(m_player);

    // Make fx chain
    FxChainPtr fxChain = audioFactory()->makeTrackFxChain(trackId, params);
    fxChain->setPlayheadPosition(playheadPosition);
    fxChain->fxChainSpecChanged().onReceive(this, [this, trackId](const AudioFxChain& fxChainSpec) {
        if (Track* t = this->track(trackId)) {
            t->params.fxChain = fxChainSpec;
            m_fxChainParamsChanged.send(trackId, t->params.fxChain);
        }
    });

    fxChain->shouldProcessDuringSilenceChanged().onReceive(this, [this, trackId](bool shouldProcess) {
        onShouldProcessDuringSilenceChanged(trackId, shouldProcess);
    });

    track.chain->setFxChain(fxChain);
    track.chain->rebuild();

    track.params.fxChain = fxChain->fxChainSpec();
}

void AudioContext::onAuxSendsParamsChanged(Track& track, const AuxSendsParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;

    m_mixer->setAuxSends(track.id, params);

    track.params.auxSends = params;
}

void AudioContext::removeTrack(const TrackId trackId)
{
    ONLY_AUDIO_ENGINE_THREAD;

    auto it = std::find_if(m_tracks.begin(), m_tracks.end(), [trackId](const Track& t) {
        return t.id == trackId;
    });

    if (it == m_tracks.end()) {
        return;
    }

    Track track = *it;
    if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(track.chain->source())) {
        source->inputParamsChanged().disconnect(this);
    }

    m_mixer->removeTrack(trackId);
    m_tracks.erase(it);
    muse::remove(m_tracksToProcessWhenIdle, trackId);

    if (m_prevActiveTrackId == trackId) {
        m_prevActiveTrackId = INVALID_TRACK_ID;
    }

    m_trackRemoved.send(trackId);
}

void AudioContext::removeAllTracks()
{
    ONLY_AUDIO_ENGINE_THREAD;

    for (const TrackId& id : trackIdList().val) {
        removeTrack(id);
    }
}

async::Channel<TrackId> AudioContext::trackAdded() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_trackAdded;
}

async::Channel<TrackId> AudioContext::trackRemoved() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_trackRemoved;
}

RetVal<TrackIdList> AudioContext::trackIdList() const
{
    ONLY_AUDIO_ENGINE_THREAD;

    TrackIdList result;
    result.reserve(m_tracks.size());

    for (const auto& t : m_tracks) {
        result.push_back(t.id);
    }

    return RetVal<TrackIdList>::make_ok(result);
}

const AudioContext::Track* AudioContext::track(const TrackId id) const
{
    ONLY_AUDIO_ENGINE_THREAD;

    if (id == MASTER_TRACK_ID) {
        return &m_masterTrack;
    }

    for (const Track& t : m_tracks) {
        if (t.id == id) {
            return &t;
        }
    }
    return nullptr;
}

AudioContext::Track* AudioContext::track(const TrackId id)
{
    ONLY_AUDIO_ENGINE_THREAD;

    if (id == MASTER_TRACK_ID) {
        return &m_masterTrack;
    }

    for (Track& t : m_tracks) {
        if (t.id == id) {
            return &t;
        }
    }
    return nullptr;
}

RetVal<TrackName> AudioContext::trackName(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;

    if (const Track* t = track(trackId)) {
        return RetVal<TrackName>::make_ok(t->name);
    }

    return RetVal<TrackName>::make_ret((int)Err::InvalidTrackId, "no track");
}

AudioSourceNodePtr AudioContext::trackSource(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        return std::dynamic_pointer_cast<AudioSourceNode>(t->chain->source());
    }
    return nullptr;
}

std::vector<AudioSourceNodePtr> AudioContext::allTracksSources() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    std::vector<AudioSourceNodePtr> result;
    result.reserve(m_tracks.size());
    for (const Track& t : m_tracks) {
        if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(t.chain->source())) {
            result.push_back(source);
        }
    }
    return result;
}

void AudioContext::onShouldProcessDuringSilenceChanged(const TrackId trackId, bool shouldProcess)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (shouldProcess) {
        m_tracksToProcessWhenIdle.insert(trackId);
    } else {
        muse::remove(m_tracksToProcessWhenIdle, trackId);
    }

    m_mixer->setTracksToProcessWhenIdle(m_tracksToProcessWhenIdle);
}

// Sources
AudioResourceMetaList AudioContext::availableInputResources() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return audioFactory()->availableInputResources();
}

SoundPresetList AudioContext::availableSoundPresets(const AudioResourceMeta& resourceMeta) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return audioFactory()->availableSoundPresets(resourceMeta);
}

RetVal<TrackParams> AudioContext::params(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        return RetVal<TrackParams>::make_ok(t->params);
    }

    return RetVal<TrackParams>::make_ret(Err::InvalidTrackId);
}

void AudioContext::setSourceParams(const TrackId trackId, const AudioSourceParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (Track* t = track(trackId)) {
        if (t->params.source != params) {
            onSourceParamsChanged(*t, params);
        }
    }
}

void AudioContext::setControlParams(const TrackId trackId, const ControlParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (Track* t = track(trackId)) {
        if (t->params.control != params) {
            onControlParamsChanged(*t, params);
        }
    }
}

void AudioContext::setFxChainParams(const TrackId trackId, const AudioFxChain& params)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (Track* t = track(trackId)) {
        if (t->params.fxChain != params) {
            onFxChainParamsChanged(*t, params);
        }
    }
}

void AudioContext::setAuxSendsParams(const TrackId trackId, const AuxSendsParams& params)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (Track* t = track(trackId)) {
        if (t->params.auxSends != params) {
            onAuxSendsParamsChanged(*t, params);
        }
    }
}

async::Channel<TrackId, AudioSourceParams> AudioContext::sourceParamsChanged() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_sourceParamsChanged;
}

async::Channel<TrackId, AudioFxChain> AudioContext::fxChainParamsChanged() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_fxChainParamsChanged;
}

void AudioContext::processInput(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(t->chain->source())) {
            source->processInput();
        }
    }
}

RetVal<InputProcessingProgress> AudioContext::inputProcessingProgress(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(t->chain->source())) {
            return RetVal<InputProcessingProgress>::make_ok(source->inputProcessingProgress());
        }
    }

    return make_ret(Err::InvalidTrackId);
}

void AudioContext::clearCache(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(t->chain->source())) {
            source->clearCache();
        }
    }
}

void AudioContext::clearSources()
{
    ONLY_AUDIO_ENGINE_THREAD;
    audioFactory()->clearSynthSources();
}

AudioResourceMetaList AudioContext::availableOutputResources() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return audioFactory()->availableOutputResources();
}

RetVal<AudioSignalChanges> AudioContext::signalChanges(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        if (auto signal = t->chain->signal()) {
            return RetVal<AudioSignalChanges>::make_ok(signal->audioSignalChanges());
        }
    }

    return RetVal<AudioSignalChanges>::make_ret(Err::InvalidTrackId);
}

RetVal<AutomatedControlParamsChanges> AudioContext::automatedControlParamsChanges(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (const Track* t = track(trackId)) {
        if (auto control = t->chain->control()) {
            return RetVal<AutomatedControlParamsChanges>::make_ok(control->automatedControlParamsChanges());
        }
    }

    return RetVal<AutomatedControlParamsChanges>::make_ret(Err::InvalidTrackId);
}

void AudioContext::clearMasterOutputParams()
{
    ONLY_AUDIO_ENGINE_THREAD;
    setControlParams(MASTER_TRACK_ID, ControlParams());
    setFxChainParams(MASTER_TRACK_ID, AudioFxChain());
}

void AudioContext::clearAllFx()
{
    ONLY_AUDIO_ENGINE_THREAD;
    audioFactory()->clearAllFx();
}

// Play
async::Promise<Ret> AudioContext::prepareToPlay()
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_player->prepareToPlay();
}

void AudioContext::play(const secs_t delay)
{
    LOGD() << "\nAudioContext chain: " << m_playheadNode->dump();

    ONLY_AUDIO_ENGINE_THREAD;
    m_player->play(delay);
}

void AudioContext::seek(const secs_t newPosition, const bool flushSound)
{
    ONLY_AUDIO_ENGINE_THREAD;
    m_player->seek(TimePosition::fromTime(newPosition, m_outputSpec.sampleRate), flushSound);
}

void AudioContext::stop()
{
    ONLY_AUDIO_ENGINE_THREAD;
    m_player->stop();
}

void AudioContext::pause()
{
    ONLY_AUDIO_ENGINE_THREAD;
    m_player->pause();
}

void AudioContext::resume(const secs_t delay)
{
    ONLY_AUDIO_ENGINE_THREAD;
    m_player->resume(delay);
}

void AudioContext::setDuration(const secs_t duration)
{
    ONLY_AUDIO_ENGINE_THREAD;
    m_player->setDuration(duration);
}

Ret AudioContext::setLoop(const secs_t from, const secs_t to)
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_player->setLoop(from, to);
}

void AudioContext::resetLoop()
{
    ONLY_AUDIO_ENGINE_THREAD;
    m_player->resetLoop();
}

PlaybackStatus AudioContext::playbackStatus() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_player->playbackStatus();
}

async::Channel<PlaybackStatus> AudioContext::playbackStatusChanged() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_player->playbackStatusChanged();
}

secs_t AudioContext::playbackPosition() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_player->playbackPosition();
}

async::Channel<secs_t> AudioContext::playbackPositionChanged() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_player->playbackPositionChanged();
}

// Export
async::Promise<Ret> AudioContext::saveSoundTrack(io::IODevice& dstDevice, const SoundTrackFormat& format)
{
    return async::make_promise<Ret>([this, &dstDevice, format](auto resolve, auto) {
        ONLY_AUDIO_ENGINE_THREAD;

#ifdef MUSE_MODULE_AUDIO_EXPORT
        //! NOTE These engine state changes must run inside execOperation so they are
        // synchronized with the audio driver process (see doSaveSoundTrack).
        Operation prepare = [this]() {
            m_player->stop();
            m_player->seek(TimePosition::zero(m_outputSpec.sampleRate));
        };
        if (m_execOperation) {
            m_execOperation->execOperation(OperationType::LongOperation, prepare);
        } else {
            prepare();
        }

        const bool lazyProcessingWasEnabled = configuration()->isLazyProcessingOfOnlineSoundsEnabled();
        configuration()->setIsLazyProcessingOfOnlineSoundsEnabled(false);

        listenInputProcessing([this, &dstDevice, format, lazyProcessingWasEnabled, resolve](Ret ret) {
            if (ret) {
                ret = doSaveSoundTrack(dstDevice, format);
            }

            configuration()->setIsLazyProcessingOfOnlineSoundsEnabled(lazyProcessingWasEnabled);
            (void)resolve(ret);
        });

        return async::Promise<Ret>::dummy_result();
#else
        return resolve(make_ret(Err::DisabledAudioExport, "audio export is disabled"));
#endif
    });
}

bool AudioContext::hasPendingChunks(const TrackId trackId) const
{
    ONLY_AUDIO_ENGINE_THREAD;

    if (const Track* t = track(trackId)) {
        if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(t->chain->source())) {
            return source->hasPendingChunks();
        }
    }

    return false;
}

size_t AudioContext::tracksBeingProcessedCount() const
{
    size_t count = 0;

    for (TrackId trackId : trackIdList().val) {
        if (inputProcessingProgress(trackId).val.isStarted || hasPendingChunks(trackId)) {
            count++;
        }
    }

    return count;
}

void AudioContext::listenInputProcessing(std::function<void(const Ret&)> completed)
{
#ifdef MUSE_MODULE_AUDIO_EXPORT

    auto soundsInProgress = std::make_shared<size_t>(0);

    for (TrackId trackId : trackIdList().val) {
        if (!isOnlineAudioResource(params(trackId).val.source.resourceMeta)) {
            continue;
        }

        InputProcessingProgress inputProgress = inputProcessingProgress(trackId).val;
        if (!inputProgress.isStarted && !hasPendingChunks(trackId)) {
            continue;
        }

        (*soundsInProgress)++;

        if (inputProgress.isStarted) {
            m_saveSoundTracksProgress.progress.send(0, 100, SaveSoundTrackStage::ProcessingOnlineSounds);
        }

        using StatusInfo = InputProcessingProgress::StatusInfo;
        using ChunkInfoList = InputProcessingProgress::ChunkInfoList;
        using ProgressInfo = InputProcessingProgress::ProgressInfo;

        auto ch = inputProgress.processedChannel;
        ch.onReceive(this, [this, trackId, soundsInProgress, completed](const StatusInfo& status,
                                                                        const ChunkInfoList&,
                                                                        const ProgressInfo& info) {
            const size_t tracksBeingProcessedCount = this->tracksBeingProcessedCount();

            if (status.status == InputProcessingProgress::Status::Finished) {
                inputProcessingProgress(trackId).val.processedChannel.disconnect(this);
            }

            if (tracksBeingProcessedCount == 0) {
                m_saveSoundTracksProgress.aborted.disconnect(this);
                completed(make_ok());
            } else if (tracksBeingProcessedCount == 1) {
                m_saveSoundTracksProgress.progress.send(info.current, info.total, SaveSoundTrackStage::ProcessingOnlineSounds);
            } else {
                const int64_t percentage = 100 - (100 / *soundsInProgress) * tracksBeingProcessedCount;
                m_saveSoundTracksProgress.progress.send(percentage, 100, SaveSoundTrackStage::ProcessingOnlineSounds);
            }
        });
    }

    if ((*soundsInProgress) == 0) {
        completed(make_ok());
        return;
    }

    m_saveSoundTracksProgress.aborted.onNotify(this, [this, completed]() {
        m_saveSoundTracksProgress.aborted.disconnect(this);

        for (TrackId trackId : trackIdList().val) {
            if (isOnlineAudioResource(params(trackId).val.source.resourceMeta)) {
                inputProcessingProgress(trackId).val.processedChannel.disconnect(this);
            }
        }

        completed(make_ret(Ret::Code::Cancel));
    });
#else
    UNUSED(completed);
#endif
}

Ret AudioContext::doSaveSoundTrack(io::IODevice& dstDevice, const SoundTrackFormat& format)
{
#ifdef MUSE_MODULE_AUDIO_EXPORT
    using namespace muse::audio::soundtrack;

    const secs_t totalDuration = m_player->duration();
    auto writer = std::make_shared<SoundTrackWriter>(dstDevice, format, totalDuration, m_mixer);

    writer->progress().progressChanged().onReceive(this, [this](int64_t current, int64_t total, std::string /*title*/) {
        m_saveSoundTracksProgress.progress.send(current, total, SaveSoundTrackStage::WritingSoundTrack);
    });

    std::weak_ptr<SoundTrackWriter> weakPtr = writer;
    m_saveSoundTracksProgress.aborted.onNotify(this, [weakPtr]() {
        if (auto writer = weakPtr.lock()) {
            writer->abort();
        }
    });

    //! NOTE The offline render and the source/engine state changes around it must run
    // inside execOperation so the audio driver process is muted for the whole export.
    // Otherwise the real time driver renders the shared graph (mixer, FX) concurrently
    // with the export, which is a data race on shared processors such as the reverb.
    Ret ret;
    Operation func = [this, writer, &ret]() {
        setMode(ProcessMode::PlayingOffline);
        ret = writer->write();
        m_mixer->setOutputSpec(outputSpec());
        setMode(ProcessMode::Idle);
        m_player->seek(TimePosition::zero(m_outputSpec.sampleRate));
    };

    if (m_execOperation) {
        m_execOperation->execOperation(OperationType::LongOperation, func);
    } else {
        func();
    }

    m_saveSoundTracksProgress.aborted.disconnect(this);

    return ret;
#else
    return make_ret(Err::DisabledAudioExport, "audio export is disabled");
#endif
}

void AudioContext::abortSavingAllSoundTracks()
{
#ifdef MUSE_MODULE_AUDIO_EXPORT
    m_saveSoundTracksProgress.aborted.notify();
#endif
}

SaveSoundTrackProgress AudioContext::saveSoundTrackProgressChanged() const
{
    ONLY_AUDIO_ENGINE_THREAD;
    return m_saveSoundTracksProgress.progress;
}

// Processing
void AudioContext::doSelfProcess(float* buffer, samples_t samplesPerChannel)
{
    ONLY_AUDIO_PROC_THREAD;
    m_playheadNode->process(buffer, samplesPerChannel);
}
