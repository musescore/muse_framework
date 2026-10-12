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
#include "../../ifxprocessor.h"
#include "../../iplayhead.h"

namespace muse::audio {
struct FxTag
{
    static constexpr const char* name = "Fx";
};
}

namespace muse::audio::engine {
class FxNode : public AudioNode<FxTag>
{
public:

    explicit FxNode(IFxProcessorPtr fxProcessor, PlayheadPositionPtr playheadPosition = nullptr);

    void setPlayheadPosition(PlayheadPositionPtr playheadPosition);

    const AudioFxParams& params() const;
    async::Channel<audio::AudioFxParams> paramsChanged() const;

    bool shouldProcessDuringSilence() const;

    //! NOTE See IFxProcessor::isReady() / resetState()
    bool isReady() const;
    void resetState();

protected:

    void onModeChanged(const ProcessMode mode) override;
    void onOutputSpecChanged(const OutputSpec& spec) override;

    void doSelfProcess(float* buffer, samples_t samplesPerChannel) override;

    IFxProcessorPtr m_fxProcessor;
    PlayheadPositionPtr m_playheadPosition;
};

using FxNodePtr = std::shared_ptr<FxNode>;
}
