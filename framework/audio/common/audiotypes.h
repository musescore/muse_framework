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

#include <map>
#include <variant>
#include <set>
#include <string>
#include <cmath>
#include <sstream>
#include <tuple>
#include <vector>

#include "global/types/number.h"
#include "global/types/secs.h"
#include "global/types/ratio.h"
#include "global/types/string.h"
#include "global/async/channel.h"
#include "global/io/iodevice.h"

#include "mpe/events.h"

#include "automatablevalue.h"

#include "log.h"

// Forward-declared audioplugins aliases; audio must not depend on that module.
namespace muse::audioplugins {
using PluginResourceId = std::string;
using PluginResourceIdList = std::vector<PluginResourceId>;
using PluginType = std::string;
}

namespace muse::audio {
using secs_t = muse::secs_t;
using msecs_t = muse::msecs_t;
using usecs_t = muse::usecs_t;

using samples_t = uint64_t;
using sample_rate_t = uint64_t;
using audioch_t = uint8_t;
using volume_db_t = db_t;
using volume_dbfs_t = db_t;
using gain_t = float;
using balance_t = number_t<float>;

using AudioCtxId = uint16_t;

using TrackId = int32_t;
using TrackIdList = std::vector<TrackId>;
using TrackName = std::string;

using aux_channel_idx_t = uint8_t;

using PlaybackData = std::variant<mpe::PlaybackData, io::IODevice*>;
using PlaybackSetupData = mpe::PlaybackSetupData;

static constexpr TrackId INVALID_TRACK_ID = -1;
static constexpr TrackId MASTER_TRACK_ID = 0;

static constexpr char DEFAULT_DEVICE_ID[] = "default";

#ifdef Q_OS_WIN
static constexpr samples_t MINIMUM_BUFFER_SIZE = 256;
#else
static constexpr samples_t MINIMUM_BUFFER_SIZE = 128;
#endif

static constexpr samples_t MAXIMUM_BUFFER_SIZE = 4096;

struct OutputSpec {
    sample_rate_t sampleRate = 0;
    samples_t samplesPerChannel = 0;
    audioch_t audioChannelCount = 0;

    inline bool isValid() const { return sampleRate > 0 && samplesPerChannel > 0 && audioChannelCount > 0; }

    inline bool operator==(const OutputSpec& other) const
    {
        return sampleRate == other.sampleRate
               && samplesPerChannel == other.samplesPerChannel
               && audioChannelCount == other.audioChannelCount;
    }

    inline bool operator!=(const OutputSpec& other) const { return !this->operator==(other); }

    std::string dump() const
    {
        std::stringstream ss;
        ss << "sampleRate: " << sampleRate
           << ", samplesPerChannel: " << samplesPerChannel
           << ", audioChannelCount: " << audioChannelCount;
        return ss.str();
    }
};

enum class SoundTrackType {
    Undefined = -1,
    MP3,
    OGG,
    FLAC,
    WAV,
    AAC
};

enum class AudioSampleFormat {
    Undefined = 0,
    Int16,
    Int24,
    Float32
};

struct SoundTrackFormat {
    SoundTrackType type = SoundTrackType::Undefined;
    OutputSpec outputSpec;
    AudioSampleFormat sampleFormat = AudioSampleFormat::Undefined;
    int bitRate = 0;
    secs_t leadingSilenceDuration = 0.0;
    secs_t trailingSilenceDuration = 0.0;

    bool operator==(const SoundTrackFormat& other) const
    {
        return type == other.type
               && outputSpec == other.outputSpec
               && sampleFormat == other.sampleFormat
               && bitRate == other.bitRate
               && leadingSilenceDuration == other.leadingSilenceDuration
               && trailingSilenceDuration == other.trailingSilenceDuration;
    }

