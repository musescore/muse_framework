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
#include "webaudiodriver.h"

#include <emscripten/val.h>

#include "log.h"

using namespace muse;
using namespace muse::audio;

void WebAudioDriver::init()
{
}

std::string WebAudioDriver::name() const
{
    return "web";
}

AudioDeviceID WebAudioDriver::defaultDevice() const
{
    static const AudioDeviceID id("default");
    return id;
}

bool WebAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    LOGI() << "try open driver";

    emscripten::val::module_property("driver")["open"]();

    emscripten::val specVal = emscripten::val::module_property("driver")["outputSpec"]();
    m_activeSpec = spec;
    m_activeSpec.output.sampleRate = specVal["sampleRate"].as<double>();
    m_activeSpec.output.samplesPerChannel = specVal["samplesPerChannel"].as<int>();
    m_activeSpecChanged.send(m_activeSpec);

    LOGI() << "activeSpec: "
           << "sampleRate: " << m_activeSpec.output.sampleRate
           << ", samplesPerChannel: " << m_activeSpec.output.samplesPerChannel
           << ", audioChannelCount: " << m_activeSpec.output.audioChannelCount
           << " (real: " << specVal["audioChannelCount"].as<int>() << ")";

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_opened = true;
    return true;
}

void WebAudioDriver::close()
{
    m_opened = false;
    emscripten::val::module_property("driver")["close"]();
}

bool WebAudioDriver::isOpened() const
{
    return m_opened;
}

const WebAudioDriver::Spec& WebAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

async::Channel<WebAudioDriver::Spec> WebAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

std::vector<samples_t> WebAudioDriver::availableOutputDeviceBufferSizes() const
{
    std::vector<samples_t> sizes;
    sizes.push_back(m_activeSpec.output.samplesPerChannel);
    return sizes;
}

std::vector<sample_rate_t> WebAudioDriver::availableOutputDeviceSampleRates() const
{
    std::vector<sample_rate_t> sizes;
    sizes.push_back(m_activeSpec.output.sampleRate);
    return sizes;
}

AudioDeviceList WebAudioDriver::availableOutputDevices() const
{
    AudioDeviceList list;
    AudioDevice d;
    d.id = defaultDevice();
    d.name = d.id;
    list.push_back(d);
    return list;
}

async::Notification WebAudioDriver::availableOutputDevicesChanged() const
{
    static async::Notification n;
    return n;
}
