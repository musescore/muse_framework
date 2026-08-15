/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2022 MuseScore Limited and others
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

#include "vstsequencer.h"

#include "global/interpolation.h"

#include <map>

using namespace muse;
using namespace muse::vst;

static constexpr ControlIdx MODWHEEL_IDX = static_cast<ControlIdx>(Steinberg::Vst::kCtrlModWheel);
static constexpr ControlIdx SUSTAIN_IDX = static_cast<ControlIdx>(Steinberg::Vst::kCtrlSustainOnOff);
static constexpr ControlIdx SOSTENUTO_IDX = static_cast<ControlIdx>(Steinberg::Vst::kCtrlSustenutoOnOff);
static constexpr ControlIdx PITCH_BEND_IDX = static_cast<ControlIdx>(Steinberg::Vst::kPitchBend);

static const mpe::ArticulationTypeSet SUSTAIN_PEDAL_CC_SUPPORTED_TYPES {
    mpe::ArticulationType::Pedal,
};

static const mpe::ArticulationTypeSet SOSTENUTO_PEDAL_CC_SUPPORTED_TYPES {
    mpe::ArticulationType::LaissezVibrer,
};

static const mpe::ArticulationTypeSet BEND_SUPPORTED_TYPES {
    mpe::ArticulationType::Multibend, mpe::ArticulationType::ContinuousGlissando,
};

// "Span" articulations expand into many rapid sub-notes sharing one meta.timestamp; an instrument
// may render them as a single sustained gesture. A new span re-sends the same keyswitch so the
// instrument can retrigger. Other articulations are one note per keyswitch and never retrigger.
static const mpe::ArticulationTypeSet SPAN_ARTICULATION_TYPES {
    mpe::ArticulationType::Tremolo8th, mpe::ArticulationType::Tremolo16th,
    mpe::ArticulationType::Tremolo32nd, mpe::ArticulationType::Tremolo64th,
};

static bool isSpanArticulation(mpe::ArticulationType type)
{
    return muse::contains(SPAN_ARTICULATION_TYPES, type);
}

// Keyswitch note the profile assigns to this articulation, or nullopt if it maps none.
static std::optional<int> keyswitchFor(const VstKeyswitchProfile& profile, mpe::ArticulationType type)
{
    auto it = profile.keyswitches.find(type);
    return it != profile.keyswitches.cend() ? std::optional<int>(it->second) : std::nullopt;
}

void VstSequencer::init(ParamsMapping&& mapping, bool useDynamicEvents, std::optional<VstKeyswitchProfile> keyswitchProfile)
{
    m_mapping = std::move(mapping);
    m_useDynamicEvents = useDynamicEvents;
    m_keyswitchProfile = std::move(keyswitchProfile);
    m_inited = true;

    updateMainStreamEvents(m_playbackData.originEvents, m_playbackData.dynamics);
}

void VstSequencer::updateMainStreamEvents(const mpe::PlaybackEventsMap& events, const mpe::DynamicAutomationLayers& dynamics)
{
    if (!m_inited) {
        return;
    }

    m_mainStreamEvents.clear();

    if (m_onMainStreamFlushed) {
        m_onMainStreamFlushed();
    }

    addPlaybackEvents(m_mainStreamEvents, events);
    sortNoteOnEventsByPitch(m_mainStreamEvents);

    if (m_useDynamicEvents) {
        addDynamicEvents(m_mainStreamEvents, dynamics);
    }

    updateMainSequenceIterator();
}

void VstSequencer::updateOffStreamEvents(const mpe::PlaybackEventsMap& events)
{
    addPlaybackEvents(m_offStreamEvents, events);
    sortNoteOnEventsByPitch(m_offStreamEvents);
    updateOffSequenceIterator();
}

muse::audio::gain_t VstSequencer::currentGain() const
{
    if (!m_useDynamicEvents) {
        return 0.5f;
    }

    mpe::dynamic_level_t maxDynamicLevel = mpe::MIN_DYNAMIC_LEVEL;
    bool foundAnyLevel = false;

    for (const auto& [_, curve] : m_playbackData.dynamics) {
        if (curve.empty()) {
            continue;
        }

        foundAnyLevel = true;
        maxDynamicLevel = std::max(maxDynamicLevel, mpe::dynamicLevelFromNormalized(mpe::evaluateCurveAt(curve, m_playbackPosition)));
    }

    if (!foundAnyLevel) {
        maxDynamicLevel = mpe::dynamicLevelFromType(mpe::DynamicType::Natural);
    }

    return expressionLevel(maxDynamicLevel);
}