    bool isValid() const
    {
        if (!outputSpec.isValid()) {
            return false;
        }

        switch (type) {
        case SoundTrackType::WAV:
        case SoundTrackType::FLAC:
            // For lossless/uncompressed, sample format must be defined
            return sampleFormat != AudioSampleFormat::Undefined;

        case SoundTrackType::MP3:
        case SoundTrackType::OGG:
        case SoundTrackType::AAC:
            // For lossy, bitrate must be positive
            return bitRate > 0;

        default:
            return false;
        }
    }
};

struct AudioEngineConfig {
    bool autoProcessOnlineSoundsInBackground = false;
    bool isLazyProcessingOfOnlineSoundsEnabled = false;
    bool useSoundFontLowPassFilter = false;
};

using AudioSourceName = std::string;
using AudioUnitConfig = std::map<std::string, std::string>;

// Audio-pipeline resource id; distinct role from audioplugins::PluginResourceId (both std::string).
using AudioResourceId = std::string;
using AudioResourceIdList = std::vector<AudioResourceId>;

inline AudioResourceId toAudioResourceId(const muse::audioplugins::PluginResourceId& id)
{
    return id;
}

inline AudioResourceIdList toAudioResourceIdList(const muse::audioplugins::PluginResourceIdList& ids)
{
    return AudioResourceIdList(ids.begin(), ids.end());
}

// audioplugins::PluginMeta is the cache-domain twin of AudioResourceMeta;
// kept separate so neither module depends on the other.
using AudioResourceVendor = std::string;
using AudioResourceAttributes = std::map<String, String>;

struct AudioResourceMeta {
    AudioResourceId id;
    AudioResourceVendor vendor;
    AudioResourceAttributes attributes;
    std::string type;  // wire-string format identifier; see AudioResourceType enum below

    const String& attributeVal(const String& key) const
    {
        auto search = attributes.find(key);
        if (search != attributes.cend()) {
            return search->second;
        }
        static const String empty;
        return empty;
    }

    bool isValid() const
    {
        return !id.empty() && !vendor.empty() && !type.empty();
    }

    bool operator==(const AudioResourceMeta& other) const
    {
        return id == other.id && vendor == other.vendor && type == other.type && attributes == other.attributes;
    }

    bool operator!=(const AudioResourceMeta& other) const { return !(*this == other); }

