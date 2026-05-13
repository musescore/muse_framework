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

#include "signalnode.h"

using namespace muse::audio;
using namespace muse::audio::engine;

static constexpr volume_dbfs_t MINIMUM_OPERABLE_DBFS_LEVEL = volume_dbfs_t::make(-100.f);
static constexpr volume_dbfs_t PRESSURE_MINIMAL_VALUABLE_DIFF = volume_dbfs_t::make(2.5f);

bool SignalNode::isSilent() const
{
    return m_isSilent;
}

AudioSignalChanges SignalNode::audioSignalChanges() const
{
    return m_audioSignalChanges;
}

void SignalNode::onOutputSpecChanged(const OutputSpec& spec)
{
    m_channelPeaks.resize(spec.audioChannelCount, 0.f);
}

void SignalNode::doSelfProcess(float* buffer, samples_t samplesPerChannel)
{
    const audioch_t channelsCount = m_outputSpec.audioChannelCount;

    for (audioch_t ch = 0; ch < channelsCount; ++ch) {
        m_channelPeaks[ch] = 0.f;
    }

    for (size_t s = 0; s < samplesPerChannel; ++s) {
        const size_t frameOffset = s * channelsCount;
        for (size_t ch = 0; ch < channelsCount; ++ch) {
            const size_t idx = frameOffset + ch;
            const float a = std::fabs(buffer[idx]);
            if (a > m_channelPeaks[ch]) {
                m_channelPeaks[ch] = a;
            }
        }
    }

    m_isSilent = true;
    for (audioch_t ch = 0; ch < channelsCount; ++ch) {
        float peak = m_channelPeaks[ch];
        if (peak > 0.f) {
            m_isSilent = false;
        }
        updateSignalValue(ch, peak);
    }

    notifyAboutChanges();
}

void SignalNode::updateSignalValue(const audioch_t ch, const float newPeak)
{
    volume_dbfs_t newPressure
        = (newPeak > 0.f) ? volume_dbfs_t(muse::linear_to_db(newPeak)) : MINIMUM_OPERABLE_DBFS_LEVEL;
    newPressure = std::max(newPressure, MINIMUM_OPERABLE_DBFS_LEVEL);

    AudioSignalVal& signalVal = m_signalValuesMap[ch];

    if (muse::is_equal(signalVal.pressure, newPressure)) {
        return;
    }

    if (std::abs(signalVal.pressure - newPressure) < PRESSURE_MINIMAL_VALUABLE_DIFF) {
        return;
    }

    signalVal.pressure = newPressure;
    m_shouldNotifyAboutChanges = true;
}

void SignalNode::notifyAboutChanges()
{
    if (m_shouldNotifyAboutChanges) {
        m_audioSignalChanges.send(m_signalValuesMap);
        m_shouldNotifyAboutChanges = false;
    }
}
