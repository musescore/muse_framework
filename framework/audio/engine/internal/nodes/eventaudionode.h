/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#pragma once

#include "audiosourcenode.h"

#include "global/async/asyncable.h"
#include "global/modularity/ioc.h"
#include "mpe/events.h"

#include "audio/common/audiotypes.h"
#include "../iaudiofactory.h"

namespace muse::audio::engine {
class EventAudioNode : public AudioSourceNode, public async::Asyncable
{
    GlobalInject<IAudioFactory> audioFactory;

public:
    using OnOffStreamEventsReceived = std::function<void ()>;

    explicit EventAudioNode(TrackId trackId, const mpe::PlaybackData& playbackData,
                            OnOffStreamEventsReceived onOffStreamReceived);

    ~EventAudioNode() override;

    void seek(const TimePosition& position, const bool flushSound = true) override;
    void flush() override;

    const AudioInputParams& inputParams() const override;
    void applyInputParams(const AudioInputParams& requiredParams) override;
    async::Channel<AudioInputParams> inputParamsChanged() const override;

    void prepareToPlay() override;
    bool readyToPlay() const override;
    async::Notification readyToPlayChanged() const override;

    bool hasPendingChunks() const override;
    void processInput() override;
    InputProcessingProgress inputProcessingProgress() const override;

    void clearCache() override;

private:
    struct SynthCtx
    {
        ProcessMode mode = ProcessMode::Undefined;
        TimePosition playbackPosition;
        bool isValid() const { return mode != ProcessMode::Undefined; }
    };

    void onModeChanged(const ProcessMode mode) override;
    void onOutputSpecChanged(const OutputSpec& spec) override;

    void doSelfProcess(float* buffer, samples_t samplesPerChannel) override;

    void setupSource();
    SynthCtx currentSynthCtx() const;
    void restoreSynthCtx(const SynthCtx& ctx);

    TrackId m_trackId = -1;
    mpe::PlaybackData m_playbackData;
    synth::ISynthesizerPtr m_synth = nullptr;
    AudioInputParams m_params;
    async::Channel<AudioInputParams> m_paramsChanges;
};

using EventAudioNodePtr = std::shared_ptr<EventAudioNode>;
}