    bool operator<(const AudioResourceMeta& other) const
    {
        return std::tie(id, vendor, type, attributes)
               < std::tie(other.id, other.vendor, other.type, other.attributes);
    }
};

using AudioResourceMetaList = std::vector<AudioResourceMeta>;
using AudioResourceMetaSet = std::set<AudioResourceMeta>;

inline bool boolAttribute(const AudioResourceMeta& meta, const String& key, bool fallback = false)
{
    const String& v = meta.attributeVal(key);
    if (v.empty()) {
        return fallback;
    }
    return v == u"true" || v == u"1";
}

inline int intAttribute(const AudioResourceMeta& meta, const String& key, int fallback = 0)
{
    const String& v = meta.attributeVal(key);
    if (v.empty()) {
        return fallback;
    }
    bool ok = true;
    int n = v.toInt(&ok);
    return ok ? n : fallback;
}

// Wire strings persisted in the plugin cache; constexpr for the RESOURCE_TYPE_NAMES map below.
inline constexpr std::string_view FLUID_SOUNDFONT_TYPE_NAME = "FluidSoundfont";
inline constexpr std::string_view NATIVE_EFFECT_TYPE_NAME = "NativeEffect";

// Engine-internal routing enum, distinct from the opaque audioplugins::PluginType string.
enum class AudioResourceType {
    Undefined = -1,
    FluidSoundfont,
    VstPlugin,
    NativeEffect,
    MuseSamplerSoundPack,
};

static const std::map<AudioResourceType, std::string> RESOURCE_TYPE_NAMES = {
    { AudioResourceType::FluidSoundfont,        std::string(FLUID_SOUNDFONT_TYPE_NAME) },
    { AudioResourceType::VstPlugin,             "VstPlugin" },
    { AudioResourceType::NativeEffect,          std::string(NATIVE_EFFECT_TYPE_NAME) },
    { AudioResourceType::MuseSamplerSoundPack,  "MuseSamplerSoundPack" },
};

inline const std::string& resourceTypeName(AudioResourceType type)
{
    auto it = RESOURCE_TYPE_NAMES.find(type);
    if (it != RESOURCE_TYPE_NAMES.end()) {
        return it->second;
    }
    static const std::string empty;
    return empty;
}

inline AudioResourceType resourceTypeFromString(const std::string& name)
{
    for (const auto& kv : RESOURCE_TYPE_NAMES) {
        if (kv.second == name) {
            return kv.first;
        }
    }
    return AudioResourceType::Undefined;
}

inline bool isResourceType(const AudioResourceMeta& meta, AudioResourceType type)
{
    return resourceTypeFromString(meta.type) == type;
}

static const String PLAYBACK_SETUP_DATA_ATTRIBUTE(u"playbackSetupData");
static const String HAS_NATIVE_EDITOR_SUPPORT_ATTRIBUTE(u"hasNativeEditorSupport");

inline bool hasNativeEditorSupport(const AudioResourceMeta& meta)
{
    return boolAttribute(meta, HAS_NATIVE_EDITOR_SUPPORT_ATTRIBUTE);
}

static const std::map<AudioResourceType, QString> RESOURCE_TYPE_MAP = {
    { AudioResourceType::Undefined, "undefined" },
    { AudioResourceType::MuseSamplerSoundPack, "muse_sampler_sound_pack" },
    { AudioResourceType::FluidSoundfont, "fluid_soundfont" },
    { AudioResourceType::VstPlugin, "vst_plugin" },
    { AudioResourceType::NativeEffect, "muse_plugin" },
};

static const AudioResourceId MUSE_REVERB_ID("Muse Reverb");

enum class AudioFxType {
    Undefined = -1,
    VstFx,
    MuseFx,
};

enum class AudioFxCategory {
    Undefined = -1,
    FxEqualizer,
    FxAnalyzer,
    FxDelay,
    FxDistortion,
    FxDynamics,
    FxFilter,
    FxGenerator,
    FxMastering,
    FxModulation,
    FxPitchShift,
    FxRestoration,
    FxReverb,
    FxSurround,
    FxTools,
    FxOther,
};

inline const std::unordered_map<AudioFxCategory, String> AUDIO_FX_CATEGORY_TO_STRING_MAP {
    { AudioFxCategory::FxEqualizer, u"EQ" },
    { AudioFxCategory::FxAnalyzer, u"Analyzer" },
    { AudioFxCategory::FxDelay, u"Delay" },
    { AudioFxCategory::FxDistortion, u"Distortion" },
    { AudioFxCategory::FxDynamics, u"Dynamics" },
    { AudioFxCategory::FxFilter, u"Filter" },
    { AudioFxCategory::FxGenerator, u"Generator" },
    { AudioFxCategory::FxMastering, u"Mastering" },
    { AudioFxCategory::FxModulation, u"Modulation" },
    { AudioFxCategory::FxPitchShift, u"Pitch Shift" },
    { AudioFxCategory::FxRestoration, u"Restoration" },
    { AudioFxCategory::FxReverb, u"Reverb" },
    { AudioFxCategory::FxSurround, u"Surround" },
    { AudioFxCategory::FxTools, u"Tools" },
    { AudioFxCategory::FxOther, u"Fx" },
};

using AudioFxCategories = std::set<AudioFxCategory>;
using AudioFxChainOrder = int8_t;

struct AudioFxParams {
    AudioFxCategories categories;
    AudioUnitConfig configuration;
    AudioResourceMeta resourceMeta;
    AudioFxChainOrder chainOrder = -1;
    bool active = false;

    AudioFxType type() const
    {
        switch (resourceTypeFromString(resourceMeta.type)) {
        case AudioResourceType::VstPlugin: return AudioFxType::VstFx;
        case AudioResourceType::NativeEffect: return AudioFxType::MuseFx;
        case AudioResourceType::FluidSoundfont:
        case AudioResourceType::MuseSamplerSoundPack:
        case AudioResourceType::Undefined: break;
        }

        return AudioFxType::Undefined;
    }

