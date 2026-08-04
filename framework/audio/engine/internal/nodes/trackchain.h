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

#include "audionode.h"

#include "audiosourcenode.h"
#include "fxchain.h"
#include "automationcontrolnode.h"
#include "signalnode.h"

namespace muse::audio {
struct TrackChainTag
{
    static constexpr const char* name = "TrackChain";
};
}

namespace muse::audio::engine {
//! NOTE A typical node chain for processing a track:
//! signalnode <- controlnode <- fxnode[] <- audiosource
class TrackChain : public ChainNode<TrackChainTag>
{
public:

    TrackChain(TrackId trackId, const std::string& trackName);

    TrackId trackId() const;
    const std::string& trackName() const;

    void setSource(IAudioNodePtr source);
    IAudioNodePtr source() const;

    void setFxChain(FxChainPtr fxChain);
    FxChainPtr fxChain() const;

    void setControl(AutomationControlNodePtr controlNode);
    AutomationControlNodePtr control() const;

    void setSignal(SignalNodePtr signalNode);
    SignalNodePtr signal() const;

    void rebuild() override;

protected:

    void doSelfProcess(float*, samples_t) override {}

    TrackId m_trackId = INVALID_TRACK_ID;
    std::string m_trackName;

    IAudioNodePtr m_source;
    FxChainPtr m_fxChain;
    AutomationControlNodePtr m_control;
    SignalNodePtr m_signal;
};

using TrackChainPtr = std::shared_ptr<TrackChain>;
}
