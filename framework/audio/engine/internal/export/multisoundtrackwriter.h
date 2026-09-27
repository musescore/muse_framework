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

#include <unordered_map>
#include <vector>

#include "global/async/asyncable.h"

#include "global/modularity/ioc.h"
#include "audio/common/rpc/irpcchannel.h"

#include "audio/common/audiotypes.h"
#include "../mixer.h"

#include "abstractaudioencoder.h"

namespace muse::io {
class IODevice;
}

namespace muse::audio::soundtrack {
//! NOTE Renders every requested track of a Mixer in a single offline pass and encodes
//! each one to its own destination file (one "stem" per track), instead of rendering
//! the whole graph once per track as repeated calls to SoundTrackWriter would. This is
//! what makes exporting audio for e.g. every instrument part of a score fast: the mixer
//! already computes a per-track buffer for every processed block before summing it into
//! the master output (see Mixer::TrackStemCallback) — this class just also encodes it.
class MultiSoundTrackWriter : public async::Asyncable
{
    muse::GlobalInject<rpc::IRpcChannel> rpcChannel;

public:
    MultiSoundTrackWriter(const SoundTrackTargetList& targets, const SoundTrackFormat& format, const secs_t totalDuration,
                          engine::MixerPtr mixer);
    ~MultiSoundTrackWriter();

    Ret write();
    void abort();

    Progress progress();

private:
    Ret writeStreaming();

    bool encodeSilenceToAllTargets(samples_t chunk);
    void onTrackStem(TrackId trackId, const float* buffer, samples_t samplesPerChannel);

    void sendProgress(uint64_t framesWritten, uint64_t totalFrames);

    struct TrackEncoder {
        TrackId trackId = 0;
        encode::AbstractAudioEncoderPtr encoder;
        bool gotChunkThisBlock = false;
    };

    engine::MixerPtr m_mixer;
    std::vector<TrackEncoder> m_trackEncoders;
    std::unordered_map<TrackId, size_t> m_trackEncoderIndexByTrackId;

    std::vector<float> m_silenceBuffer;
    std::vector<float> m_scratchOutBuffer;

    samples_t m_renderStep = 0;
    samples_t m_leadingSilenceSamples = 0;
    samples_t m_dataSamples = 0;
    samples_t m_totalSamples = 0;

    Progress m_progress;
    std::atomic<bool> m_isAborted = false;
    std::atomic<bool> m_hasEncodeError = false;
};
}
