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

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "global/async/asyncable.h"

#include "global/modularity/ioc.h"
#include "audio/common/rpc/irpcchannel.h"

#include "audio/common/audiotypes.h"
#include "../nodes/trackchain.h"

#include "abstractaudioencoder.h"

namespace muse::io {
class IODevice;
}

namespace muse::audio::soundtrack {
//! NOTE Renders several audio files (typically one per part, maybe plus the full score) at the same
//! time, on a pool of worker threads. Every file is mixed the way Mixer::process() mixes: its tracks'
//! outputs plus their aux sends run through the aux channels. The result is the same as exporting the
//! file on its own with every other track muted, which is what the per-file export does.
//!
//! Every track is rendered exactly once, by one worker (tracks are borrowed from the live mixer, and
//! a track can't be processed by two threads):
//! - Phase 1 (render jobs, heaviest first): a file whose tracks aren't in any other rendered file is
//!   rendered and encoded directly by one job. Tracks that are also part of a combined file (e.g. the
//!   full score, or a part sharing an instrument with another part) additionally add their output and
//!   aux sends to that combined file's accumulated buffers. Tracks that only belong to combined files
//!   get render-only jobs.
//! - Phase 2 (after all render jobs): each combined file runs its accumulated aux sends through the
//!   aux channels, mixes, and is encoded.
//!
//! The aux channels are shared by every track, so each worker has its own copy of them (AuxChannels),
//! created and fully loaded by the caller, and reset before every file.
class ParallelSoundTrackWriter : public async::Asyncable
{
    muse::GlobalInject<rpc::IRpcChannel> rpcChannel;

public:
    struct Track {
        engine::TrackChainPtr chain;
        AuxSendsParams auxSends;
        std::optional<secs_t> firstNoteTime; // nullopt: no notes at all
        int weight = 1;                      // estimated relative rendering cost, for scheduling
    };

    struct File {
        std::vector<size_t> tracks; // indices into the track list
        io::IODevice* dstDevice = nullptr;
    };

    struct Options {
        //! NOTE Don't process a track until shortly before its first note (it's silent until then)
        bool idleUntilFirstNote = true;
        secs_t idlePreRoll = 0.1;
    };

    //! NOTE One worker's copy of the aux channels, indexed like AuxSendsParams.
    //! A null entry means that aux channel has no fx chain, which Mixer skips as well
    using AuxChannels = std::vector<engine::TrackChainPtr>;

    //! NOTE The number of workers is auxChannelsPerWorker.size()
    ParallelSoundTrackWriter(std::vector<Track> tracks, std::vector<File> files, std::vector<AuxChannels> auxChannelsPerWorker,
                             const SoundTrackFormat& format, const secs_t totalDuration, const Options& options);

    //! NOTE Renders and encodes all files; blocks until done, cancelled or failed
    Ret write();
    //! NOTE Can be called from another thread; write() then returns Ret::Code::Cancel
    void abort();

    //! NOTE Overall progress of write(), in percent
    Progress progress();

    //! NOTE Per file, in the order of the files given to the constructor: [0; 100]
    std::vector<int> filesProgress() const;

    //! NOTE Sent on the thread that calls write(): file index, percent [0; 100]
    async::Channel<size_t, int> fileProgressChanged() const;

private:
    struct Accumulator {
        std::mutex mutex;
        std::vector<float> dry;                    // m_dataSamples * channels
        std::vector<std::vector<float> > auxSends; // per aux channel, empty if unused
    };

    struct FileState {
        File file;
        encode::AbstractAudioEncoderPtr encoder;
        std::vector<bool> auxUsed;                 // per aux channel
        std::unique_ptr<Accumulator> accumulator;  // combined files only
        std::atomic<samples_t> framesWritten = 0;
        int weight = 0;
    };

    struct RenderJob {
        std::optional<size_t> fileIdx; // a directly rendered file, or none for render-only jobs
        std::vector<size_t> tracks;
        int weight = 0;
    };

    //! NOTE Worker thread: render jobs first, then combined files once all render jobs are done
    void workerLoop(size_t workerIdx);
    //! NOTE Renders the job's tracks; encodes its file, if any, and feeds the combined files
    bool runRenderJob(const RenderJob& job, const AuxChannels& auxChannels);
    //! NOTE Mixes a combined file from its accumulated buffers and encodes it
    bool runCombinedFile(FileState& file, const AuxChannels& auxChannels);

    //! NOTE Processes one block of a track; false if it produced no sound
    bool renderTrack(size_t trackIdx, samples_t dataFrame, samples_t chunk, std::vector<float>& trackBuffer);
    //! NOTE Adds a track's block and aux sends to the combined files it belongs to
    void contributeToCombinedFiles(size_t trackIdx, samples_t dataFrame, samples_t chunk, const float* trackBuffer);
    //! NOTE Runs the aux sends through the aux channels and adds them to the mix, like Mixer::processAuxChannels()
    void processAuxChannels(const std::vector<bool>& auxUsed, const AuxChannels& auxChannels, std::vector<std::vector<float> >& auxBuffers,
                            const std::vector<bool>& auxReceived, samples_t chunk, float* mixBuffer);
    //! NOTE Encodes the leading/trailing silence of a file
    bool encodeSilence(FileState& file, samples_t frames, std::vector<float>& silenceBuffer);

    std::vector<Track> m_tracks;
    std::vector<std::unique_ptr<FileState> > m_files;
    std::vector<std::vector<size_t> > m_trackCombinedFiles; // per track: combined files that need its output
    std::vector<samples_t> m_trackStartFrames;               // per track: first frame it's processed at
    std::vector<RenderJob> m_renderJobs;
    std::vector<size_t> m_combinedFiles;
    std::vector<AuxChannels> m_auxChannelsPerWorker;
    Options m_options;

    OutputSpec m_outputSpec;
    samples_t m_leadingSilenceSamples = 0;
    samples_t m_dataSamples = 0;
    samples_t m_totalSamples = 0;
    samples_t m_idlePreRollSamples = 0;

    std::atomic<size_t> m_nextRenderJobIdx = 0;
    std::atomic<size_t> m_renderJobsDone = 0;
    std::atomic<size_t> m_nextCombinedFileIdx = 0;
    std::atomic<samples_t> m_renderOnlyFramesDone = 0;
    std::atomic<bool> m_hasEncodeError = false;
    std::atomic<bool> m_isAborted = false;

    Progress m_progress;
    async::Channel<size_t, int> m_fileProgressChanged;
};
}
