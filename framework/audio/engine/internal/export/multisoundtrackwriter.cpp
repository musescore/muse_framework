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

#include "multisoundtrackwriter.h"

#include <algorithm>
#include <cstdint>

#include "global/defer.h"

#include "audio/common/audioerrors.h"

#include "encoderfactory.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::soundtrack;

MultiSoundTrackWriter::MultiSoundTrackWriter(const SoundTrackTargetList& targets, const SoundTrackFormat& format,
                                             const secs_t totalDuration, engine::MixerPtr mixer)
    : m_mixer(std::move(mixer))
{
    if (!m_mixer) {
        return;
    }
    IF_ASSERT_FAILED(format.isValid()) {
        return;
    }

    const OutputSpec& outputSpec = format.outputSpec;

    auto durationToSamples = [&](const secs_t& duration) {
        const double sec = std::max(0.0, duration.raw());
        return static_cast<samples_t>(std::llround(sec * static_cast<double>(outputSpec.sampleRate)));
    };

    m_dataSamples = durationToSamples(totalDuration);
    m_leadingSilenceSamples = durationToSamples(format.leadingSilenceDuration);
    const samples_t trailingSilenceSamples = durationToSamples(format.trailingSilenceDuration);
    m_totalSamples = m_leadingSilenceSamples + m_dataSamples + trailingSilenceSamples;

    const samples_t intermediateSamplesNumber = outputSpec.samplesPerChannel * outputSpec.audioChannelCount;
    m_silenceBuffer.assign(intermediateSamplesNumber, 0.f);
    m_scratchOutBuffer.resize(intermediateSamplesNumber);
    m_renderStep = outputSpec.samplesPerChannel;

    m_trackEncoders.reserve(targets.size());
    for (const SoundTrackTarget& target : targets) {
        IF_ASSERT_FAILED(target.dstDevice) {
            continue;
        }

        encode::AbstractAudioEncoderPtr encoder = createEncoder(format, *target.dstDevice);
        if (!encoder || !encoder->begin(m_totalSamples)) {
            LOGE() << "Failed to start encoder for track " << target.trackId;
            m_hasEncodeError = true;
            continue;
        }

        m_trackEncoderIndexByTrackId[target.trackId] = m_trackEncoders.size();
        m_trackEncoders.push_back(TrackEncoder { target.trackId, std::move(encoder), false });
    }
}

MultiSoundTrackWriter::~MultiSoundTrackWriter()
{
    if (m_mixer) {
        m_mixer->setTrackStemCallback(nullptr);
    }
}

Ret MultiSoundTrackWriter::write()
{
    TRACEFUNC;

    if (!m_mixer || m_trackEncoders.empty() || m_hasEncodeError) {
        return make_ret(Err::NoAudioToExport);
    }

    m_mixer->setOutputSpec(m_trackEncoders.front().encoder->format().outputSpec);
    m_mixer->setMode(ProcessMode::PlayingOffline);

    m_mixer->setTrackStemCallback([this](TrackId trackId, const float* buffer, samples_t samplesPerChannel) {
        onTrackStem(trackId, buffer, samplesPerChannel);
    });

    DEFER {
        m_mixer->setTrackStemCallback(nullptr);

        if (!m_isAborted) {
            for (auto& te : m_trackEncoders) {
                te.encoder->end();
            }
        }

        m_isAborted = false;
    };

    Ret ret = writeStreaming();
    if (!ret) {
        return ret;
    }

    return muse::make_ok();
}

void MultiSoundTrackWriter::abort()
{
    m_isAborted = true;
}

Progress MultiSoundTrackWriter::progress()
{
    return m_progress;
}

bool MultiSoundTrackWriter::encodeSilenceToAllTargets(samples_t chunk)
{
    for (TrackEncoder& te : m_trackEncoders) {
        if (te.encoder->encode(chunk, m_silenceBuffer.data()) == 0) {
            return false;
        }
    }

    return true;
}

