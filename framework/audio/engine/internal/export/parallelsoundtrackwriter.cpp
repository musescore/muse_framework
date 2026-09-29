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

#include "parallelsoundtrackwriter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <new>
#include <numeric>
#include <thread>

#ifdef __APPLE__
#include <pthread.h>
#include <sys/qos.h>
#endif

#include "global/types/number.h"
#include "audio/common/audioerrors.h"

#include "../nodes/audiosourcenode.h"

#include "encoderfactory.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;
using namespace muse::audio::soundtrack;

static bool isChainSilent(const TrackChainPtr& chain)
{
    if (auto signal = chain->signal()) {
        return signal->isSilent();
    }
    return false;
}

static void mixInto(float* dst, const float* src, size_t size, float gain = 1.f)
{
    for (size_t i = 0; i < size; ++i) {
        dst[i] += src[i] * gain;
    }
}

static bool isSendActive(const AuxSendsParams& sends, size_t auxIdx)
{
    return auxIdx < sends.size() && sends.at(auxIdx).active && !muse::is_zero(sends.at(auxIdx).signalAmount);
}

ParallelSoundTrackWriter::ParallelSoundTrackWriter(std::vector<Track> tracks, std::vector<File> files,
                                                   std::vector<AuxChannels> auxChannelsPerWorker, const SoundTrackFormat& format,
                                                   const secs_t totalDuration, const Options& options)
    : m_tracks(std::move(tracks)), m_auxChannelsPerWorker(std::move(auxChannelsPerWorker)), m_options(options)
{
    IF_ASSERT_FAILED(format.isValid()) {
        m_hasEncodeError = true;
        return;
    }

    m_outputSpec = format.outputSpec;

    auto durationToSamples = [this](const secs_t& duration) {
        const double sec = std::max(0.0, duration.raw());
        return static_cast<samples_t>(std::llround(sec * static_cast<double>(m_outputSpec.sampleRate)));
    };

    m_dataSamples = durationToSamples(totalDuration);
    m_leadingSilenceSamples = durationToSamples(format.leadingSilenceDuration);
    const samples_t trailingSilenceSamples = durationToSamples(format.trailingSilenceDuration);
    m_totalSamples = m_leadingSilenceSamples + m_dataSamples + trailingSilenceSamples;
    m_idlePreRollSamples = durationToSamples(options.idlePreRoll);

    const AuxChannels* firstAuxChannels = m_auxChannelsPerWorker.empty() ? nullptr : &m_auxChannelsPerWorker.front();
    const size_t auxCount = firstAuxChannels ? firstAuxChannels->size() : 0;

    for (File& file : files) {
        const bool validTracks = std::all_of(file.tracks.cbegin(), file.tracks.cend(), [this](size_t t) { return t < m_tracks.size(); });
        IF_ASSERT_FAILED(file.dstDevice && !file.tracks.empty() && validTracks) {
            m_hasEncodeError = true;
            return;
        }

        encode::AbstractAudioEncoderPtr encoder = createEncoder(format, *file.dstDevice);
        if (!encoder || !encoder->begin(m_totalSamples)) {
            LOGE() << "Failed to start encoder";
            m_hasEncodeError = true;
            return;
        }

        auto state = std::make_unique<FileState>();
        state->encoder = std::move(encoder);
        state->auxUsed.assign(auxCount, false);

        //! NOTE Only the aux channels this file actually sends to (active, above 0%) are processed for it.
        //! The others would get no input and stay silent, so Mixer::process() would skip them as well
        for (const size_t t : file.tracks) {
            state->weight += m_tracks.at(t).weight;

            for (size_t auxIdx = 0; auxIdx < auxCount; ++auxIdx) {
                if (firstAuxChannels->at(auxIdx) && isSendActive(m_tracks.at(t).auxSends, auxIdx)) {
                    state->auxUsed[auxIdx] = true;
                }
            }
        }

        state->file = std::move(file);
        m_files.push_back(std::move(state));
    }

    // Decide which files are rendered directly: smallest first (parts before the full score), and a file
    // qualifies when none of its tracks is already rendered by another directly rendered file
    std::vector<size_t> fileOrder(m_files.size());
    std::iota(fileOrder.begin(), fileOrder.end(), 0);
    std::stable_sort(fileOrder.begin(), fileOrder.end(), [this](size_t a, size_t b) {
        return m_files.at(a)->file.tracks.size() < m_files.at(b)->file.tracks.size();
    });

    std::vector<bool> trackOwned(m_tracks.size(), false);
    m_trackCombinedFiles.assign(m_tracks.size(), {});

    for (const size_t fileIdx : fileOrder) {
        FileState& state = *m_files.at(fileIdx);

        const bool isFree = std::none_of(state.file.tracks.cbegin(), state.file.tracks.cend(), [&](size_t t) { return trackOwned.at(t); });
        if (isFree) {
            for (const size_t t : state.file.tracks) {
                trackOwned[t] = true;
            }

            m_renderJobs.push_back({ fileIdx, state.file.tracks, state.weight });
            continue;
        }

        //! NOTE A combined file keeps its tracks' whole output in memory until all of them are rendered
        try {
            state.accumulator = std::make_unique<Accumulator>();
            state.accumulator->dry.assign(m_dataSamples * m_outputSpec.audioChannelCount, 0.f);
            state.accumulator->auxSends.resize(auxCount);
            for (size_t auxIdx = 0; auxIdx < auxCount; ++auxIdx) {
                if (state.auxUsed.at(auxIdx)) {
                    state.accumulator->auxSends[auxIdx].assign(m_dataSamples * m_outputSpec.audioChannelCount, 0.f);
                }
            }
        } catch (const std::bad_alloc&) {
            LOGE() << "Not enough memory for the combined files of the export";
            m_hasEncodeError = true;
            return;
        }

        for (const size_t t : state.file.tracks) {
            m_trackCombinedFiles[t].push_back(fileIdx);
        }

        m_combinedFiles.push_back(fileIdx);
    }

    //! NOTE Tracks that only belong to combined files still need to be rendered once
    for (size_t t = 0; t < m_tracks.size(); ++t) {
        if (!trackOwned.at(t) && !m_trackCombinedFiles.at(t).empty()) {
            m_renderJobs.push_back({ std::nullopt, { t }, m_tracks.at(t).weight });
        }
    }

    //! NOTE Heaviest first, so a slow job doesn't start last and hold up the whole export
    std::stable_sort(m_renderJobs.begin(), m_renderJobs.end(), [](const RenderJob& a, const RenderJob& b) {
        return a.weight > b.weight;
    });

    std::stable_sort(m_combinedFiles.begin(), m_combinedFiles.end(), [this](size_t a, size_t b) {
        auto usedAuxCount = [this](size_t f) { return std::count(m_files.at(f)->auxUsed.cbegin(), m_files.at(f)->auxUsed.cend(), true); };
        return usedAuxCount(a) > usedAuxCount(b);
    });

    if (!m_combinedFiles.empty()) {
        size_t bufferCount = 0;
        for (const size_t f : m_combinedFiles) {
            bufferCount += 1 + std::count(m_files.at(f)->auxUsed.cbegin(), m_files.at(f)->auxUsed.cend(), true);
        }

        const size_t bytes = bufferCount * m_dataSamples * m_outputSpec.audioChannelCount * sizeof(float);
        LOGI() << m_combinedFiles.size() << " combined files, accumulated in " << (bytes / (1024 * 1024)) << " MB";
    }
}