void VstSequencer::addPlaybackEvents(EventSequenceMap& destination, const mpe::PlaybackEventsMap& events)
{
    SostenutoTimeAndDurations sostenutoTimeAndDurations;
    LastKeyswitchPerTimestamp lastKeyswitch;

    for (const auto& evPair : events) {
        for (const mpe::PlaybackEvent& event : evPair.second) {
            if (std::holds_alternative<mpe::NoteEvent>(event)) {
                addNoteEvent(destination, std::get<mpe::NoteEvent>(event), sostenutoTimeAndDurations, lastKeyswitch);
            } else if (std::holds_alternative<mpe::ControllerChangeEvent>(event)) {
                addControlChangeEvent(destination, evPair.first, std::get<mpe::ControllerChangeEvent>(event));
            }
        }
    }

    addSostenutoEvents(destination, sostenutoTimeAndDurations);
}

void VstSequencer::addDynamicEvents(EventSequenceMap& destination, const mpe::DynamicAutomationLayers& layers)
{
    constexpr mpe::timestamp_t STEP_INTERVAL_US = 30000;

    //! NOTE: VST instance has a single gain parameter, so merge layers by tracking each one's last-known level
    std::map<mpe::timestamp_t, std::vector<std::pair<mpe::layer_idx_t, mpe::dynamic_level_t> > > updatesAt;
    for (const auto& [layerIdx, curve] : layers) {
        mpe::resampleCurve(curve, STEP_INTERVAL_US, [&](mpe::timestamp_t t, muse::real_t normalized) {
            updatesAt[t].emplace_back(layerIdx, mpe::dynamicLevelFromNormalized(normalized));
        });
    }

    std::map<mpe::layer_idx_t, mpe::dynamic_level_t> currentLevel;
    std::optional<float> lastGain;

    for (const auto& [t, updates] : updatesAt) {
        for (const auto& [layerIdx, level] : updates) {
            currentLevel[layerIdx] = level;
        }

        mpe::dynamic_level_t maxDynamicLevel = mpe::MIN_DYNAMIC_LEVEL;
        for (const auto& [_, level] : currentLevel) {
            maxDynamicLevel = std::max(maxDynamicLevel, level);
        }

        const float gain = expressionLevel(maxDynamicLevel);
        if (lastGain.has_value() && muse::RealIsEqual(*lastGain, gain)) {
            continue;
        }
        lastGain = gain;
        destination[t].emplace_back(gain);
    }
}

