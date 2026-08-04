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

 #include "trackchain.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;

TrackChain::TrackChain(TrackId trackId, const std::string& trackName)
    : m_trackId(trackId), m_trackName(trackName)
{
    setName(std::string(TrackChainTag::name) + "[" + m_trackName + "]");
}

void TrackChain::rebuild()
{
    clear();

    if (m_signal) {
        doAdd(m_signal);
    }
    if (m_control) {
        doAdd(m_control);
    }
    if (m_fxChain) {
        doAdd(m_fxChain);
    }
    if (m_source) {
        doAdd(m_source);
    }

    ChainNode<TrackChainTag>::rebuild();
}

TrackId TrackChain::trackId() const
{
    return m_trackId;
}

const std::string& TrackChain::trackName() const
{
    return m_trackName;
}

void TrackChain::setSource(IAudioNodePtr source)
{
    clear();
    m_source = source;
}

IAudioNodePtr TrackChain::source() const
{
    return m_source;
}

void TrackChain::setFxChain(FxChainPtr fxChain)
{
    clear();
    m_fxChain = fxChain;
}

FxChainPtr TrackChain::fxChain() const
{
    return m_fxChain;
}

void TrackChain::setControl(AutomationControlNodePtr controlNode)
{
    clear();
    m_control = controlNode;
}

AutomationControlNodePtr TrackChain::control() const
{
    return m_control;
}

void TrackChain::setSignal(SignalNodePtr signalNode)
{
    clear();
    m_signal = signalNode;
}

SignalNodePtr TrackChain::signal() const
{
    return m_signal;
}
