/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
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

#include <memory>

#include "common/iaudiotaskscheduler.h"
#include "global/modularity/ioc.h"
#include "global/async/asyncable.h"

#include "iaudiofactory.h"

#include "nodes/trackchain.h"

#include "muse_framework_config.h"
namespace muse {
class TaskScheduler;
}

namespace muse::audio {
struct ContextMixerTag
{
    static constexpr const char* name = "ContextMixer";
};
}

namespace muse::audio::engine {
class Mixer : public AudioNode<ContextMixerTag>, public async::Asyncable
{
    GlobalInject<IAudioFactory> audioFactory;
#ifdef MUSE_THREADS_SUPPORT
    GlobalInject<IAudioTaskScheduler> audioTaskScheduler;
#endif

public:
    ~Mixer() override;

    void init();

    Ret addTrack(TrackChainPtr trackChain, const AuxSendsParams& auxSends);
    Ret addAuxTrack(TrackChainPtr trackChain);
    Ret removeTrack(const TrackId trackId);

    void setAuxSends(const TrackId trackId, const AuxSendsParams& auxSends);

    void setTracksToProcessWhenIdle(const std::unordered_set<TrackId>& trackIds);
    void setNonMutedTrackCount(size_t count);

    void process(float* buffer, samples_t samplesPerChannel) override;

    std::string dump() const override;

private:

    void onOutputSpecChanged(const OutputSpec& spec) override;
    void onModeChanged(const ProcessMode mode) override;

    void doSelfProcess(float*, samples_t) override {}

    void processTrackChannels(size_t outBufferSize, size_t samplesPerChannel);
    void mixOutputFromChannel(float* outBuffer, const float* inBuffer, size_t bufferSize) const;
    void prepareAuxBuffers(size_t outBufferSize);
    void writeTrackToAuxBuffers(const float* trackBuffer, size_t outBufferSize, const AuxSendsParams& auxSends);
    void processAuxChannels(float* buffer, samples_t samplesPerChannel);

    bool useMultithreading() const;

    struct TrackData {
        TrackId trackId;
        TrackChainPtr chain;
        std::vector<float> buffer;
        bool processed = false;
    };

    std::vector<TrackData> m_tracks;
    std::vector<TrackData> m_auxTracks;
    std::map<TrackId, AuxSendsParams> m_auxSends;

    std::vector<IAudioTaskScheduler::Task> m_trackTasks;

    size_t m_nonMutedTrackCount = 0;
    std::unordered_set<TrackId> m_tracksToProcessWhenIdle;
};

using MixerPtr = std::shared_ptr<Mixer>;
}
