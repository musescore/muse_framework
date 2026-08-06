/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "eventaudionode.h"

#include "audio/common/audiosanitizer.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;
using namespace muse::audio::synth;
using namespace muse::mpe;

EventAudioNode::EventAudioNode(TrackId trackId, const mpe::PlaybackData& playbackData,
                               OnOffStreamEventsReceived onOffStreamReceived)
    : m_trackId(trackId), m_playbackData(playbackData)
{
    ONLY_AUDIO_ENGINE_THREAD;

    if (onOffStreamReceived) {
        m_playbackData.offStream.onReceive(this, [onOffStreamReceived](const PlaybackEventsMap&,
                                                                       bool) {
            onOffStreamReceived();
        });
    }
}

EventAudioNode::~EventAudioNode()
{
    m_playbackData.offStream.disconnect(this);
}

void EventAudioNode::onModeChanged(const ProcessMode mode)
{
    ONLY_AUDIO_ENGINE_THREAD;

    if (!m_synth) {
        return;
    }

    m_synth->setMode(mode);
    m_synth->flushSound();
}

void EventAudioNode::onOutputSpecChanged(const OutputSpec& spec)
{
    ONLY_AUDIO_ENGINE_THREAD;
    if (!m_synth) {
        return;
    }

    m_synth->setOutputSpec(spec);
}

void EventAudioNode::doSelfProcess(float* buffer, samples_t samplesPerChannel)
{
    ONLY_AUDIO_PROC_THREAD;
    if (!m_synth) {
        return;
    }

    m_synth->process(buffer, samplesPerChannel);
}

void EventAudioNode::seek(const TimePosition& position, const bool flushSound)
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return;
    }

    if (m_synth->playbackPosition() == position) {
        return;
    }

    m_synth->setPlaybackPosition(position);

    if (flushSound) {
        m_synth->flushSound();
    }
}

void EventAudioNode::flush()
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return;
    }

    m_synth->flushSound();
}

const AudioInputParams& EventAudioNode::inputParams() const
{
    return m_params;
}

void EventAudioNode::applyInputParams(const AudioInputParams& requiredParams)
{
    IF_ASSERT_FAILED(m_outputSpec.isValid()) {
        return;
    }

    if (m_params.isValid() && m_params == requiredParams) {
        return;
    }

    SynthCtx ctx = currentSynthCtx();

    if (m_synth) {
        m_playbackData = m_synth->playbackData();
    }

    RetVal<synth::ISynthesizerPtr> synth = audioFactory()->makeSynth(m_trackId, requiredParams, m_playbackData.setupData);

    if (!synth.ret) {
        synth = audioFactory()->makeDefaultSynth(m_trackId);
        IF_ASSERT_FAILED(synth.val) {
            LOGE() << "Default synth not found!";
            return;
        }
    }

    m_synth = synth.val;

    m_synth->paramsChanged().onReceive(this, [this](const AudioInputParams& params) {
        m_paramsChanges.send(params);
    });

    setupSource();

    if (ctx.isValid()) {
        restoreSynthCtx(ctx);
    } else {
        m_synth->setMode(ProcessMode::Idle);
    }

    setName(std::string("Source[") + m_synth->name() + "]");

    m_params = m_synth->params();
    m_paramsChanges.send(m_params);
}

async::Channel<AudioInputParams> EventAudioNode::inputParamsChanged() const
{
    return m_paramsChanges;
}

void EventAudioNode::prepareToPlay()
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return;
    }

    m_synth->prepareToPlay();
}

bool EventAudioNode::readyToPlay() const
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return false;
    }

    return m_synth->readyToPlay();
}

async::Notification EventAudioNode::readyToPlayChanged() const
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return {};
    }

    return m_synth->readyToPlayChanged();
}

bool EventAudioNode::hasPendingChunks() const
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return false;
    }

    return m_synth->hasPendingChunks();
}

void EventAudioNode::processInput()
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return;
    }

    m_synth->processInput();
}

InputProcessingProgress EventAudioNode::inputProcessingProgress() const
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return {};
    }

    return m_synth->inputProcessingProgress();
}

void EventAudioNode::clearCache()
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return;
    }

    m_synth->clearCache();
}

EventAudioNode::SynthCtx EventAudioNode::currentSynthCtx() const
{
    if (!m_synth) {
        return SynthCtx();
    }

    return { m_synth->mode(), m_synth->playbackPosition() };
}

void EventAudioNode::restoreSynthCtx(const SynthCtx& ctx)
{
    m_synth->setPlaybackPosition(ctx.playbackPosition);
    m_synth->setMode(ctx.mode);
}

void EventAudioNode::setupSource()
{
    ONLY_AUDIO_ENGINE_THREAD;

    IF_ASSERT_FAILED(m_synth) {
        return;
    }

    m_synth->setOutputSpec(m_outputSpec);
    m_synth->setup(m_playbackData);
}
