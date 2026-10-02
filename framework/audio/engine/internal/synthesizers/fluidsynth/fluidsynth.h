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

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "global/io/path.h"

#include "global/modularity/ioc.h"
#include "midi/imidioutport.h"

#include "../abstractsynthesizer.h"
#include "fluidsequencer.h"

#include "log.h"

namespace muse::audio::synth {
struct Fluid;
class FluidSynth : public AbstractSynthesizer
{
    GlobalInject<midi::IMidiOutPort> midiOutPort;

public:
    FluidSynth(const audio::AudioSourceParams& params);

    Ret init(const OutputSpec& spec);

    Ret addSoundFonts(const std::vector<io::path_t>& sfonts);
    void setPreset(const std::optional<midi::Program>& preset);

    std::string name() const override;
    AudioSourceType type() const override;

    void setupSound(const mpe::PlaybackSetupData& setupData) override;
    void setupEvents(const mpe::PlaybackData& playbackData) override;
    const mpe::PlaybackData& playbackData() const override;

    void flushSound() override; // all channels

    TimePosition playbackPosition() const override;
    void setPlaybackPosition(const TimePosition& position) override;

    samples_t process(float* buffer, samples_t samplesPerChannel) override;

    void setMode(const ProcessMode mode) override;
    void setOutputSpec(const OutputSpec& spec) override;

    bool isValid() const override;

private:
    struct KeyTuning {
        static constexpr int MIDI_KEY_COUNT = 128;

        std::array<int, MIDI_KEY_COUNT> keys;
        std::array<double, MIDI_KEY_COUNT> pitches;
        std::array<int, MIDI_KEY_COUNT> keyIndex; // index into keys/pitches for a given key, or -1 if not present
        int count = 0;

        KeyTuning()
        {
            keyIndex.fill(-1);
        }

        void add(int key, double tuning)
        {
            IF_ASSERT_FAILED(key >= 0 && key < MIDI_KEY_COUNT) {
                return;
            }

            const double pitch = (key * 100.0) + tuning;

            const int index = keyIndex[key];
            if (index >= 0) {
                pitches[index] = pitch;
                return;
            }

            keyIndex[key] = count;
            keys[count] = key;
            pitches[count] = pitch;
            ++count;
        }

        int size() const
        {
            return count;
        }

        void reset()
        {
            for (int i = 0; i < count; ++i) {
                keyIndex[keys[i]] = -1;
            }
            count = 0;
        }

        bool isEmpty() const
        {
            return count == 0;
        }
    };

    void createFluidInstance();

    void doFlushSound();

    bool processSequence(const FluidSequencer::EventSequence& sequence, const samples_t samples, float* buffer);
    bool handleEvent(const midi::Event& event);

    void updateExpressionLevels();

    int setControllerValue(int channel, int ctrl, int value);
    int setPitchBend(int channel, int pitchBend);

    OutputSpec m_outputSpec;

    std::shared_ptr<Fluid> m_fluid;
    std::shared_ptr<midi::IMidiOutPort> m_midiOutPort;

    FluidSequencer m_sequencer;
    std::set<io::path_t> m_sfontPaths;
    std::optional<midi::Program> m_preset;

    KeyTuning m_tuning;

    bool m_flushSoundRequested = false;
};

using FluidSynthPtr = std::shared_ptr<FluidSynth>;
}
