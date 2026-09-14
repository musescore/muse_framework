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
#include "sanitizernode.h"

#include <cmath>
#include <cstring>

#include "global/log.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::engine;

// 24dBFS ceiling, above what volume/pan boost can push a single track to
static constexpr float HARD_LIMIT = 16.0f;

#define MY_ASSERT(reason) if (!m_corruptedOutputAntiSpam) { ASSERT_X(reason); m_corruptedOutputAntiSpam = true; }

void SanitizerNode::doSelfProcess(float* buffer, samples_t samplesPerChannel)
{
    const size_t sampleCount = static_cast<size_t>(samplesPerChannel) * outputSpec().audioChannelCount;
    bool corrupted = false;

    for (size_t i = 0; i < sampleCount; ++i) {
        float x = buffer[i];

        if (!std::isfinite(x)) {
            MY_ASSERT(String(u"!!! CORRUPTED AUDIO: non-finite sample, muting buffer"));
            corrupted = true;
            break;
        } else if (std::abs(x) > HARD_LIMIT) {
            MY_ASSERT(String(u"!!! CORRUPTED AUDIO: sample %1 exceeded hard limit, muting buffer").arg(x));
            corrupted = true;
            break;
        }
    }

    if (corrupted) {
        std::memset(buffer, 0, sampleCount * sizeof(float));
    }
}
