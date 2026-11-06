/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include <memory>

#include "audio/common/audiotypes.h"

namespace muse::audio {
class IFxProcessor
{
public:
    virtual ~IFxProcessor() = default;

    virtual AudioFxType type() const = 0;
    virtual std::string name() const = 0;

    virtual const AudioFxParams& params() const = 0;
    virtual async::Channel<audio::AudioFxParams> paramsChanged() const = 0;

    virtual void setOutputSpec(const OutputSpec& spec) = 0;

    virtual bool active() const = 0;
    virtual void setActive(bool active) = 0;

    virtual void setMode(const ProcessMode mode) = 0;

    virtual bool shouldProcessDuringSilence() const = 0;

    virtual void process(float* buffer, samples_t sampleCount, samples_t playbackPositionSamples = 0) = 0;

    //! NOTE Whether the processor is fully set up and will actually process audio. Plugins that
    //! load asynchronously (e.g. VST) pass audio through untouched until they report ready.
    virtual bool isReady() const { return true; }

    //! NOTE Clears internal audio state (reverb tails, compressor envelopes, delay lines) so the
    //! next processed block starts from silence, e.g. between two files of a multi-file export.
    virtual void resetState() {}
};

using IFxProcessorPtr = std::shared_ptr<IFxProcessor>;
}
