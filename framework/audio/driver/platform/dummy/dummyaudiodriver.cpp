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
#include "dummyaudiodriver.h"

#include <chrono>

#include "global/runtime.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;

DummyAudioDriver::~DummyAudioDriver()
{
    close();
}

void DummyAudioDriver::init()
{
}

std::string DummyAudioDriver::name() const
{
    return NAME;
}

AudioDeviceID DummyAudioDriver::defaultDevice() const
{
    return DEFAULT_DEVICE_ID;
}

bool DummyAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    if (isOpened()) {
        close();
    }

    if (!spec.output.isValid() || !spec.callback) {
        LOGE() << "invalid spec: " << spec;
        return false;
    }

    m_activeSpec = spec;
    m_activeSpec.deviceId = DEFAULT_DEVICE_ID;
    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_running = true;
    m_thread = std::thread([this]() { run(); });

    LOGI() << "audio output is disabled, the audio is discarded";
    m_activeSpecChanged.send(m_activeSpec);
    return true;
}

void DummyAudioDriver::run()
{
    runtime::setThreadName("audio_dummy_driver");

    const OutputSpec& output = m_activeSpec.output;
    std::vector<float> buffer(output.samplesPerChannel * output.audioChannelCount);
    const int len = static_cast<int>(buffer.size() * sizeof(float));

    //! NOTE Pull the audio at the real time pace, as a device would do.
    //! Like a device, the first pull is after a period, not immediately on open:
    //! the audio engine is set up in parallel and must not be pulled before it is ready
    const auto period = std::chrono::nanoseconds(1'000'000'000LL * output.samplesPerChannel / output.sampleRate);
    auto next = std::chrono::steady_clock::now() + period;

    while (m_running) {
        std::this_thread::sleep_until(next);
        if (!m_running) {
            break;
        }

        m_activeSpec.callback(reinterpret_cast<uint8_t*>(buffer.data()), len);
        next += period;
    }
}

void DummyAudioDriver::close()
{
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool DummyAudioDriver::isOpened() const
{
    return m_running;
}

const IAudioDriver::Spec& DummyAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

async::Channel<IAudioDriver::Spec> DummyAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

std::vector<samples_t> DummyAudioDriver::availableOutputDeviceBufferSizes() const
{
    return { 256, 512, 1024, 2048, 4096 };
}

std::vector<sample_rate_t> DummyAudioDriver::availableOutputDeviceSampleRates() const
{
    return { 44100, 48000, 88200, 96000 };
}

AudioDeviceList DummyAudioDriver::availableOutputDevices() const
{
    return { AudioDevice { DEFAULT_DEVICE_ID, "Dummy" } };
}

async::Notification DummyAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}