void MultiSoundTrackWriter::onTrackStem(TrackId trackId, const float* buffer, samples_t samplesPerChannel)
{
    auto it = m_trackEncoderIndexByTrackId.find(trackId);
    if (it == m_trackEncoderIndexByTrackId.end()) {
        return;
    }

    TrackEncoder& te = m_trackEncoders[it->second];
    te.gotChunkThisBlock = true;

    if (te.encoder->encode(samplesPerChannel, buffer) == 0) {
        LOGE() << "Failed to encode stem for track " << trackId;
        m_hasEncodeError = true;
    }
}

Ret MultiSoundTrackWriter::writeStreaming()
{
    TRACEFUNC;
    if (m_totalSamples == 0) {
        LOGI() << "No audio to export";
        return make_ret(Err::NoAudioToExport);
    }

    samples_t framesWritten = 0;

    sendProgress(0, m_totalSamples);

    // Phase 1: leading silence
    const samples_t leadingEnd = m_leadingSilenceSamples;
    while (framesWritten < leadingEnd && !m_isAborted) {
        const samples_t chunk = static_cast<samples_t>(
            std::min<uint64_t>(m_renderStep, leadingEnd - framesWritten));

        if (!encodeSilenceToAllTargets(chunk)) {
            return make_ret(Err::ErrorEncode);
        }

        framesWritten += chunk;
        sendProgress(framesWritten, m_totalSamples);
        rpcChannel()->process();
    }

    // Phase 2: actual audio data. A single mixer render drives every stem at once:
    // Mixer::process() computes every track's own buffer before summing them, and our
    // stem callback (set in write()) intercepts and encodes each one as it's produced.
    const samples_t audioEnd = m_leadingSilenceSamples + m_dataSamples;
    while (framesWritten < audioEnd && !m_isAborted) {
        const samples_t chunk = static_cast<samples_t>(
            std::min<uint64_t>(m_renderStep, audioEnd - framesWritten));

        for (TrackEncoder& te : m_trackEncoders) {
            te.gotChunkThisBlock = false;
        }

        //! NOTE The mixer mixes additively and relies on the caller to zero the output buffer
        //! (real time does this via AudioEngine::fillSilent), so clear it before each block.
        //! The summed result itself is discarded here; only the per-track stems are used.
        std::fill(m_scratchOutBuffer.begin(), m_scratchOutBuffer.end(), 0.f);
        m_mixer->process(m_scratchOutBuffer.data(), chunk);

        for (TrackEncoder& te : m_trackEncoders) {
            if (te.gotChunkThisBlock) {
                continue;
            }

            //! NOTE A requested track wasn't processed this block (e.g. removed mid-export);
            //! keep its stem in sync with the others by writing silence for this chunk.
            if (te.encoder->encode(chunk, m_silenceBuffer.data()) == 0) {
                return make_ret(Err::ErrorEncode);
            }
        }

        if (m_hasEncodeError) {
            return make_ret(Err::ErrorEncode);
        }

        framesWritten += chunk;
        sendProgress(framesWritten, m_totalSamples);
        rpcChannel()->process();
    }

    // Phase 3: trailing silence
    while (framesWritten < m_totalSamples && !m_isAborted) {
        const samples_t chunk = static_cast<samples_t>(
            std::min<uint64_t>(m_renderStep, m_totalSamples - framesWritten));

        if (!encodeSilenceToAllTargets(chunk)) {
            return make_ret(Err::ErrorEncode);
        }

        framesWritten += chunk;
        sendProgress(framesWritten, m_totalSamples);
        rpcChannel()->process();
    }

    if (m_isAborted) {
        return make_ret(Ret::Code::Cancel);
    }

    return muse::make_ok();
}

void MultiSoundTrackWriter::sendProgress(uint64_t framesWritten, uint64_t totalFrames)
{
    const int current = totalFrames > 0 ? static_cast<int>((framesWritten * 100) / totalFrames) : 0;
    m_progress.progress(current, 100);
}
