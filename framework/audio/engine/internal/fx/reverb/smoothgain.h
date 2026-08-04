/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2023 MuseScore Limited and others
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

#ifndef MUSE_AUDIO_SMOOTHGAIN_H
#define MUSE_AUDIO_SMOOTHGAIN_H

#include "audio/engine/internal/dsp/smoothlinearvalue.h"
#include "vectorops.h"

namespace muse::audio::fx {
/// helper logic to apply gain ramps to blocks of audio
inline void apply_smooth_gain(dsp::SmoothLinearValue<float>& smooth_value, float** s_in, float** s_out,
                              int num_channels, int num_s)
{
    if (smooth_value.isAtTargetValue()) {
        for (int ch = 0; ch < num_channels; ++ch) {
            vo::constantMultiply(s_in[ch], smooth_value.getTargetValue(), s_out[ch], num_s);
        }
    } else {
        for (int i = 0; i < num_s; ++i) {
            for (int ch = 0; ch < num_channels; ++ch) {
                s_out[ch][i] = s_in[ch][i] * smooth_value.getValue();
            }
            smooth_value.tick();
        }
    }
}
} // namespace muse::audio::fx

#endif // MUSE_AUDIO_SMOOTHGAIN_H
