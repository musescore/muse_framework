/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Copyright (C) 2026 MuseScore Limited and others
 */

#include <gtest/gtest.h>

#include <array>
#include <memory>

#include "audio/engine/internal/audioengineconfiguration.h"
#include "audio/engine/internal/synthesizers/fluidsynth/fluidsynth.h"
#include "audio/engine/internal/synthesizers/fluidsynth/fluidtreatmentcontroller.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::synth;

namespace {
// A virtual preset creates two sample layers without an external SoundFont asset.
struct LayeredPreset {
    fluid_sample_t* sample = nullptr;
    std::vector<fluid_voice_t*> voices;

    static int noteOn(fluid_preset_t* preset, fluid_synth_t* synth, int channel, int key, int velocity)
    {
        auto* data = static_cast<LayeredPreset*>(fluid_preset_get_data(preset));
        for (int layer = 0; layer < 2; ++layer) {
            fluid_voice_t* voice = fluid_synth_alloc_voice(synth, data->sample, channel, key, velocity);
            if (!voice) {
                return FLUID_FAILED;
            }
            fluid_voice_gen_set(voice, GEN_VOLENVHOLD, -2000.0f);
            fluid_voice_gen_set(voice, GEN_VOLENVDECAY, -3000.0f);
            fluid_voice_gen_set(voice, GEN_ATTENUATION, 20.0f);
            fluid_synth_start_voice(synth, voice);
            data->voices.push_back(voice);
        }
        return FLUID_OK;
    }

    static fluid_preset_t* preset(fluid_sfont_t* sfont, int bank, int program)
    {
        if (bank != 0 || program != 0) {
            return nullptr;
        }
        auto* result = new_fluid_preset(sfont, [](fluid_preset_t*) { return "layered"; },
                                        [](fluid_preset_t*) { return 0; }, [](fluid_preset_t*) { return 0; },
                                        noteOn, delete_fluid_preset);
        fluid_preset_set_data(result, fluid_sfont_get_data(sfont));
        return result;
    }
};
}

TEST(Audio_FluidTreatmentController, AppliesToEveryNewVoiceWithoutLeakingToUntreatedNotes)
{
    std::unique_ptr<fluid_settings_t, decltype(& delete_fluid_settings)> settings(new_fluid_settings(), delete_fluid_settings);
    ASSERT_NE(settings, nullptr);
    fluid_settings_setint(settings.get(), "synth.reverb.active", 0);
    fluid_settings_setint(settings.get(), "synth.chorus.active", 0);

    std::array<short, 256> samples {};
    std::unique_ptr<fluid_sample_t, decltype(& delete_fluid_sample)> sample(new_fluid_sample(), delete_fluid_sample);
    ASSERT_NE(sample, nullptr);
    ASSERT_EQ(fluid_sample_set_sound_data(sample.get(), samples.data(), nullptr, static_cast<unsigned int>(samples.size()), 44100, 1),
              FLUID_OK);
    ASSERT_EQ(fluid_sample_set_pitch(sample.get(), 60, 0), FLUID_OK);
    LayeredPreset data { sample.get(), {} };
    // Destroy the synth before the sample and callback data it references.
    std::unique_ptr<fluid_synth_t, decltype(& delete_fluid_synth)> synth(new_fluid_synth(settings.get()), delete_fluid_synth);
    ASSERT_NE(synth, nullptr);
    fluid_sfont_t* sfont = new_fluid_sfont([](fluid_sfont_t*) { return "test"; }, LayeredPreset::preset,
                                           nullptr, nullptr, delete_fluid_sfont);
    ASSERT_NE(sfont, nullptr);
    fluid_sfont_set_data(sfont, &data);
    const int sfontId = fluid_synth_add_sfont(synth.get(), sfont);
    ASSERT_GE(sfontId, 0);
    ASSERT_EQ(fluid_synth_program_select(synth.get(), 0, sfontId, 0, 0), FLUID_OK);

    const NoteTreatment& treatment = PlaybackTreatmentResolver::noteTreatment(NoteTreatmentId::GuitarCrossNote);
    ASSERT_EQ(FluidTreatmentController::noteOn(synth.get(), 0, 60, 100, treatment), FLUID_OK);
    ASSERT_EQ(data.voices.size(), 2);
    for (fluid_voice_t* voice : data.voices) {
        EXPECT_FLOAT_EQ(fluid_voice_gen_get(voice, GEN_VOLENVHOLD), GUITAR_CROSS_NOTE_ENVELOPE[60].holdTimecents);
        EXPECT_FLOAT_EQ(fluid_voice_gen_get(voice, GEN_VOLENVDECAY), GUITAR_CROSS_NOTE_ENVELOPE[60].decayTimecents);
        EXPECT_FLOAT_EQ(fluid_voice_gen_get(voice, GEN_VOLENVRELEASE), GUITAR_CROSS_NOTE_RELEASE_TIMECENTS);
        EXPECT_FLOAT_EQ(fluid_voice_gen_get(voice, GEN_VOLENVSUSTAIN), 1000.0f);
        EXPECT_FLOAT_EQ(fluid_voice_gen_get(voice, GEN_ATTENUATION), 65.0f);
    }

    auto expectUntreatedNote = [&](int key) {
        const size_t firstVoice = data.voices.size();
        ASSERT_EQ(FluidTreatmentController::noteOn(synth.get(), 0, key, 100, NoteTreatment {}), FLUID_OK);
        ASSERT_EQ(data.voices.size(), firstVoice + 2);
        for (size_t i = firstVoice; i < data.voices.size(); ++i) {
            EXPECT_FLOAT_EQ(fluid_voice_gen_get(data.voices[i], GEN_VOLENVHOLD), -2000.0f);
            EXPECT_FLOAT_EQ(fluid_voice_gen_get(data.voices[i], GEN_VOLENVDECAY), -3000.0f);
            EXPECT_FLOAT_EQ(fluid_voice_gen_get(data.voices[i], GEN_ATTENUATION), 20.0f);
        }
    };
    expectUntreatedNote(62);

    // Rejected and velocity-zero note-ons must not leave a treatment callback installed.
    EXPECT_EQ(FluidTreatmentController::noteOn(synth.get(), 0, 128, 100, treatment), FLUID_FAILED);
    ASSERT_EQ(fluid_synth_unset_program(synth.get(), 1), FLUID_OK);
    EXPECT_EQ(FluidTreatmentController::noteOn(synth.get(), 1, 60, 100, treatment), FLUID_FAILED);
    EXPECT_EQ(FluidTreatmentController::noteOn(synth.get(), 0, 60, 0, treatment), FLUID_OK);
    EXPECT_EQ(data.voices.size(), 4);
    expectUntreatedNote(64);
}

