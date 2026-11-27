/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Copyright (C) 2026 MuseScore Limited and others
 */

#include "fluidtreatmentcontroller.h"

using namespace muse::audio::synth;

namespace {
void setGenerator(fluid_voice_t* voice, int generator, float value)
{
    fluid_voice_gen_set(voice, generator, value);
    fluid_voice_update_param(voice, generator);
}

struct NoteTreatmentContext {
    const NoteTreatment& treatment;
    int key;
};
}

int FluidTreatmentController::noteOn(fluid_synth_t* synth, int channel, int key, int velocity,
                                     const NoteTreatment& treatment)
{
    if (treatment.id == NoteTreatmentId::None || velocity == 0) {
        return fluid_synth_noteon(synth, channel, key, velocity);
    }

    NoteTreatmentContext context { treatment, key };
    return fluid_synth_noteon_with_callback(synth, channel, key, velocity, voiceCreated, &context);
}

void FluidTreatmentController::voiceCreated(fluid_voice_t* voice, void* data)
{
    const auto* context = static_cast<const NoteTreatmentContext*>(data);
    applyVoice(voice, context->key, context->treatment);
}

void FluidTreatmentController::applyVoice(fluid_voice_t* voice, int key, const NoteTreatment& treatment)
{
    if (treatment.envelope) {
        const VoiceEnvelopeKey& envelope = treatment.envelope->keys[key];
        setGenerator(voice, GEN_VOLENVDELAY, -12000.0f);
        setGenerator(voice, GEN_VOLENVATTACK, -12000.0f);
        setGenerator(voice, GEN_VOLENVHOLD, envelope.holdTimecents);
        setGenerator(voice, GEN_KEYTOVOLENVHOLD, 0.0f);
        setGenerator(voice, GEN_KEYTOVOLENVDECAY, 0.0f);
        setGenerator(voice, GEN_VOLENVSUSTAIN, 1000.0f);
        setGenerator(voice, GEN_VOLENVDECAY, envelope.decayTimecents);
        setGenerator(voice, GEN_VOLENVRELEASE, treatment.envelope->releaseTimecents);
    }
    fluid_voice_gen_incr(voice, GEN_ATTENUATION, -10.0f * treatment.levelDb);
    fluid_voice_update_param(voice, GEN_ATTENUATION);
}
