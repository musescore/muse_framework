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

namespace muse::audio {
struct SanitizerTag
{
    static constexpr const char* name = "Sanitizer";
};
}

namespace muse::audio::engine {
//! Silences the buffer if bad or loud values are detected in the output
class SanitizerNode : public AudioNode<SanitizerTag>
{
private:
    void doSelfProcess(float* buffer, samples_t samplesPerChannel) override;

    bool m_corruptedOutputAntiSpam = false;
};

using SanitizerNodePtr = std::shared_ptr<SanitizerNode>;
}