void VstSequencer::addNoteEvent(EventSequenceMap& destination, const mpe::NoteEvent& noteEvent,
                                SostenutoTimeAndDurations& sostenutoTimeAndDurations,
                                LastKeyswitchPerTimestamp& lastKeyswitch)
{
    const mpe::ArrangementContext& arrangementCtx = noteEvent.arrangementCtx();
    const int32_t noteId = noteIndex(noteEvent.pitchCtx().nominalPitchLevel);
    const float velocityFraction = noteVelocityFraction(noteEvent);
    const float tuning = noteTuning(noteEvent, noteId);

    if (arrangementCtx.hasStart()) {
        if (m_useDynamicEvents) {
            destination[arrangementCtx.actualTimestamp].emplace_back(expressionLevel(noteEvent.expressionCtx().nominalDynamicLevel));
        }

        destination[arrangementCtx.actualTimestamp].emplace_back(buildEvent(VstEvent::kNoteOnEvent, noteId, velocityFraction, tuning));
    }

    if (arrangementCtx.hasEnd()) {
        const mpe::timestamp_t timestampTo = arrangementCtx.actualTimestamp + noteEvent.arrangementCtx().actualDuration;
        destination[timestampTo].emplace_back(buildEvent(VstEvent::kNoteOffEvent, noteId, velocityFraction, tuning));
    }

    // Latching keyswitch with span retrigger: send a keyswitch NoteOn when the articulation changes,
    // or when a span articulation starts a new span. The keyswitch persists until a different one is
    // sent (no NoteOff); re-sending the same one signals a span retrigger.
    if (m_keyswitchProfile.has_value() && arrangementCtx.hasStart()) {
        const VstKeyswitchProfile& profile = *m_keyswitchProfile;
        // Default to the Standard keyswitch, or to nothing when the instrument does not advertise it:
        // never fabricate a pitch the instrument did not advertise.
        std::optional<int> keyswitchPitch = keyswitchFor(profile, mpe::ArticulationType::Standard);
        mpe::timestamp_t spanStart = -1;

        // Pick by precedence: a specific timbre (pizzicato, mute, harmonic, staccato...) beats a
        // tremolo, which beats normal. On a tie between two primary timbres on one note, take the
        // lowest keyswitch note, so the choice is deterministic and not the hash map's iteration order.
        int bestRank = -1;
        for (const auto& artPair : noteEvent.expressionCtx().articulations) {
            if (mpe::isRangedArticulation(artPair.first)) {
                continue; // forwarded as its own span below, never the primary keyswitch
            }
            const std::optional<int> mapped = keyswitchFor(profile, artPair.first);
            if (!mapped.has_value()) {
                continue;
            }
            const bool span = isSpanArticulation(artPair.first);
            const bool standard = (artPair.first == mpe::ArticulationType::Standard);
            const int rank = standard ? 0 : (span ? 1 : 2);
            if (rank > bestRank || (rank == bestRank && mapped.value() < keyswitchPitch.value())) {
                bestRank = rank;
                keyswitchPitch = mapped;
                spanStart = span ? artPair.second.meta.timestamp : -1;
            }
        }

        // upper_bound, not lower_bound: std::prev then yields the most recent state at or before this
        // timestamp even when an earlier note of the same chord already inserted an entry at it, so a
        // chord sharing one articulation does not re-emit a duplicate keyswitch per note.
        auto it = lastKeyswitch.upper_bound(arrangementCtx.actualTimestamp);
        LastKeyswitchState prevState;
        if (it != lastKeyswitch.begin()) {
            prevState = std::prev(it)->second;
        }

        if (keyswitchPitch.has_value()
            && (keyswitchPitch.value() != prevState.keyswitch || (spanStart != -1 && spanStart != prevState.spanStart))) {
            // Full velocity: a keyswitch is a control signal, not a note. A soft dynamic rounds to
            // velocity 0, which a receiver reads as a note-off and drops, losing the switch.
            destination[arrangementCtx.actualTimestamp].emplace_back(
                buildEvent(VstEvent::kNoteOnEvent, keyswitchPitch.value(), 1.f, 0.f));
            lastKeyswitch[arrangementCtx.actualTimestamp] = { keyswitchPitch.value(), spanStart };
        }
    }

    for (const auto& artPair : noteEvent.expressionCtx().articulations) {
        if (artPair.first == mpe::ArticulationType::Standard) {
            continue;
        }

        const mpe::ArticulationMeta& meta = artPair.second.meta;

        if (!noteEvent.pitchCtx().pitchCurve.empty() && muse::contains(BEND_SUPPORTED_TYPES, meta.type)) {
            addPitchCurve(destination, noteEvent, meta);
            continue;
        }

        if (muse::contains(SUSTAIN_PEDAL_CC_SUPPORTED_TYPES, meta.type)) {
            addPedalEvent(destination, meta);
            continue;
        }

        if (muse::contains(SOSTENUTO_PEDAL_CC_SUPPORTED_TYPES, meta.type)) {
            const mpe::timestamp_t timestamp = arrangementCtx.actualTimestamp + noteEvent.arrangementCtx().actualDuration * 0.1; // add offset for Sostenuto to take effect
            sostenutoTimeAndDurations.push_back(mpe::TimestampAndDuration { timestamp, meta.overallDuration });
            continue;
        }

        // A ranged articulation the instrument advertises as a keyswitch is forwarded as a keyswitch
        // spanning the range: pressed at the start, released at the end. The instrument decides what
        // the modifier means; the sequencer only reports when it is active.
        if (m_keyswitchProfile.has_value() && mpe::isRangedArticulation(meta.type)) {
            if (const std::optional<int> pitch = keyswitchFor(*m_keyswitchProfile, meta.type)) {
                addKeyswitchSpanEvent(destination, meta, arrangementCtx.actualTimestamp, *pitch);
            }
            continue;
        }
    }
}

