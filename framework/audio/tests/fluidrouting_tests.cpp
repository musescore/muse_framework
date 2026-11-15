/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Copyright (C) 2026 MuseScore Limited and others
 */

#include <gtest/gtest.h>

#include "audio/engine/internal/synthesizers/fluidsynth/fluidsequencer.h"

using namespace muse;
using namespace muse::audio::synth;

static const mpe::PlaybackSetupData GUITAR {
    mpe::SoundId::Guitar, mpe::SoundCategory::Strings, { mpe::SoundSubCategory::Electric }
};

static mpe::NoteEvent noteWithArticulation(mpe::ArticulationType type, mpe::voice_layer_idx_t voice = 0)
{
    mpe::ArrangementContext arrangement;
    arrangement.voiceLayerIndex = voice;
    arrangement.actualDuration = 100000;
    mpe::ExpressionContext expression;
    expression.articulations.emplace(type, mpe::ArticulationAppliedData());
    return mpe::NoteEvent(std::move(arrangement), mpe::PitchContext(), std::move(expression));
}

class Audio_FluidChannelSharing : public testing::TestWithParam<bool>
{
};

TEST_P(Audio_FluidChannelSharing, SameProgramArticulationsReuseChannels)
{
    ChannelMap channels;
    channels.init(GUITAR, std::nullopt, 16);
    const auto normal = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::Standard));
    if (GetParam()) {
        channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote));
    }
    const auto palm = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::PalmMute));
    const auto cross = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote));
    const auto mute = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::Mute));
    EXPECT_NE(normal.channelIdx, cross.channelIdx);
    EXPECT_EQ(palm.channelIdx, cross.channelIdx);
    EXPECT_EQ(mute.channelIdx, cross.channelIdx);
    EXPECT_EQ(mute.articulation, mpe::ArticulationType::Mute);
    EXPECT_EQ(cross.articulation, mpe::ArticulationType::CrossNote);
    EXPECT_EQ(palm.articulation, mpe::ArticulationType::PalmMute);
    EXPECT_EQ(normal.articulation, mpe::ArticulationType::Standard);
    EXPECT_EQ(normal.channelIdx, 0);
    EXPECT_EQ(channels.data().begin()->second.at(mpe::ArticulationType::CrossNote).second, midi::Program(0, 28));
    EXPECT_EQ(channels.channelCount(), 2);
    const auto repeated = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote));
    EXPECT_EQ(repeated.channelIdx, cross.channelIdx);
    EXPECT_EQ(repeated.articulation, mpe::ArticulationType::CrossNote);
}

INSTANTIATE_TEST_SUITE_P(CrossFirst, Audio_FluidChannelSharing, testing::Bool());

TEST(Audio_FluidChannelMap, CrossNotesOnOtherInstrumentsAreUnchanged)
{
    ChannelMap channels;
    channels.init({ mpe::SoundId::Violin, mpe::SoundCategory::Strings }, std::nullopt, 16);
    const auto normal = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::Standard));
    const auto cross = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote));
    EXPECT_EQ(normal.channelIdx, cross.channelIdx);
    EXPECT_EQ(cross.articulation, mpe::ArticulationType::Standard);
}

TEST(Audio_FluidChannelMap, FailedResolutionsStayUndefinedUntilReinitialization)
{
    ChannelMap channels;
    channels.init(GUITAR, std::nullopt, 1);
    for (int request = 0; request < 2; ++request) {
        const auto result = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote));
        EXPECT_EQ(result.channelIdx, 0);
        EXPECT_EQ(result.articulation, mpe::ArticulationType::Undefined);
    }
    EXPECT_FALSE(channels.data().at(0).contains(mpe::ArticulationType::CrossNote));
    channels.init(GUITAR, std::nullopt, 2);
    EXPECT_EQ(channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote)).articulation,
              mpe::ArticulationType::CrossNote);
}

TEST(Audio_FluidChannelMap, FailedStandardResolutionReturnsUndefined)
{
    ChannelMap channels;
    channels.init(GUITAR, std::nullopt, 0);
    const auto result = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::Standard));
    EXPECT_EQ(result.channelIdx, 0);
    EXPECT_EQ(result.articulation, mpe::ArticulationType::Undefined);
}

TEST(Audio_FluidChannelMap, ExhaustedBudgetReusesPalmMuteAcrossLayersWithPerNoteTreatment)
{
    ChannelMap channels;
    channels.init(GUITAR, std::nullopt, 2);
    const auto palm = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::PalmMute));
    const auto cross = channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote, 1));
    EXPECT_EQ(cross.channelIdx, palm.channelIdx);
    EXPECT_EQ(cross.articulation, mpe::ArticulationType::CrossNote);
    EXPECT_EQ(palm.articulation, mpe::ArticulationType::PalmMute);
    EXPECT_EQ(channels.channelCount(), 2);
}

TEST(Audio_FluidChannelMap, ReinitializationClearsNoteRouting)
{
    ChannelMap channels;
    channels.init(GUITAR, std::nullopt, 16);
    channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote));
    channels.init({ mpe::SoundId::Violin, mpe::SoundCategory::Strings }, std::nullopt, 16);
    EXPECT_EQ(channels.channelCount(), 1);
    EXPECT_EQ(channels.resolveChannelForEvent(noteWithArticulation(mpe::ArticulationType::CrossNote)).articulation,
              mpe::ArticulationType::Standard);
}

