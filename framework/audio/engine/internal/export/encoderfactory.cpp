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

#include "encoderfactory.h"

#include "mp3encoder.h"
#include "oggencoder.h"
#include "flacencoder.h"
#include "wavencoder.h"
#include "aacencoder.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::soundtrack;

encode::AbstractAudioEncoderPtr muse::audio::soundtrack::createEncoder(const SoundTrackFormat& format, io::IODevice& dstDevice)
{
    switch (format.type) {
    case SoundTrackType::MP3: return std::make_unique<encode::Mp3Encoder>(format, dstDevice);
    case SoundTrackType::OGG: return std::make_unique<encode::OggEncoder>(format, dstDevice);
    case SoundTrackType::FLAC: return std::make_unique<encode::FlacEncoder>(format, dstDevice);
    case SoundTrackType::WAV: return std::make_unique<encode::WavEncoder>(format, dstDevice);
    case SoundTrackType::AAC: return std::make_unique<encode::AacEncoder>(format, dstDevice);
    case SoundTrackType::Undefined: break;
    }

    UNREACHABLE;
    return nullptr;
}
