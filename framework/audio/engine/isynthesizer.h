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

#pragma once

#include <string>

#include "global/async/channel.h"
#include "global/async/notification.h"

#include "audio/common/audiotypes.h"
#include "audio/common/timeposition.h"

namespace muse::audio::synth {
class ISynthesizer
{
public:
    virtual ~ISynthesizer() = default;

    virtual std::string name() const = 0;
    virtual AudioSourceType type() const = 0;
    virtual bool isValid() const = 0;

    virtual void setMode(const ProcessMode mode) = 0;
    virtual ProcessMode mode() const = 0;

    virtual void setOutputSpec(const OutputSpec& spec) = 0;

    virtual void setup(const mpe::PlaybackData& playbackData) = 0;
    virtual const mpe::PlaybackData& playbackData() const = 0;

    // Optional host track/instrument name, forwarded to instruments that auto-select their sound
    // from it (e.g. via VST3 channel context). Ignored by default.
    virtual void setHostTrackName(const std::string& /*name*/) {}

    virtual const audio::AudioInputParams& params() const = 0;
    virtual async::Channel<audio::AudioInputParams> paramsChanged() const = 0;

    virtual TimePosition playbackPosition() const = 0;
    virtual void setPlaybackPosition(const TimePosition& position) = 0;

    virtual void prepareToPlay() = 0;
    virtual bool readyToPlay() const = 0;
    virtual async::Notification readyToPlayChanged() const = 0;

    virtual void flushSound() = 0;

    virtual bool hasPendingChunks() const = 0;
    virtual void processInput() = 0;
    virtual InputProcessingProgress inputProcessingProgress() const = 0;

    virtual void clearCache() = 0;

    virtual samples_t process(float* buffer, samples_t samplesPerChannel) = 0;
};

using ISynthesizerPtr = std::shared_ptr<ISynthesizer>;
}