TEST(Audio_FluidChannelMap, TreatmentUsesFirstMappedArticulation)
{
    ChannelMap channels;
    channels.init(GUITAR, std::nullopt, 16);
    mpe::ExpressionContext expression;
    expression.articulations.emplace(mpe::ArticulationType::Pedal, mpe::ArticulationAppliedData());
    expression.articulations.emplace(mpe::ArticulationType::PalmMute, mpe::ArticulationAppliedData());
    expression.articulations.emplace(mpe::ArticulationType::CrossNote, mpe::ArticulationAppliedData());
    const mpe::NoteEvent note(mpe::ArrangementContext(), mpe::PitchContext(), std::move(expression));
    const auto& articulations = note.expressionCtx().articulations;
    auto first = std::find_if(articulations.begin(), articulations.end(), [](const auto& entry) {
        return entry.first != mpe::ArticulationType::Pedal;
    });
    ASSERT_NE(first, articulations.end());
    const auto mapping = channels.resolveChannelForEvent(note);
    EXPECT_EQ(mapping.articulation, first->first);
    EXPECT_EQ(channels.data().at(0).at(first->first).first, mapping.channelIdx);
}

static std::vector<FluidEvent> eventsWithOpcode(const FluidSequencer::EventSequenceMap& sequences, midi::Event::Opcode opcode)
{
    std::vector<FluidEvent> result;
    for (const auto& [_, sequence] : sequences) {
        for (const auto& event : sequence) {
            const FluidEvent& fluidEvent = std::get<FluidEvent>(event);
            if (fluidEvent.event.opcode() == opcode) {
                result.push_back(fluidEvent);
            }
        }
    }
    return result;
}

static std::vector<FluidEvent> noteOns(const FluidSequencer::EventSequenceMap& sequences)
{
    return eventsWithOpcode(sequences, midi::Event::Opcode::NoteOn);
}

TEST(Audio_FluidSequencer, MainStreamCarriesIndependentTreatmentsOnSharedChannel)
{
    FluidSequencer sequencer;
    sequencer.init(GUITAR, std::nullopt, false, 16);
    mpe::PlaybackData data;
    data.originEvents[0] = {
        noteWithArticulation(mpe::ArticulationType::CrossNote),
        noteWithArticulation(mpe::ArticulationType::PalmMute),
        noteWithArticulation(mpe::ArticulationType::CrossNote)
    };
    sequencer.load(data);
    sequencer.setActive(true);
    const auto sequences = sequencer.movePlaybackForward(msecs_t(200000));
    const auto notes = noteOns(sequences);
    ASSERT_EQ(notes.size(), 3);
    EXPECT_EQ(notes[0].event.channel(), notes[1].event.channel());
    EXPECT_EQ(notes[1].event.channel(), notes[2].event.channel());
    EXPECT_EQ(notes[0].treatmentId, NoteTreatmentId::GuitarCrossNote);
    EXPECT_EQ(notes[1].treatmentId, NoteTreatmentId::None);
    EXPECT_EQ(notes[2].treatmentId, NoteTreatmentId::GuitarCrossNote);

    const auto noteOffs = eventsWithOpcode(sequences, midi::Event::Opcode::NoteOff);
    ASSERT_EQ(noteOffs.size(), 3);
    for (const FluidEvent& event : noteOffs) {
        EXPECT_EQ(event.event.channel(), notes[0].event.channel());
        EXPECT_EQ(event.treatmentId, NoteTreatmentId::None);
    }
}

enum class UntreatedReason {
    ExplicitPreset, ExhaustedBudget, StandardArticulation
};

class Audio_FluidUntreatedRouting : public testing::TestWithParam<UntreatedReason>
{
};

TEST_P(Audio_FluidUntreatedRouting, CrossNotesSkipTreatmentWhenRoutingDisablesIt)
{
    FluidSequencer sequencer;
    // ExplicitPreset: a user-selected preset bypasses articulation routing and treatment.
    const auto preset = GetParam() == UntreatedReason::ExplicitPreset
                        ? std::optional<midi::Program>(midi::Program(0, 24)) : std::nullopt;
    // ExhaustedBudget: the standard channel consumes the only slot, so cross notes fall back to it.
    sequencer.init(GUITAR, preset, false, GetParam() == UntreatedReason::ExhaustedBudget ? 1 : 16);
    mpe::ArrangementContext arrangement;
    arrangement.actualDuration = 100000;
    mpe::ExpressionContext expression;
    // StandardArticulation: Standard takes precedence over CrossNote, so no treatment is selected.
    if (GetParam() == UntreatedReason::StandardArticulation) {
        expression.articulations.emplace(mpe::ArticulationType::Standard, mpe::ArticulationAppliedData());
    }
    expression.articulations.emplace(mpe::ArticulationType::CrossNote, mpe::ArticulationAppliedData());
    mpe::PlaybackData data;
    const mpe::NoteEvent note(std::move(arrangement), mpe::PitchContext(), std::move(expression));
    data.originEvents[0] = { note, note };
    sequencer.load(data);
    sequencer.setActive(true);
    const auto notes = noteOns(sequencer.movePlaybackForward(msecs_t(1)));
    ASSERT_EQ(notes.size(), 2);
    EXPECT_EQ(notes[0].treatmentId, NoteTreatmentId::None);
    EXPECT_EQ(notes[1].treatmentId, NoteTreatmentId::None);
    EXPECT_EQ(notes[0].event.channel(), 0);
    EXPECT_EQ(notes[1].event.channel(), 0);
}

INSTANTIATE_TEST_SUITE_P(DisabledTreatment, Audio_FluidUntreatedRouting,
                         testing::Values(UntreatedReason::ExplicitPreset, UntreatedReason::ExhaustedBudget,
                                         UntreatedReason::StandardArticulation));
