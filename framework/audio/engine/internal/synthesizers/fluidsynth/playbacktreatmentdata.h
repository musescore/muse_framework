/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Copyright (C) 2026 MuseScore Limited and others
 */

#ifndef MUSE_AUDIO_PLAYBACKTREATMENTDATA_H
#define MUSE_AUDIO_PLAYBACKTREATMENTDATA_H

#include <optional>

#include "mpe/events.h"
#include "guitarcrossnoteenvelope.h"

namespace muse::audio::synth {
enum class NoteTreatmentId {
    None,
    GuitarCrossNote
};

struct VoiceEnvelope {
    const std::array<VoiceEnvelopeKey, 128>& keys;
    float releaseTimecents;
};

struct NoteTreatment {
    NoteTreatmentId id = NoteTreatmentId::None;
    std::optional<VoiceEnvelope> envelope;
    float levelDb = 0.0f;
};

class PlaybackTreatmentResolver
{
public:
    static NoteTreatmentId resolve(const mpe::PlaybackSetupData& setup, mpe::ArticulationType articulation)
    {
        return setup.soundId() == mpe::SoundId::Guitar && articulation == mpe::ArticulationType::CrossNote
               ? NoteTreatmentId::GuitarCrossNote : NoteTreatmentId::None;
    }

    static const NoteTreatment& noteTreatment(NoteTreatmentId id)
    {
        static const NoteTreatment none;
        // Acoustic and electric currently share the muted-electric samples.
        static const NoteTreatment crossNote {
            NoteTreatmentId::GuitarCrossNote, VoiceEnvelope {
                GUITAR_CROSS_NOTE_ENVELOPE, GUITAR_CROSS_NOTE_RELEASE_TIMECENTS
            },
            // To match palm mute sample loudness.
            -4.5f
        };
        return id == NoteTreatmentId::GuitarCrossNote ? crossNote : none;
    }
};
}

#endif // MUSE_AUDIO_PLAYBACKTREATMENTDATA_H