    bool operator ==(const AudioFxParams& other) const
    {
        return active == other.active
               && chainOrder == other.chainOrder
               && resourceMeta == other.resourceMeta
               && categories == other.categories
               && configuration == other.configuration;
    }

    bool operator !=(const AudioFxParams& other) const
    {
        return !(*this == other);
    }

    bool operator<(const AudioFxParams& other) const
    {
        return resourceMeta < other.resourceMeta
               && chainOrder < other.chainOrder;
    }

    bool isValid() const
    {
        return resourceMeta.isValid();
    }
};

using AudioFxChain = std::map<AudioFxChainOrder, AudioFxParams>;

struct AuxSendParams {
    gain_t signalAmount = 0.f; // [0; 1]
    bool active = false;

    bool operator ==(const AuxSendParams& other) const
    {
        return RealIsEqual(signalAmount, other.signalAmount) && active == other.active;
    }
};

using AuxSendsParams = std::vector<AuxSendParams>;

static constexpr volume_db_t VOLUME_DB_MIN = volume_db_t::make(-60.f);
static constexpr volume_db_t VOLUME_DB_MAX = volume_db_t::make(12.f);
static constexpr balance_t BALANCE_MIN = balance_t::make(-1.f);
static constexpr balance_t BALANCE_MAX = balance_t::make(1.f);

struct ControlParams {
    AutomatableValue<volume_db_t> volume;
    AutomatableValue<balance_t> balance;
    bool muted = false;

    bool operator ==(const ControlParams& other) const
    {
        return muted == other.muted
               && volume == other.volume
               && balance == other.balance;
    }

    bool operator !=(const ControlParams& other) const
    {
        return !this->operator==(other);
    }
};

enum class AudioSourceType {
    Undefined = -1,
    Fluid,
    Vsti,
    MuseSampler
};

inline AudioSourceType sourceTypeFromResourceType(const muse::audioplugins::PluginType& metaType)
{
    switch (resourceTypeFromString(metaType)) {
    case AudioResourceType::FluidSoundfont: return AudioSourceType::Fluid;
    case AudioResourceType::VstPlugin: return AudioSourceType::Vsti;
    case AudioResourceType::MuseSamplerSoundPack: return AudioSourceType::MuseSampler;
    case AudioResourceType::NativeEffect:
    case AudioResourceType::Undefined: break;
    }

    return AudioSourceType::Undefined;
}

struct AudioSourceParams {
    AudioResourceMeta resourceMeta;
    AudioUnitConfig configuration;

    AudioSourceType type() const
    {
        return sourceTypeFromResourceType(resourceMeta.type);
    }

    bool isValid() const
    {
        return type() != AudioSourceType::Undefined
               && resourceMeta.isValid();
    }

    bool operator ==(const AudioSourceParams& other) const
    {
        return type() == other.type()
               && resourceMeta == other.resourceMeta
               && configuration == other.configuration;
    }

    bool operator !=(const AudioSourceParams& other) const { return !this->operator==(other); }
};

using AudioInputParams = AudioSourceParams;

struct TrackParams {
    AudioInputParams source;
    AudioFxChain fxChain;
    AuxSendsParams auxSends;
    ControlParams control;
};

struct AudioSignalVal {
    volume_dbfs_t pressure = 0.f;

    inline bool operator ==(const AudioSignalVal& other) const
    {
        return pressure == other.pressure;
    }
};

using AudioSignalValuesMap = std::map<audioch_t, AudioSignalVal>;
using AudioSignalChanges = async::Channel<AudioSignalValuesMap>;

struct AutomatedControlParams {
    volume_db_t volume = 0.f;
    balance_t balance = 0.f;
};

using AutomatedControlParamsChanges = async::Channel<AutomatedControlParams>;

enum class PlaybackStatus {
    Stopped = 0,
    Paused,
    Running
};

using AudioDeviceID = std::string;
struct AudioDevice {
    AudioDeviceID id;
    std::string name;

