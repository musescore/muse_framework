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

#include "audioworkgroup.h"
#include "realtimethreadpool.h"
#include "iaudiodriver.h"
#include "iaudiotaskscheduler.h"
#include "audiosanitizer.h"
#include "global/async/asyncable.h"
#include "global/log.h"
#include <mutex>

namespace muse::audio {
class AudioTaskScheduler : public IAudioTaskScheduler, public muse::async::Asyncable
{
    constexpr static const char* threadpoolName = "audio_rt"; // The name must be no more than 12 characters.
public:
    void submitRealtimeTasksAndWait(const std::vector<Task>& tasks) override
    {
        std::lock_guard lock(m_threadPoolMutex);
        for (const auto& task : tasks) {
            IF_ASSERT_FAILED(m_threadPool->enqueue(task)) {
                task();
            }
        }
        m_threadPool->participateAndWait();
    }

    void setAudioDriver(const IAudioDriverPtr& audioDriver)
    {
        if (!audioDriver) {
            return;
        }
        setWorkgroup(audioDriver->getAudioWorkGroup());
        audioDriver->currentWorkgroupChanged().onNotify(
            this, [this, audioDriverWeak = std::weak_ptr<IAudioDriver>(audioDriver)]() {
            if (auto audioDriver = audioDriverWeak.lock()) {
                setWorkgroup(audioDriver->getAudioWorkGroup());
            }
        });
    }

private:

    void setWorkgroup(const AudioWorkGroup& workGroup)
    {
        std::lock_guard lock(m_threadPoolMutex);
        ensureThreadPoolSize(workGroup);
        m_threadPool->setAudioWorkgroup(workGroup);
    }

    int getIdealThreadCount(AudioWorkGroup workGroup) const
    {
        int bestThreadHint = std::thread::hardware_concurrency();
        if (workGroup.getProvider() != nullptr) {
            bestThreadHint = workGroup.getMaxParallelThreadCount();
        }
        constexpr int hardwareToRealtimeRatio = 2; // This is a heuristic value. The optimal value may vary depending on the workload and system.
        return bestThreadHint > 0 ? static_cast<int>(bestThreadHint / hardwareToRealtimeRatio) : 1;
    }

    void ensureThreadPoolSize(const AudioWorkGroup& currentWorkGroup)
    {
        auto idealWorkerCount = getIdealThreadCount(currentWorkGroup);
        std::lock_guard lock(m_threadPoolMutex);
        if (m_threadPool->getNumberOfWorkers() != idealWorkerCount) {
            m_threadPool = std::make_unique<RealtimeThreadPool>(threadpoolName, idealWorkerCount);
            AudioSanitizer::setMixerThreads(m_threadPool->threadIdSet());
        }
    }

    std::unique_ptr<RealtimeThreadPool> m_threadPool{ std::make_unique<RealtimeThreadPool>(threadpoolName, getIdealThreadCount({})) };

    std::recursive_mutex m_threadPoolMutex;
};
}