void VstSequencer::addKeyswitchSpanEvent(EventSequenceMap& destination, const mpe::ArticulationMeta& meta,
                                         const mpe::timestamp_t noteTimestamp, int keyswitchPitch)
{
    // Press at every covered note's onset, not once at the range start, so starting playback partway
    // through the range still engages it. Dedup within one onset (a chord shares it), like the
    // release below, so we do not queue N identical note-ons at one instant. Full velocity (a
    // keyswitch is a control signal, see above).
    EventSequence& onset = destination[noteTimestamp];
    bool alreadyPressed = false;
    for (const EventType& queued : onset) {
        if (std::holds_alternative<VstEvent>(queued)) {
            const VstEvent& ev = std::get<VstEvent>(queued);
            if (ev.type == VstEvent::kNoteOnEvent && ev.noteOn.pitch == keyswitchPitch) {
                alreadyPressed = true;
                break;
            }
        }
    }
    if (!alreadyPressed) {
        onset.emplace_back(buildEvent(VstEvent::kNoteOnEvent, keyswitchPitch, 1.f, 0.f));
    }

    if (meta.hasEnd()) {
        // Dedup the release: every covered note computes the same range-end, so without this we queue
        // N identical note-offs at one instant and can overflow the host block's fixed-size event list.
        EventSequence& bucket = destination[meta.timestamp + meta.overallDuration];
        bool alreadyReleased = false;
        for (const EventType& queued : bucket) {
            if (std::holds_alternative<VstEvent>(queued)) {
                const VstEvent& ev = std::get<VstEvent>(queued);
                if (ev.type == VstEvent::kNoteOffEvent && ev.noteOff.pitch == keyswitchPitch) {
                    alreadyReleased = true;
                    break;
                }
            }
        }
        if (!alreadyReleased) {
            bucket.emplace_back(buildEvent(VstEvent::kNoteOffEvent, keyswitchPitch, 1.f, 0.f));
        }
    }
}

void VstSequencer::addPedalEvent(EventSequenceMap& destination, const mpe::ArticulationMeta& meta)
{
    if (meta.hasStart()) {
        addParamChange(destination, meta.timestamp, SUSTAIN_IDX, 1);
    }

    if (meta.hasEnd()) {
        addParamChange(destination, meta.timestamp + meta.overallDuration, SUSTAIN_IDX, 0);
    }
}

void VstSequencer::addControlChangeEvent(EventSequenceMap& destination, const mpe::timestamp_t timestamp,
                                         const mpe::ControllerChangeEvent& event)
{
    switch (event.type) {
    case mpe::ControllerChangeEvent::Modulation:
        addParamChange(destination, timestamp, MODWHEEL_IDX, event.val);
        break;
    case mpe::ControllerChangeEvent::SustainPedalOnOff:
        addParamChange(destination, timestamp, SUSTAIN_IDX, event.val);
        break;
    case mpe::ControllerChangeEvent::PitchBend:
        addParamChange(destination, timestamp, PITCH_BEND_IDX, event.val);
        break;
    case mpe::ControllerChangeEvent::Undefined:
        break;
    }
}

void VstSequencer::addParamChange(EventSequenceMap& destination, const mpe::timestamp_t timestamp,
                                  const ControlIdx controlIdx, const PluginParamValue value)
{
    auto controlIt = m_mapping.find(controlIdx);
    if (controlIt == m_mapping.cend()) {
        return;
    }

    const PluginParamId paramId = controlIt->second;
    EventSequence& events = destination[timestamp];

    for (const EventType& e : events) {
        if (!std::holds_alternative<ParamChangeEvent>(e)) {
            continue;
        }

        const ParamChangeEvent& pce = std::get<ParamChangeEvent>(e);
        if (pce.paramId == paramId && RealIsEqual(pce.value, value)) {
            return;
        }
    }

    events.emplace_back(ParamChangeEvent { paramId, value });
}