Ret ParallelSoundTrackWriter::write()
{
    TRACEFUNC;

    if (m_files.empty() || m_hasEncodeError || m_totalSamples == 0) {
        return make_ret(Err::NoAudioToExport);
    }

    IF_ASSERT_FAILED(!m_auxChannelsPerWorker.empty()) {
        return make_ret(Err::NoAudioToExport);
    }

    //! NOTE The borrowed tracks get their mode/spec from the mixer (see AudioContext),
    //! the aux copies belong to this export only
    for (AuxChannels& auxChannels : m_auxChannelsPerWorker) {
        for (TrackChainPtr& aux : auxChannels) {
            if (aux) {
                aux->setOutputSpec(m_outputSpec);
                aux->setMode(ProcessMode::PlayingOffline);
            }
        }
    }

    //! NOTE Tracks that idle until their first note start at the block containing their wake-up point.
    //! Their sources are moved there now, on this (engine) thread, rather than by the workers
    const samples_t renderStep = m_outputSpec.samplesPerChannel;
    m_trackStartFrames.assign(m_tracks.size(), 0);
    for (size_t t = 0; t < m_tracks.size(); ++t) {
        const Track& track = m_tracks.at(t);
        if (!m_options.idleUntilFirstNote || renderStep == 0) {
            continue;
        }

        if (!track.firstNoteTime.has_value()) {
            m_trackStartFrames[t] = m_dataSamples; // no notes: never processed
            continue;
        }

        const samples_t firstNote
            = static_cast<samples_t>(std::llround(std::max(0.0, track.firstNoteTime->raw()) * m_outputSpec.sampleRate));
        const samples_t wakeUp = firstNote > m_idlePreRollSamples ? firstNote - m_idlePreRollSamples : 0;
        const samples_t startFrame = (wakeUp / renderStep) * renderStep;
        m_trackStartFrames[t] = startFrame;

        if (startFrame > 0 && startFrame < m_dataSamples) {
            if (auto source = std::dynamic_pointer_cast<AudioSourceNode>(track.chain->source())) {
                source->seek(TimePosition::fromSamples(startFrame, m_outputSpec.sampleRate), true);
            }
        }
    }

    const size_t workerCount = std::max<size_t>(1, std::min(m_auxChannelsPerWorker.size(),
                                                            std::max(m_renderJobs.size(), m_combinedFiles.size())));
    LOGI() << "Exporting " << m_files.size() << " files (" << m_combinedFiles.size() << " combined, " << m_renderJobs.size()
           << " render jobs) on " << workerCount << " threads";

    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (size_t i = 0; i < workerCount; ++i) {
        workers.emplace_back(&ParallelSoundTrackWriter::workerLoop, this, i);
    }

    //! NOTE This (engine) thread only reports progress and handles incoming messages (e.g. abort).
    //! The workers never send messages themselves, see SignalNode::notifyAboutChanges()
    const size_t renderOnlyJobCount = std::count_if(m_renderJobs.cbegin(), m_renderJobs.cend(), [](const RenderJob& job) {
        return !job.fileIdx.has_value();
    });
    const uint64_t totalFrames = static_cast<uint64_t>(m_totalSamples) * m_files.size()
                                 + static_cast<uint64_t>(m_dataSamples) * renderOnlyJobCount;
    int lastProgress = -1;
    std::vector<int> lastFilesProgress(m_files.size(), -1);

    auto allDone = [this]() {
        if (m_renderJobsDone.load() < m_renderJobs.size()) {
            return false;
        }

        for (const auto& file : m_files) {
            if (file->framesWritten.load() < m_totalSamples) {
                return false;
            }
        }

        return true;
    };

    while (true) {
        uint64_t framesDone = m_renderOnlyFramesDone.load();
        for (const auto& file : m_files) {
            framesDone += file->framesWritten.load();
        }

        const int current = totalFrames > 0 ? static_cast<int>((framesDone * 100) / totalFrames) : 0;
        if (current != lastProgress) {
            lastProgress = current;
            m_progress.progress(current, 100);
        }

        const std::vector<int> filesProgress = this->filesProgress();
        for (size_t i = 0; i < filesProgress.size(); ++i) {
            if (filesProgress.at(i) != lastFilesProgress.at(i)) {
                lastFilesProgress[i] = filesProgress.at(i);
                m_fileProgressChanged.send(i, filesProgress.at(i));
            }
        }

        rpcChannel()->process();

        if (m_isAborted || m_hasEncodeError || allDone()) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    if (m_isAborted) {
        m_isAborted = false;
        return make_ret(Ret::Code::Cancel);
    }

    for (auto& file : m_files) {
        file->encoder->end();
    }

    if (m_hasEncodeError) {
        return make_ret(Err::ErrorEncode);
    }

    return muse::make_ok();
}

void ParallelSoundTrackWriter::abort()
{
    m_isAborted = true;
}

Progress ParallelSoundTrackWriter::progress()
{
    return m_progress;
}

async::Channel<size_t, int> ParallelSoundTrackWriter::fileProgressChanged() const
{
    return m_fileProgressChanged;
}

std::vector<int> ParallelSoundTrackWriter::filesProgress() const
{
    std::vector<int> result;
    result.reserve(m_files.size());

    for (const auto& file : m_files) {
        const uint64_t written = file->framesWritten.load();
        result.push_back(m_totalSamples > 0 ? static_cast<int>((written * 100) / m_totalSamples) : 0);
    }

    return result;
}

void ParallelSoundTrackWriter::workerLoop(size_t workerIdx)
{
#ifdef __APPLE__
    //! NOTE Work the user is waiting for: lets macOS prefer the performance cores for it
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif

    const AuxChannels& auxChannels = m_auxChannelsPerWorker.at(workerIdx);

    // Phase 1: render jobs
    while (!m_isAborted && !m_hasEncodeError) {
        const size_t jobIdx = m_nextRenderJobIdx.fetch_add(1);
        if (jobIdx >= m_renderJobs.size()) {
            break;
        }

        const RenderJob& job = m_renderJobs.at(jobIdx);

        if (!runRenderJob(job, auxChannels)) {
            m_hasEncodeError = true;
        }

        ++m_renderJobsDone;
    }

    // Phase 2: combined files, once every track has been rendered
    while (!m_isAborted && !m_hasEncodeError && m_renderJobsDone.load() < m_renderJobs.size()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    while (!m_isAborted && !m_hasEncodeError) {
        const size_t idx = m_nextCombinedFileIdx.fetch_add(1);
        if (idx >= m_combinedFiles.size()) {
            break;
        }

        FileState& file = *m_files.at(m_combinedFiles.at(idx));

        if (!runCombinedFile(file, auxChannels)) {
            m_hasEncodeError = true;
        }
    }
}

bool ParallelSoundTrackWriter::renderTrack(size_t trackIdx, samples_t dataFrame, samples_t chunk, std::vector<float>& trackBuffer)
{
    const Track& track = m_tracks.at(trackIdx);

    std::fill(trackBuffer.begin(), trackBuffer.end(), 0.f);

    //! NOTE A track that idles until its first note isn't processed before its start frame (see write())
    if (dataFrame < m_trackStartFrames.at(trackIdx)) {
        return false;
    }

    track.chain->process(trackBuffer.data(), chunk);

    return !isChainSilent(track.chain);
}

void ParallelSoundTrackWriter::contributeToCombinedFiles(size_t trackIdx, samples_t dataFrame, samples_t chunk, const float* trackBuffer)
{
    const size_t channelCount = m_outputSpec.audioChannelCount;
    const size_t offset = dataFrame * channelCount;
    const size_t size = chunk * channelCount;
    const AuxSendsParams& sends = m_tracks.at(trackIdx).auxSends;

    for (const size_t fileIdx : m_trackCombinedFiles.at(trackIdx)) {
        FileState& file = *m_files.at(fileIdx);
        Accumulator& acc = *file.accumulator;

        std::lock_guard lock(acc.mutex);

        mixInto(acc.dry.data() + offset, trackBuffer, size);

        for (size_t auxIdx = 0; auxIdx < file.auxUsed.size(); ++auxIdx) {
            if (file.auxUsed.at(auxIdx) && isSendActive(sends, auxIdx)) {
                mixInto(acc.auxSends[auxIdx].data() + offset, trackBuffer, size, sends.at(auxIdx).signalAmount);
            }
        }
    }
}

void ParallelSoundTrackWriter::processAuxChannels(const std::vector<bool>& auxUsed, const AuxChannels& auxChannels,
                                                  std::vector<std::vector<float> >& auxBuffers, const std::vector<bool>& auxReceived,
                                                  samples_t chunk, float* mixBuffer)
{
    const size_t chunkSize = chunk * m_outputSpec.audioChannelCount;

    for (size_t auxIdx = 0; auxIdx < auxUsed.size() && auxIdx < auxChannels.size(); ++auxIdx) {
        const TrackChainPtr& aux = auxChannels.at(auxIdx);
        if (!auxUsed.at(auxIdx) || !aux) {
            continue;
        }

        //! NOTE Process when the aux received a signal this block and/or if it's not yet silent
        //! (e.g. reverb is still ringing out), same as Mixer::processAuxChannels()
        if (!auxReceived.at(auxIdx) && isChainSilent(aux)) {
            continue;
        }

        aux->process(auxBuffers[auxIdx].data(), chunk);

        if (!isChainSilent(aux)) {
            mixInto(mixBuffer, auxBuffers[auxIdx].data(), chunkSize);
        }
    }
}

bool ParallelSoundTrackWriter::encodeSilence(FileState& file, samples_t frames, std::vector<float>& silenceBuffer)
{
    const samples_t renderStep = m_outputSpec.samplesPerChannel;
    samples_t done = 0;

    while (done < frames && !m_isAborted) {
        const samples_t chunk = std::min<samples_t>(renderStep, frames - done);
        if (file.encoder->encode(chunk, silenceBuffer.data()) == 0) {
            LOGE() << "Failed to encode";
            return false;
        }

        file.framesWritten += chunk;
        done += chunk;
    }

    return true;
}

bool ParallelSoundTrackWriter::runRenderJob(const RenderJob& job, const AuxChannels& auxChannels)
{
    const size_t channelCount = m_outputSpec.audioChannelCount;
    const samples_t renderStep = m_outputSpec.samplesPerChannel;
    const size_t bufferSize = renderStep * channelCount;

    FileState* file = job.fileIdx ? m_files.at(*job.fileIdx).get() : nullptr;
    const std::vector<bool> auxUsed = file ? file->auxUsed : std::vector<bool>(auxChannels.size(), false);

    //! NOTE This worker's aux copies may still hold the tail of its previous file
    for (size_t auxIdx = 0; auxIdx < auxUsed.size() && auxIdx < auxChannels.size(); ++auxIdx) {
        if (auxUsed.at(auxIdx) && auxChannels.at(auxIdx)) {
            if (FxChainPtr fxChain = auxChannels.at(auxIdx)->fxChain()) {
                fxChain->resetState();
            }
        }
    }

    std::vector<float> trackBuffer(bufferSize, 0.f);
    std::vector<float> mixBuffer(bufferSize, 0.f);
    std::vector<std::vector<float> > auxBuffers(auxUsed.size());
    std::vector<bool> auxReceived(auxUsed.size(), false);

    for (size_t auxIdx = 0; auxIdx < auxUsed.size(); ++auxIdx) {
        if (auxUsed.at(auxIdx)) {
            auxBuffers[auxIdx].assign(bufferSize, 0.f);
        }
    }

    // Phase 1: leading silence
    if (file && !encodeSilence(*file, m_leadingSilenceSamples, mixBuffer)) {
        return false;
    }

    // Phase 2: actual audio data, mixed like Mixer::process()
    for (samples_t dataFrame = 0; dataFrame < m_dataSamples; dataFrame += renderStep) {
        if (m_isAborted) {
            return true;
        }

        const samples_t chunk = std::min<samples_t>(renderStep, m_dataSamples - dataFrame);
        const size_t chunkSize = chunk * channelCount;

        if (file) {
            std::fill(mixBuffer.begin(), mixBuffer.end(), 0.f);
            for (size_t auxIdx = 0; auxIdx < auxUsed.size(); ++auxIdx) {
                if (auxUsed.at(auxIdx)) {
                    std::fill(auxBuffers[auxIdx].begin(), auxBuffers[auxIdx].end(), 0.f);
                    auxReceived[auxIdx] = false;
                }
            }
        }

        for (const size_t trackIdx : job.tracks) {
            if (!renderTrack(trackIdx, dataFrame, chunk, trackBuffer)) {
                continue;
            }

            contributeToCombinedFiles(trackIdx, dataFrame, chunk, trackBuffer.data());

            if (!file) {
                continue;
            }

            mixInto(mixBuffer.data(), trackBuffer.data(), chunkSize);

            const AuxSendsParams& sends = m_tracks.at(trackIdx).auxSends;
            for (size_t auxIdx = 0; auxIdx < auxUsed.size(); ++auxIdx) {
                if (auxUsed.at(auxIdx) && isSendActive(sends, auxIdx)) {
                    mixInto(auxBuffers[auxIdx].data(), trackBuffer.data(), chunkSize, sends.at(auxIdx).signalAmount);
                    auxReceived[auxIdx] = true;
                }
            }
        }

        if (!file) {
            m_renderOnlyFramesDone += chunk;
            continue;
        }

        processAuxChannels(auxUsed, auxChannels, auxBuffers, auxReceived, chunk, mixBuffer.data());

        if (file->encoder->encode(chunk, mixBuffer.data()) == 0) {
            LOGE() << "Failed to encode";
            return false;
        }

        file->framesWritten += chunk;
    }

    // Phase 3: trailing silence
    if (file) {
        std::fill(mixBuffer.begin(), mixBuffer.end(), 0.f);
        if (!encodeSilence(*file, m_totalSamples - m_leadingSilenceSamples - m_dataSamples, mixBuffer)) {
            return false;
        }
    }

    return true;
}

bool ParallelSoundTrackWriter::runCombinedFile(FileState& file, const AuxChannels& auxChannels)
{
    IF_ASSERT_FAILED(file.accumulator) {
        return false;
    }

    Accumulator& acc = *file.accumulator;

    const size_t channelCount = m_outputSpec.audioChannelCount;
    const samples_t renderStep = m_outputSpec.samplesPerChannel;
    const size_t bufferSize = renderStep * channelCount;

    for (size_t auxIdx = 0; auxIdx < file.auxUsed.size() && auxIdx < auxChannels.size(); ++auxIdx) {
        if (file.auxUsed.at(auxIdx) && auxChannels.at(auxIdx)) {
            if (FxChainPtr fxChain = auxChannels.at(auxIdx)->fxChain()) {
                fxChain->resetState();
            }
        }
    }

    std::vector<float> mixBuffer(bufferSize, 0.f);
    std::vector<std::vector<float> > auxBuffers(file.auxUsed.size());
    std::vector<bool> auxReceived(file.auxUsed.size(), false);

    for (size_t auxIdx = 0; auxIdx < file.auxUsed.size(); ++auxIdx) {
        if (file.auxUsed.at(auxIdx)) {
            auxBuffers[auxIdx].assign(bufferSize, 0.f);
        }
    }

    if (!encodeSilence(file, m_leadingSilenceSamples, mixBuffer)) {
        return false;
    }

    for (samples_t dataFrame = 0; dataFrame < m_dataSamples; dataFrame += renderStep) {
        if (m_isAborted) {
            return true;
        }

        const samples_t chunk = std::min<samples_t>(renderStep, m_dataSamples - dataFrame);
        const size_t offset = dataFrame * channelCount;
        const size_t chunkSize = chunk * channelCount;

        std::copy(acc.dry.cbegin() + offset, acc.dry.cbegin() + offset + chunkSize, mixBuffer.begin());

        for (size_t auxIdx = 0; auxIdx < file.auxUsed.size(); ++auxIdx) {
            if (!file.auxUsed.at(auxIdx)) {
                continue;
            }

            const auto sendsBegin = acc.auxSends[auxIdx].cbegin() + offset;
            std::copy(sendsBegin, sendsBegin + chunkSize, auxBuffers[auxIdx].begin());
            auxReceived[auxIdx] = std::any_of(sendsBegin, sendsBegin + chunkSize, [](float s) { return s != 0.f; });
        }

        processAuxChannels(file.auxUsed, auxChannels, auxBuffers, auxReceived, chunk, mixBuffer.data());

        if (file.encoder->encode(chunk, mixBuffer.data()) == 0) {
            LOGE() << "Failed to encode";
            return false;
        }

        file.framesWritten += chunk;
    }

    //! NOTE The accumulated audio isn't needed anymore
    acc.dry = std::vector<float>();
    acc.auxSends = std::vector<std::vector<float> >();

    std::fill(mixBuffer.begin(), mixBuffer.end(), 0.f);
    return encodeSilence(file, m_totalSamples - m_leadingSilenceSamples - m_dataSamples, mixBuffer);
}
