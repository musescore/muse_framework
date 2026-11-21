/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Copyright (C) 2026 MuseScore Limited and others
 */

#ifndef MUSE_AUDIO_FLUIDTREATMENTCONTROLLER_H
#define MUSE_AUDIO_FLUIDTREATMENTCONTROLLER_H

#include <fluidsynth.h>

#include "playbacktreatmentdata.h"

namespace muse::audio::synth {
// Apply various treatments for special playback effects, e.g. simulated articulations.
// Currently only operate at per voice level.
class FluidTreatmentController
{
public:
    static int noteOn(fluid_synth_t* synth, int channel, int key, int velocity, const NoteTreatment& treatment);

private:
    static void voiceCreated(fluid_voice_t* voice, void* data);
    static void applyVoice(fluid_voice_t* voice, int key, const NoteTreatment& treatment);
};
}

#endif // MUSE_AUDIO_FLUIDTREATMENTCONTROLLER_H
