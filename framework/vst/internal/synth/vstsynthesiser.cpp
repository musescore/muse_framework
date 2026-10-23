/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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
#include "vstsynthesiser.h"

#include <string>

#include "mpe/articulationstringutils.h"

#include "pluginterfaces/vst/ivstnoteexpression.h"
#include "pluginterfaces/vst/ivstchannelcontextinfo.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

#include "log.h"

using namespace muse;
using namespace muse::vst;
using namespace muse::audio::synth;
using namespace muse::audio;
using namespace muse::audioplugins;

static const std::set<Steinberg::Vst::CtrlNumber> SUPPORTED_CONTROLLERS = {
    Steinberg::Vst::kCtrlVolume,
    Steinberg::Vst::kCtrlExpression,
    Steinberg::Vst::kCtrlSustainOnOff,
    Steinberg::Vst::kCtrlSustenutoOnOff,
    Steinberg::Vst::kPitchBend,
};

// Query the plugin's IKeyswitchController and build a keyswitch profile. Each keyswitch is advertised
// with the exact mpe::ArticulationType name, so titles convert straight to the type via MuseScore's
// own table; the host does no fuzzy matching. Only keyswitchMin on bus 0 / channel 0 is read: min
// alone selects the articulation.
static std::optional<VstKeyswitchProfile> queryKeyswitchProfile(const PluginControllerPtr& controller)
{
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    if (!controller) {
        return std::nullopt;
    }

    FUnknownPtr<IKeyswitchController> keyswitchCtrl(controller);
    if (!keyswitchCtrl) {
        return std::nullopt; // the plugin does not support keyswitches
    }

    const int32 busIndex = 0;
    const int16 channel = 0;
    const int32 count = keyswitchCtrl->getKeyswitchCount(busIndex, channel);
    if (count <= 0) {
        return std::nullopt;
    }

    VstKeyswitchProfile profile;

    for (int32 i = 0; i < count; ++i) {
        KeyswitchInfo info;
        if (keyswitchCtrl->getKeyswitchInfo(busIndex, channel, i, info) != kResultTrue) {
            continue;
        }

        // The title is the canonical articulation name; unknown titles map to Undefined and are skipped.
        const std::string title = VST3::StringConvert::convert(info.title);
        const mpe::ArticulationType type = mpe::articulationTypeFromString(QString::fromStdString(title));
        if (type != mpe::ArticulationType::Undefined) {
            profile.keyswitches[type] = info.keyswitchMin;
        }
    }

    if (profile.keyswitches.empty()) {
        return std::nullopt;
    }

    return profile;
}

VstSynthesiser::VstSynthesiser(const TrackId trackId, const muse::audio::AudioInputParams& params)
    : AbstractSynthesizer(params),
    m_vstAudioClient(std::make_unique<VstAudioClient>()),
    m_trackId(trackId)
{
}

VstSynthesiser::~VstSynthesiser()
{
    instancesRegister()->unregisterInstrPlugin(m_params.resourceMeta.id, m_trackId);
}

void VstSynthesiser::init(const OutputSpec& spec)
{
    IF_ASSERT_FAILED(spec.isValid()) {
        return;
    }

    m_outputSpec = spec;

    m_pluginPtr = instancesRegister()->makeAndRegisterInstrPlugin(m_params.resourceMeta.id, m_trackId);

    m_vstAudioClient->init(PluginType::Instrument, m_pluginPtr);

    auto onPluginLoaded = [this]() {
        m_pluginPtr->updatePluginConfig(m_params.configuration);
        m_vstAudioClient->setOutputSpec(m_outputSpec);
        m_vstAudioClient->loadSupportedParams();

        const std::optional<VstKeyswitchProfile> keyswitchProfile = queryKeyswitchProfile(m_pluginPtr->controller());
        m_sequencer.init(m_vstAudioClient->paramsMapping(SUPPORTED_CONTROLLERS), m_useDynamicEvents, keyswitchProfile);
        m_inited = true;
        sendChannelContext();
    };

    if (m_pluginPtr->isLoaded()) {
        onPluginLoaded();
    } else {
        m_pluginPtr->loadingCompleted().onNotify(this, onPluginLoaded);
    }

    m_pluginPtr->pluginSettingsChanged().onReceive(this, [this](const muse::audio::AudioUnitConfig& newConfig) {
        if (m_params.configuration == newConfig) {
            return;
        }

        m_params.configuration = newConfig;
        m_paramsChanges.send(m_params);
    });

    m_sequencer.setOnOffStreamFlushed([this]() {
        m_vstAudioClient->flushSound();
    });
}

void VstSynthesiser::toggleVolumeGain(const bool isActive)
{
    static constexpr muse::audio::gain_t NON_ACTIVE_GAIN = 0.5f;

    if (isActive) {
        m_vstAudioClient->setVolumeGain(m_sequencer.currentGain());
    } else {
        m_vstAudioClient->setVolumeGain(NON_ACTIVE_GAIN);
    }
}

bool VstSynthesiser::isValid() const
{
    if (!m_pluginPtr) {
        return false;
    }

    return m_pluginPtr->isLoaded();
}

muse::audio::AudioSourceType VstSynthesiser::type() const
{
    return m_params.type();
}

std::string VstSynthesiser::name() const
{
    if (!m_pluginPtr) {
        return std::string();
    }

    return m_pluginPtr->name();
}

