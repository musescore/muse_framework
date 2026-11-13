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

#include <cstdlib>
#include <limits>

#include "audio/common/audiotypes.h"

namespace muse::audio::dsp {
inline float balanceGain(const balance_t balance, const int audioChannelNumber)
{
    return (audioChannelNumber * 2 - 1) * balance + 1.f;
}

template<typename T>
constexpr T convertFloatSamples(const float value, const int bits)
{
    const int64_t max_val = (1LL << (bits - 1)) - 1LL;
    const float clampedValue = std::clamp(value, -1.0f, 1.0f);
    const float scaledValue = clampedValue * max_val;
    return static_cast<T>(std::round(scaledValue));
}
}
