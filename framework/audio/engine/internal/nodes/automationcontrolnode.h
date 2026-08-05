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

#include <optional>

#include "audionode.h"

#include "audio/engine/iplayhead.h"
#include "audio/engine/internal/dsp/smoothlinearvalue.h"

namespace muse::audio {
struct AutomationControlTag
{
    static constexpr const char* name = "AutomationControl";
};
}

namespace muse::audio::engine {
class AutomationControlNode : public AudioNode<AutomationControlTag>
{
public:

    void setPlayheadPosition(const PlayheadPositionPtr& playheadPosition);
    void setVolume(const AutomatableValue<volume_db_t>& volume);
    void setPan(const AutomatableValue<balance_t>& pan);
    void setMuted(bool muted);
    bool muted() const;

    AutomatedControlParamsChanges automatedControlParamsChanges() const;

protected:

    secs_t currentPos() const;
    void updateChannelGains(secs_t pos);

    void onOutputSpecChanged(const OutputSpec& spec) override;
    void doSelfProcess(float* buffer, samples_t samplesPerChannel) override;

    PlayheadPositionPtr m_playheadPosition;
    AutomatableValue<volume_db_t> m_volume;
    AutomatableValue<balance_t> m_pan;
    std::vector<dsp::SmoothLinearValue<float> > m_channelGains;
    std::optional<secs_t> m_lastGainsPos;

    std::optional<AutomatedControlParams> m_lastSentAutomatedControlParams;
    AutomatedControlParamsChanges m_automatedControlParamsChanges = AutomatedControlParamsChanges(
        async::makeOpt()
        .name("audio::automatedControlParamsChanges")
        .threads(100)
        .disableWaitPendingsOnSend());
};

using AutomationControlNodePtr = std::shared_ptr<AutomationControlNode>;
}