    bool operator==(const AudioDevice& other) const
    {
        return id == other.id;
    }
};

using AudioDeviceList = std::vector<AudioDevice>;

using SoundPresetAttributes = std::map<String, String>;
static const String PLAYING_TECHNIQUES_ATTRIBUTE(u"playing_techniques");

struct SoundPreset
{
    String code;
    String name;
    bool isDefault = false;
    SoundPresetAttributes attributes;

    bool operator==(const SoundPreset& other) const
    {
        return code == other.code && name == other.name && isDefault == other.isDefault && attributes == other.attributes;
    }

    bool isValid() const
    {
        return !code.empty();
    }
};

using SoundPresetList = std::vector<SoundPreset>;

enum class ProcessMode {
    Undefined = 0,
    Idle,
    Playing,
    PlayingOffline
};

inline std::string to_string(ProcessMode mode)
{
    switch (mode) {
    case ProcessMode::Undefined: return "Undefined";
    case ProcessMode::Idle: return "Idle";
    case ProcessMode::Playing: return "Playing";
    case ProcessMode::PlayingOffline: return "PlayingOffline";
    }
    return "Unknown";
}

inline bool isModePlaying(ProcessMode mode)
{
    return mode == ProcessMode::Playing || mode == ProcessMode::PlayingOffline;
}

struct InputProcessingProgress {
    struct ChunkInfo {
        secs_t start = 0.0;
        secs_t end = 0.0;

        bool operator==(const ChunkInfo& c) const
        {
            return start == c.start && end == c.end;
        }
    };

    using ChunkInfoList = std::vector<ChunkInfo>;

    struct ProgressInfo {
        int64_t current = 0;
        int64_t total = 0;
    };

    enum Status : unsigned int {
        Undefined = 0,
        Started,
        Processing,
        Finished
    };

    struct StatusInfo {
        Status status = Status::Undefined;
        int errorCode = 0;
        std::string errorText;
        using StatusData = std::map<std::string, std::string>;
        StatusData data;
    };

    void start()
    {
        isStarted = true;
        processedChannel.send({ Status::Started, 0, {}, {} }, {}, {});
    }

    void process(const ChunkInfoList& chunks, int64_t current, int64_t total)
    {
        processedChannel.send({ Status::Processing, 0, {}, {} }, chunks, { current, total });
    }

    void finish(int errcode, const std::string& err = {}, const StatusInfo::StatusData& data = {})
    {
        isStarted = false;
        processedChannel.send({ Status::Finished, errcode, err, data }, {}, {});
    }

    bool isStarted = false;
    async::Channel<StatusInfo, ChunkInfoList, ProgressInfo> processedChannel;
};

enum SaveSoundTrackStage {
    Unknown = 0,
    ProcessingOnlineSounds,
    WritingSoundTrack,
};

using SaveSoundTrackProgress = async::Channel<int64_t /*current*/, int64_t /*total*/, SaveSoundTrackStage>;

struct TransportEvent {
    enum class Type : unsigned char {
        Unknown = 0,
        Play,
        Pause,
        Stop,
        Seek,
    };

    struct SeekData {
        secs_t position = 0.;
    };

    static TransportEvent play() { return { Type::Play, {} }; }
    static TransportEvent pause() { return { Type::Pause, {} }; }
    static TransportEvent stop() { return { Type::Stop, {} }; }
    static TransportEvent seek(secs_t pos) { return { Type::Seek, SeekData { pos } }; }

    Type type = Type::Unknown;
    std::variant<std::monostate, SeekData> data;
};
using TransportEvents = std::vector<TransportEvent>;
}

inline muse::logger::Stream& operator<<(muse::logger::Stream& s, const muse::audio::OutputSpec& spec)
{
    s << "sampleRate: " << spec.sampleRate
      << ", channels: " << spec.audioChannelCount
      << ", samplesPerChannel: " << spec.samplesPerChannel;
    return s;
}