void VstSynthesiser::setHostTrackName(const std::string& name)
{
    m_hostTrackName = name;
    if (m_inited) {
        sendChannelContext();
    }
}

void VstSynthesiser::sendChannelContext()
{
    if (m_hostTrackName.empty() || !m_pluginPtr) {
        return;
    }

    PluginControllerPtr controller = m_pluginPtr->controller();
    if (!controller) {
        return;
    }

    Steinberg::FUnknownPtr<Steinberg::Vst::ChannelContext::IInfoListener> infoListener(controller);
    if (!infoListener) {
        return; // the plugin does not use channel context
    }

    Steinberg::IPtr<Steinberg::Vst::IAttributeList> list = Steinberg::Vst::HostAttributeList::make();
    Steinberg::Vst::String128 name128 = {};
    Steinberg::Vst::StringConvert::convert(m_hostTrackName, name128);
    list->setString(Steinberg::Vst::ChannelContext::kChannelNameKey, name128);
    infoListener->setChannelContextInfos(list);
}

void VstSynthesiser::flushSound()
{
    m_sequencer.flushOffstream();
    m_vstAudioClient->flushSound();
}

void VstSynthesiser::setupSound(const mpe::PlaybackSetupData& setupData)
{
    m_useDynamicEvents = setupData.supportsSingleNoteDynamics;
}

void VstSynthesiser::setupEvents(const mpe::PlaybackData& playbackData)
{
    m_sequencer.load(playbackData);
}

const mpe::PlaybackData& VstSynthesiser::playbackData() const
{
    return m_sequencer.playbackData();
}

void VstSynthesiser::setMode(const muse::audio::ProcessMode mode)
{
    if (m_mode == mode) {
        return;
    }

    AbstractSynthesizer::setMode(mode);

    bool isActive = isModePlaying(mode);
    m_sequencer.setActive(isActive);
    toggleVolumeGain(isActive);
    m_vstAudioClient->setIsPlaying(isActive);
    m_vstAudioClient->setIsActive(isActive);

    if (mode == ProcessMode::PlayingOffline) {
        m_vstAudioClient->setProcessMode(VstProcessMode::kOffline);
    } else {
        m_vstAudioClient->setProcessMode(VstProcessMode::kRealtime);
    }
}

muse::audio::TimePosition VstSynthesiser::playbackPosition() const
{
    return m_currentPosition;
}

void VstSynthesiser::setPlaybackPosition(const muse::audio::TimePosition& position)
{
    IF_ASSERT_FAILED(position.isValid()) {
        return;
    }

    //! NOTE Don't trust that msecs_t is used everywhere here,
    // in fact, usecs_t (microseconds) is stored there.
    const usecs_t usecs = muse::secs_to_usecs(position.time());
    m_sequencer.setPlaybackPosition(msecs_t(usecs.raw()));

    m_currentPosition = position;

    if (m_sequencer.isActive()) {
        m_vstAudioClient->setVolumeGain(m_sequencer.currentGain());
    }
}

void VstSynthesiser::setOutputSpec(const audio::OutputSpec& spec)
{
    m_outputSpec = spec;
    if (m_inited) {
        m_vstAudioClient->setOutputSpec(spec);
    }
}

samples_t VstSynthesiser::process(float* buffer, samples_t samplesPerChannel)
{
    if (!buffer) {
        return 0;
    }

    const msecs_t nextMsecs = samplesToMsecs(samplesPerChannel, m_outputSpec.sampleRate);
    const VstSequencer::EventSequenceMap sequences = m_sequencer.movePlaybackForward(nextMsecs);
    const bool active = m_sequencer.isActive();

    samples_t sampleOffset = 0;
    samples_t processedSamples = 0;

    for (auto it = sequences.cbegin(); it != sequences.cend(); ++it) {
        samples_t durationInSamples = samplesPerChannel - sampleOffset;

        auto nextIt = std::next(it);
        if (nextIt != sequences.cend()) {
            msecs_t duration = nextIt->first - it->first;
            durationInSamples = microSecsToSamples(duration, m_outputSpec.sampleRate);
        }

        IF_ASSERT_FAILED(sampleOffset + durationInSamples <= samplesPerChannel) {
            break;
        }

        processedSamples += processSequence(it->second, durationInSamples, buffer + sampleOffset * m_outputSpec.audioChannelCount);
        sampleOffset += durationInSamples;

        if (active) {
            m_currentPosition.forward(durationInSamples);
        }
    }

    return processedSamples;
}

samples_t VstSynthesiser::processSequence(const VstSequencer::EventSequence& sequence, const samples_t samples, float* buffer)
{
    for (const VstSequencer::EventType& event : sequence) {
        if (std::holds_alternative<VstEvent>(event)) {
            m_vstAudioClient->handleEvent(std::get<VstEvent>(event));
        } else if (std::holds_alternative<ParamChangeEvent>(event)) {
            m_vstAudioClient->handleParamChange(std::get<ParamChangeEvent>(event));
        } else {
            muse::audio::gain_t newGain = std::get<muse::audio::gain_t>(event);
            m_vstAudioClient->setVolumeGain(newGain);
        }
    }

    if (samples == 0) {
        return 0;
    }

    return m_vstAudioClient->process(buffer, samples, m_currentPosition.samples());
}
