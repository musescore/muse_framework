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

#include "fxchain.h"
#include "fxnode.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;

void FxChain::setFxList(const std::vector<IFxProcessorPtr>& fxList)
{
    clear();

    for (auto it = fxList.rbegin(); it != fxList.rend(); ++it) {
        FxNodePtr node = std::make_shared<FxNode>(*it);
        doAdd(node);

        node->paramsChanged().onReceive(this, [this](const AudioFxParams& fxParams) {
            m_fxChainSpec.insert_or_assign(fxParams.chainOrder, fxParams);
            m_fxChainSpecChanged.send(m_fxChainSpec);
            updateShouldProcessDuringSilence();
        }, async::Asyncable::Mode::SetReplace);
    }

    rebuild();
    updateShouldProcessDuringSilence();

    if (!name().empty() && !m_nodes.empty()) {
        LOGD() << name() << ": " << m_nodes.front()->dump();
    }
}

void FxChain::setFxChainSpec(const AudioFxChain& fxChainSpec)
{
    if (m_fxChainSpec == fxChainSpec) {
        return;
    }

    auto findFxNode = [this](const std::pair<AudioFxChainOrder, AudioFxParams>& params) -> FxNodePtr {
        for (auto& node : m_nodes) {
            FxNodePtr fx = std::dynamic_pointer_cast<FxNode>(node);
            IF_ASSERT_FAILED(fx) {
                continue;
            }

            if (fx->params().chainOrder != params.first) {
                continue;
            }

            if (fx->params().resourceMeta == params.second.resourceMeta) {
                return fx;
            }
        }

        return nullptr;
    };

    m_fxChainSpec = fxChainSpec;
    for (auto it = m_fxChainSpec.begin(); it != m_fxChainSpec.end();) {
        if (FxNodePtr fx = findFxNode(*it)) {
            fx->setBypassed(!it->second.active);
            ++it;
        } else {
            it = m_fxChainSpec.erase(it);
        }
    }

    m_fxChainSpecChanged.send(m_fxChainSpec);
    updateShouldProcessDuringSilence();
}

const AudioFxChain& FxChain::fxChainSpec() const
{
    return m_fxChainSpec;
}

async::Channel<AudioFxChain> FxChain::fxChainSpecChanged() const
{
    return m_fxChainSpecChanged;
}

void FxChain::setPlayheadPosition(PlayheadPositionPtr playheadPosition)
{
    for (auto& node : m_nodes) {
        FxNodePtr fx = std::dynamic_pointer_cast<FxNode>(node);
        IF_ASSERT_FAILED(fx) {
            continue;
        }

        fx->setPlayheadPosition(playheadPosition);
    }
}

void FxChain::rebuild()
{
    for (size_t i = 1; i < m_nodes.size(); ++i) {
        IAudioNodePtr& prev = m_nodes.at(i - 1);
        IAudioNodePtr& curr = m_nodes.at(i);
        curr->disconnectAll();
        curr->connect(prev); // prev->input = curr
    }
}

void FxChain::doSelfProcess(float* buffer, samples_t samplesPerChannel)
{
    if (!m_nodes.empty()) {
        m_nodes.front()->process(buffer, samplesPerChannel);
    }
}

void FxChain::updateShouldProcessDuringSilence()
{
    bool shouldProcessDuringSilence = false;
    for (const auto& node : m_nodes) {
        FxNodePtr fx = std::dynamic_pointer_cast<FxNode>(node);
        IF_ASSERT_FAILED(fx) {
            continue;
        }

        if (fx->shouldProcessDuringSilence()) {
            shouldProcessDuringSilence = true;
            break;
        }
    }

    if (m_shouldProcessDuringSilence != shouldProcessDuringSilence) {
        m_shouldProcessDuringSilence = shouldProcessDuringSilence;
        m_shouldProcessDuringSilenceChanged.send(shouldProcessDuringSilence);
    }
}

bool FxChain::shouldProcessDuringSilence() const
{
    return m_shouldProcessDuringSilence;
}

async::Channel<bool> FxChain::shouldProcessDuringSilenceChanged() const
{
    return m_shouldProcessDuringSilenceChanged;
}

//! Whether every effect of the chain has finished loading (see IFxProcessor::isReady()).
bool FxChain::isReady() const
{
    for (const auto& node : m_nodes) {
        FxNodePtr fx = std::dynamic_pointer_cast<FxNode>(node);
        if (fx && !fx->isReady()) {
            return false;
        }
    }

    return true;
}

//! Clears the audio state of every effect of the chain (see IFxProcessor::resetState()).
void FxChain::resetState()
{
    for (auto& node : m_nodes) {
        if (FxNodePtr fx = std::dynamic_pointer_cast<FxNode>(node)) {
            fx->resetState();
        }
    }
}
