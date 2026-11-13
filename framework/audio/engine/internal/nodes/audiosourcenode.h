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

#include "global/async/notification.h"
#include "global/async/channel.h"
#include "audio/common/audiotypes.h"
#include "audio/common/timeposition.h"

namespace muse::audio {
struct SourceTag
{
    static constexpr const char* name = "Source";
};
}

namespace muse::audio::engine {
//! NOTE Abstract Base Audio Source Node
class AudioSourceNode : public AudioNode<SourceTag>
{
public:

    virtual void seek(const TimePosition& position, const bool flushSound = true) = 0;
    virtual void flush() = 0;

    virtual const AudioInputParams& inputParams() const = 0;
    virtual void applyInputParams(const AudioInputParams& requiredParams) = 0;
    virtual async::Channel<AudioInputParams> inputParamsChanged() const = 0;

    virtual void prepareToPlay() = 0;
    virtual bool readyToPlay() const = 0;
    virtual async::Notification readyToPlayChanged() const = 0;

    virtual bool hasPendingChunks() const = 0;
    virtual void processInput() = 0;
    virtual InputProcessingProgress inputProcessingProgress() const = 0;

    virtual void clearCache() = 0;

    //! NOTE Time of the source's first note, if it has any (used to keep a track idle until then)
    virtual std::optional<secs_t> firstNoteTime() const { return std::nullopt; }
};

using AudioSourceNodePtr = std::shared_ptr<AudioSourceNode>;
}
