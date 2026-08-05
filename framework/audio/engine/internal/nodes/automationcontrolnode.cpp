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

#include "automationcontrolnode.h"

#include "../dsp/audiomathutils.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;

void AutomationControlNode::setPlayheadPosition(const PlayheadPositionPtr& playheadPosition)
{
    m_playheadPosition = playheadPosition;
    m_lastGainsPos = std::nullopt;
}

void AutomationControlNode::setVolume(const AutomatableValue<volume_db_t>& volume)
{
    m_volume = volume;
    updateChannelGains(currentPos());
}

void AutomationControlNode::setPan(const AutomatableValue<balance_t>& pan)
{
    m_pan = pan;
    updateChannelGains(currentPos());
}

void AutomationControlNode::setMuted(bool muted)
{
    setEnabled(!muted);
}

bool AutomationControlNode::muted() const
{
    return !enabled();
}

AutomatedControlParamsChanges AutomationControlNode::automatedControlParamsChanges() const
{
    return m_automatedControlParamsChanges;
}

void AutomationControlNode::onOutputSpecChanged(const OutputSpec&)
{
    updateChannelGains(currentPos());
}

secs_t AutomationControlNode::currentPos() const
{
    return m_playheadPosition ? m_playheadPosition->currentPosition().time() : secs_t(0.0);
}

void AutomationControlNode::updateChannelGains(secs_t pos)
{
    const volume_db_t volumeDb = m_volume.evaluateAt(pos, VOLUME_DB_MIN, VOLUME_DB_MAX);
    const balance_t panValue = m_pan.evaluateAt(pos, BALANCE_MIN, BALANCE_MAX);

    const float volume = muse::db_to_linear(volumeDb.raw());
    const float pan = panValue.raw();

    const audioch_t channelsCount = m_outputSpec.audioChannelCount;
    const bool firstUpdate = !m_lastGainsPos.has_value();
    m_channelGains.resize(channelsCount);

    for (audioch_t audioChNum = 0; audioChNum < channelsCount; ++audioChNum) {
        const float gain = dsp::balanceGain(pan, audioChNum) * volume;
        if (firstUpdate) {
            m_channelGains[audioChNum].initWithValue(gain);
        } else {
            m_channelGains[audioChNum].setTargetValue(gain);
        }
    }

    m_lastGainsPos = pos;

    if (m_volume.hasAutomation() || m_pan.hasAutomation()) {
        //! NOTE Avoid flooding m_automatedControlParamsChanges with updates the UI couldn't display anyway
        constexpr volume_db_t VOLUME_MINIMAL_VALUABLE_DIFF = volume_db_t::make(0.1f);
        constexpr balance_t PAN_MINIMAL_VALUABLE_DIFF = balance_t::make(0.01f);

        bool volumeChanged = false;
        bool panChanged = false;

        if (m_lastSentAutomatedControlParams.has_value()) {
            volumeChanged = std::abs(volumeDb - m_lastSentAutomatedControlParams->volume) >= VOLUME_MINIMAL_VALUABLE_DIFF;
            panChanged = std::abs(panValue - m_lastSentAutomatedControlParams->balance) >= PAN_MINIMAL_VALUABLE_DIFF;
        } else {
            volumeChanged = panChanged = true;
        }

        if (volumeChanged || panChanged) {
            m_lastSentAutomatedControlParams = AutomatedControlParams { volumeDb, panValue };
            m_automatedControlParamsChanges.send(*m_lastSentAutomatedControlParams);
        }
    }
}

void AutomationControlNode::doSelfProcess(float* buffer, samples_t samplesPerChannel)
{
    if (m_volume.hasAutomation() || m_pan.hasAutomation()) {
        const secs_t pos = currentPos();
        if (m_lastGainsPos != pos) {
            updateChannelGains(pos);
        }
    }

    const audioch_t channelsCount = m_outputSpec.audioChannelCount;

    for (size_t s = 0; s < samplesPerChannel; ++s) {
        const size_t frameOffset = s * channelsCount;
        for (size_t ch = 0; ch < channelsCount; ++ch) {
            const size_t idx = frameOffset + ch;
            dsp::SmoothLinearValue<float>& gain = m_channelGains[ch];
            buffer[idx] = buffer[idx] * gain.getValue();
            gain.tick();
        }
    }
}