namespace muse::audio::synth {
class FluidSynthTests : public testing::Test
{
protected:
    static const ChannelMap& channels(const FluidSynth& synth) { return synth.m_sequencer.channels(); }
    static FluidSequencer::EventSequenceMap advance(FluidSynth& synth)
    {
        return synth.m_sequencer.movePlaybackForward(msecs_t(1));
    }
};

TEST_F(FluidSynthTests, OutputSpecChangePreservesQueuedArticulationChannels)
{
    FluidSynth synth { AudioSourceParams {} };
    synth.config.set(std::make_shared<engine::AudioEngineConfiguration>());
    OutputSpec spec;
    spec.sampleRate = 44100;
    ASSERT_TRUE(synth.init(spec));
    mpe::PlaybackData playback;
    playback.setupData = { mpe::SoundId::Guitar, mpe::SoundCategory::Strings, { mpe::SoundSubCategory::Electric } };
    mpe::ArrangementContext arrangement;
    arrangement.actualDuration = 100000;
    mpe::ExpressionContext expression;
    expression.articulations.emplace(mpe::ArticulationType::CrossNote, mpe::ArticulationAppliedData());
    playback.originEvents[0] = { mpe::NoteEvent(std::move(arrangement), mpe::PitchContext(), std::move(expression)) };
    synth.setup(playback);
    synth.setMode(ProcessMode::Playing);
    const auto mappings = channels(synth).data();
    ASSERT_EQ(channels(synth).channelCount(), 2);

    spec.sampleRate = 48000;
    synth.setOutputSpec(spec);
    EXPECT_EQ(channels(synth).data(), mappings);
    EXPECT_EQ(channels(synth).channelCount(), 2);
    const auto events = advance(synth);
    size_t notes = 0;
    for (const auto& [_, sequence] : events) {
        for (const auto& event : sequence) {
            const auto& fluidEvent = std::get<FluidEvent>(event);
            if (fluidEvent.event.opcode() == midi::Event::Opcode::NoteOn) {
                EXPECT_EQ(fluidEvent.event.channel(), mappings.at(0).at(mpe::ArticulationType::CrossNote).first);
                EXPECT_EQ(fluidEvent.treatmentId, NoteTreatmentId::GuitarCrossNote);
                ++notes;
            }
        }
    }
    EXPECT_EQ(notes, 1);
}
}