void VstSequencer::addPitchCurve(EventSequenceMap& destination, const mpe::NoteEvent& noteEvent,
                                 const mpe::ArticulationMeta& artMeta)
{
    auto pitchBendIt = m_mapping.find(PITCH_BEND_IDX);
    if (pitchBendIt == m_mapping.cend()) {
        return;
    }

    const mpe::timestamp_t noteTimestampTo = noteEvent.arrangementCtx().actualTimestamp + noteEvent.arrangementCtx().actualDuration;
    const mpe::timestamp_t pitchBendTimestampTo = std::min(artMeta.timestamp + artMeta.overallDuration, noteTimestampTo);

    ParamChangeEvent event;
    event.paramId = pitchBendIt->second;
    event.value = 0.5f;
    destination[pitchBendTimestampTo].push_back(event);

    auto currIt = noteEvent.pitchCtx().pitchCurve.cbegin();
    auto nextIt = std::next(currIt);
    auto endIt = noteEvent.pitchCtx().pitchCurve.cend();

    float prevBendValue = -1.f;

    for (; nextIt != endIt; currIt = nextIt, nextIt = std::next(currIt)) {
        const float currValue = pitchBendLevel(currIt->second);
        const float nextValue = pitchBendLevel(nextIt->second);

        const mpe::timestamp_t currTime = artMeta.timestamp + artMeta.overallDuration * mpe::percentageToFactor(currIt->first);
        const mpe::timestamp_t nextTime = artMeta.timestamp + artMeta.overallDuration * mpe::percentageToFactor(nextIt->first);

        using namespace muse::interpolation;
        const Point currPoint { static_cast<double>(currTime), currValue };
        const Point nextPoint { static_cast<double>(nextTime), nextValue };

        //! NOTE: Increasing this number results in fewer points being interpolated
        constexpr mpe::pitch_level_t POINT_WEIGHT = mpe::PITCH_LEVEL_STEP / 25;
        size_t pointCount = std::abs(nextIt->second - currIt->second) / POINT_WEIGHT;
        pointCount = std::max(pointCount, size_t(1));

        const std::vector<Point> points = lerp(currPoint, nextPoint, pointCount);

        for (const Point& point : points) {
            const mpe::timestamp_t time = static_cast<mpe::timestamp_t>(std::round(point.x));
            const float bendValue = static_cast<float>(point.y);

            if (time < pitchBendTimestampTo && !RealIsEqual(prevBendValue, bendValue)) {
                event.value = bendValue;
                destination[time].push_back(event);
            }

            prevBendValue = bendValue;
        }
    }
}

void VstSequencer::addSostenutoEvents(EventSequenceMap& destination, const SostenutoTimeAndDurations& sostenutoTimeAndDurations)
{
    for (size_t i = 0; i < sostenutoTimeAndDurations.size(); ++i) {
        const mpe::TimestampAndDuration& currentTnD = sostenutoTimeAndDurations.at(i);
        const mpe::timestamp_t timestampTo = currentTnD.timestamp + currentTnD.duration;

        addParamChange(destination, currentTnD.timestamp, SOSTENUTO_IDX, 1);

        if (i == sostenutoTimeAndDurations.size() - 1) {
            addParamChange(destination, timestampTo, SOSTENUTO_IDX, 0);
            continue;
        }

        const mpe::TimestampAndDuration& nextTnD = sostenutoTimeAndDurations.at(i + 1);
        if (timestampTo <= nextTnD.timestamp) { // handle potential overlap
            addParamChange(destination, timestampTo, SOSTENUTO_IDX, 0);
        }
    }
}

//! Hack to make keyswitches work until we have proper UI support
//! see: https://github.com/musescore/MuseScore/issues/32150
void VstSequencer::sortNoteOnEventsByPitch(EventSequenceMap& destination)
{
    for (auto& [_, seq] : destination) {
        if (seq.size() <= 1) {
            continue;
        }

        std::stable_sort(seq.begin(), seq.end(), [](const EventType& e1, const EventType& e2) {
            if (!std::holds_alternative<VstEvent>(e1) || !std::holds_alternative<VstEvent>(e2)) {
                return false;
            }

            const VstEvent& ve1 = std::get<VstEvent>(e1);
            const VstEvent& ve2 = std::get<VstEvent>(e2);

            if (ve1.type == VstEvent::kNoteOnEvent && ve2.type == VstEvent::kNoteOnEvent) {
                return ve1.noteOn.pitch < ve2.noteOn.pitch;
            }

            return false;
        });
    }
}

VstEvent VstSequencer::buildEvent(const VstEvent::EventTypes type, const int32_t noteIdx, const float velocityFraction,
                                  const float tuning) const
{
    VstEvent result;

    result.busIndex = 0;
    result.sampleOffset = 0;
    result.ppqPosition = 0;
    result.flags = VstEvent::kIsLive;
    result.type = type;

    if (type == VstEvent::kNoteOnEvent) {
        result.noteOn.noteId = -1;
        result.noteOn.channel = 0;
        result.noteOn.pitch = noteIdx;
        result.noteOn.tuning = tuning;
        result.noteOn.velocity = velocityFraction;
    } else {
        result.noteOff.noteId = -1;
        result.noteOff.channel = 0;
        result.noteOff.pitch = noteIdx;
        result.noteOff.tuning = tuning;
        result.noteOff.velocity = velocityFraction;
    }

    return result;
}

int32_t VstSequencer::noteIndex(const mpe::pitch_level_t pitchLevel) const
{
    float stepCount = mpe::ZERO_PITCH_LEVEL_MIDI_EQUIVALENT + pitchLevel / static_cast<float>(mpe::PITCH_LEVEL_STEP);

    return std::clamp(stepCount, 0.f, 127.f);
}

float VstSequencer::noteTuning(const mpe::NoteEvent& noteEvent, const int noteIdx) const
{
    int semitonesCount = noteIdx - mpe::ZERO_PITCH_LEVEL_MIDI_EQUIVALENT;

    mpe::pitch_level_t tuningPitchLevel = noteEvent.pitchCtx().nominalPitchLevel - semitonesCount * mpe::PITCH_LEVEL_STEP;

    return (tuningPitchLevel / static_cast<float>(mpe::PITCH_LEVEL_STEP)) * 100.f;
}

float VstSequencer::noteVelocityFraction(const mpe::NoteEvent& noteEvent) const
{
    const mpe::ExpressionContext& expressionCtx = noteEvent.expressionCtx();

    if (expressionCtx.velocityOverride.has_value()) {
        return std::clamp(expressionCtx.velocityOverride.value(), 0.f, 1.f);
    }

    mpe::dynamic_level_t dynamicLevel = expressionCtx.expressionCurve.empty()
                                        ? expressionCtx.nominalDynamicLevel : expressionCtx.expressionCurve.maxAmplitudeLevel();
    return expressionLevel(dynamicLevel);
}

float VstSequencer::expressionLevel(const mpe::dynamic_level_t dynamicLevel) const
{
    static constexpr mpe::dynamic_level_t MIN_SUPPORTED_DYNAMIC_LEVEL = mpe::dynamicLevelFromType(mpe::DynamicType::ppp);
    static constexpr mpe::dynamic_level_t MAX_SUPPORTED_DYNAMIC_LEVEL = mpe::dynamicLevelFromType(mpe::DynamicType::fff);
    static constexpr mpe::dynamic_level_t AVAILABLE_RANGE = MAX_SUPPORTED_DYNAMIC_LEVEL - MIN_SUPPORTED_DYNAMIC_LEVEL;

    if (dynamicLevel <= MIN_SUPPORTED_DYNAMIC_LEVEL) {
        return (0.5f * mpe::ONE_PERCENT) / AVAILABLE_RANGE;
    }

    if (dynamicLevel >= MAX_SUPPORTED_DYNAMIC_LEVEL) {
        return 1.f;
    }

    return RealRound((dynamicLevel - MIN_SUPPORTED_DYNAMIC_LEVEL) / static_cast<float>(AVAILABLE_RANGE), 2);
}

float VstSequencer::pitchBendLevel(const mpe::pitch_level_t pitchLevel) const
{
    static constexpr float SEMITONE_RANGE = 2.f;
    static constexpr float PITCH_BEND_SEMITONE_STEP = 0.5f / SEMITONE_RANGE;

    float pitchLevelSteps = pitchLevel / static_cast<float>(mpe::PITCH_LEVEL_STEP);
    float offset = pitchLevelSteps * PITCH_BEND_SEMITONE_STEP;

    return std::clamp(0.5f + offset, 0.f, 1.f);
}
