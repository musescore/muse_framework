/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include "osxdirectaudiodriver.h"

#include <CoreAudio/AudioHardware.h>
#include <CoreAudio/AudioHardwareBase.h>
#include <CoreAudioTypes/CoreAudioBaseTypes.h>
#include <MacTypes.h>
#include <QtCore/qsemaphore.h>
#include <Security/cssmconfig.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <mutex>

#include <AudioToolbox/AudioToolbox.h>
#include <optional>
#include <string_view>

#include "common/audiotypes.h"
#include "common/audioworkgroup.h"
#include "osxidlesleeppolicy.h"
#include "translation.h"
#include "log.h"

typedef AudioDeviceID OSXAudioDeviceID;

using namespace muse;
using namespace muse::audio;

struct ChannelBufferDetails {
    int streamNumber;
    unsigned int hardwareChannelNumber;
    unsigned int dataOffsetSamples;
    unsigned int dataStrideSamples;
};

struct OSXDirectAudioDriver::Data {
    Spec format;
    AudioDeviceIOProcID procId{};
    bool canBeDirectlyMapped = false;

    OSXAudioDeviceID deviceId{};

    std::atomic<bool> stopPending{ false };
    std::atomic<bool> stopped{ false };

    std::vector<ChannelBufferDetails> channelBufferOutputDetails;
    std::vector<float> outBuffer;

    Data& operator=(const Data& other)
    {
        format = other.format;
        procId = other.procId;
        canBeDirectlyMapped = other.canBeDirectlyMapped;
        deviceId = other.deviceId;
        stopPending = other.stopPending.load();
        stopped = other.stopped.load();
        channelBufferOutputDetails = other.channelBufferOutputDetails;
        outBuffer = other.outBuffer;
        return *this;
    }

    void clear()
    {
        *this = Data();
    }
};
namespace muse::audio {
AudioWorkGroup makeAudioWorkgroup(void* opaqueHandle);
AudioWorkGroup OSXDirectAudioDriver::getAudioWorkGroup() const
{
    return m_audioWorkGroup;
}

async::Notification OSXDirectAudioDriver::currentWorkgroupChanged() const
{
    return m_currentWorkgroupChanged;
}
} // namespace muse::audio

OSXDirectAudioDriver::OSXDirectAudioDriver()
    : m_data(std::make_unique<Data>())
{
    initDeviceMapListener();
    updateDeviceMap();
}

OSXDirectAudioDriver::~OSXDirectAudioDriver()
{
    removeDeviceMapListener();
    doClose();
}

void OSXDirectAudioDriver::init()
{
}

std::string OSXDirectAudioDriver::name() const
{
    return "OSX";
}

muse::audio::AudioDeviceID OSXDirectAudioDriver::defaultDevice() const
{
    return DEFAULT_DEVICE_ID;
}

static uint32_t outputBufferSamplesPerChannel(const AudioBuffer& buffer)
{
    if (buffer.mNumberChannels == 0) {
        return 0;
    }

    return buffer.mDataByteSize / (buffer.mNumberChannels * sizeof(float));
}

static uint32_t callbackSamplesPerChannel(const AudioBufferList* outputData,
                                          const std::vector<ChannelBufferDetails>& details)
{
    std::optional<uint32_t> result;

    for (const ChannelBufferDetails& detail : details) {
        if (detail.streamNumber < 0 || static_cast<UInt32>(detail.streamNumber) >= outputData->mNumberBuffers) {
            continue;
        }

        const AudioBuffer& outputBuffer = outputData->mBuffers[detail.streamNumber];
        const uint32_t frames = outputBufferSamplesPerChannel(outputBuffer);
        if (frames == 0) {
            continue;
        }

        result = result.has_value() ? std::min(result.value(), frames) : frames;
    }

    return result.value_or(0);
}

static std::optional<uint32_t> getSameRequestedSampleCount(const AudioBufferList* outOutputData,
                                                           const std::vector<ChannelBufferDetails>& details)
{
    std::optional<uint32_t> sampleCount;

    for (const ChannelBufferDetails& detail : details) {
        if (detail.streamNumber < 0 || static_cast<UInt32>(detail.streamNumber) >= outOutputData->mNumberBuffers) {
            continue;
        }

        const AudioBuffer& outputBuffer = outOutputData->mBuffers[detail.streamNumber];
        const uint32_t frames = outputBufferSamplesPerChannel(outputBuffer);
        if (frames == 0) {
            continue;
        }

        if (!sampleCount.has_value()) {
            sampleCount = frames;
        } else if (sampleCount.value() != frames) {
            return std::nullopt;
        }
    }

    return sampleCount;
}

//TODO: make use of timing parameters
static int coreAudioIOProc(AudioObjectID /* inDevice*/,
                           const AudioTimeStamp* /* inNow */,
                           const AudioBufferList* /* inInputData */,
                           const AudioTimeStamp* /*  inInputTime */,
                           AudioBufferList* outOutputData,
                           const AudioTimeStamp* /* inOutputTime */,
                           void* __nullable inClientData)
{
    auto* data = reinterpret_cast<OSXDirectAudioDriver::Data*>(inClientData);
    if (data->stopPending) {
        AudioDeviceStop(data->deviceId, data->procId);
        data->stopPending = false;
        data->stopped = true;
        return noErr;
    }
    if (data->channelBufferOutputDetails.empty()) {
        return noErr;
    }

    uint32_t samplesPerChannel = callbackSamplesPerChannel(outOutputData, data->channelBufferOutputDetails);
    if (samplesPerChannel == 0) {
        return noErr;
    }

    const uint32_t callbackDataStride = data->format.output.audioChannelCount;
    int dataSize = static_cast<int>(samplesPerChannel * callbackDataStride * sizeof(float));
    if (data->canBeDirectlyMapped && getSameRequestedSampleCount(outOutputData,
                                                                 data->channelBufferOutputDetails).value_or(0) == samplesPerChannel) {
        data->format.callback(
            (uint8_t*)outOutputData
            ->mBuffers[data->channelBufferOutputDetails[0].streamNumber]
            .mData,
            dataSize);
        return 0;
    }

    if (data->outBuffer.size() < static_cast<size_t>(samplesPerChannel) * callbackDataStride) {
        samplesPerChannel = static_cast<uint32_t>(data->outBuffer.size() / callbackDataStride);
        dataSize = static_cast<int>(samplesPerChannel * callbackDataStride * sizeof(float));
    }

    data->format.callback((uint8_t*)data->outBuffer.data(), dataSize);

    for (int ch = 0; ch < (int)data->channelBufferOutputDetails.size(); ++ch) {
        const auto& details = data->channelBufferOutputDetails[ch];
        auto& outputBuffer = outOutputData->mBuffers[details.streamNumber];
        int stride = details.dataStrideSamples;
        if (stride == 0) {
            continue;
        }

        const uint64_t requiredSamples = details.dataOffsetSamples
                                         + static_cast<uint64_t>(samplesPerChannel - 1) * stride
                                         + 1;
        if (outputBuffer.mDataByteSize < (requiredSamples * sizeof(float))) {
            // this is very unexpected. Should always be the same as samplesPerChannel
            continue;
        }

        float* src = data->outBuffer.data() + ch;
        float* dest = static_cast<float*>(outputBuffer.mData)
                      + details.dataOffsetSamples;
        for (int j = samplesPerChannel; --j >= 0;) {
            *dest = *src;
            dest += stride;
            src += callbackDataStride;
        }
    }
    return 0;
}

std::optional<std::array<uint32_t, 2> > getPreferredStereoHardwareChannels(OSXAudioDeviceID deviceId)
{
    AudioObjectPropertyAddress addr{
        kAudioDevicePropertyPreferredChannelsForStereo,
        kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMaster };
    UInt32 stereo[2] = {};
    UInt32 size = sizeof(stereo);

    if (AudioObjectGetPropertyData(deviceId, &addr, 0, nullptr, &size, stereo) != noErr) {
        return std::nullopt;
    }
    return std::array<uint32_t, 2> { stereo[0], stereo[1] };
}

bool filterAndReorderStereoHardwareChannels(const OSXAudioDeviceID& deviceId,
                                            std::vector<ChannelBufferDetails>& details)
{
    auto stereoChannels = getPreferredStereoHardwareChannels(deviceId);
    if (!stereoChannels.has_value()) {
        return false;
    }
    details.erase(
        std::remove_if(details.begin(), details.end(),
                       [&stereoChannels](const ChannelBufferDetails& details) {
        return details.hardwareChannelNumber
               != stereoChannels->at(0)
               && details.hardwareChannelNumber
               != stereoChannels->at(1);
    }),
        details.end());
    std::sort(details.begin(), details.end(),
              [&stereoChannels](const ChannelBufferDetails& a,
                                const ChannelBufferDetails& b) {
        auto aIndex = (a.hardwareChannelNumber
                       == stereoChannels->at(0))
                      ? 0
                      : 1;
        auto bIndex = (b.hardwareChannelNumber
                       == stereoChannels->at(0))
                      ? 0
                      : 1;
        return aIndex < bIndex;
    });
    return true;
}

std::vector<ChannelBufferDetails> getChannelBufferDetails(const OSXAudioDeviceID& deviceId,
                                                          const std::function<void(std::string, OSStatus)>& onError)
{
    std::vector<ChannelBufferDetails> result;

    UInt32 size{};
    AudioObjectPropertyAddress address = { kAudioDevicePropertyStreamConfiguration,
                                           kAudioDevicePropertyScopeOutput,
                                           kAudioObjectPropertyElementMaster };
    if (auto status = AudioObjectGetPropertyDataSize(deviceId, &address, 0, nullptr, &size); status != noErr) {
        onError("Failed to get device " + std::to_string(deviceId) + " stream configuration" + ", err: " + std::to_string(status), status);
        return result;
    }

    std::vector<uint8_t> buf(size);

    auto status = AudioObjectGetPropertyData(deviceId, &address, 0, nullptr,
                                             &size, buf.data());
    if (status != noErr) {
        onError("Failed to get device " + std::to_string(deviceId) + " stream configuration" + ", err: " + std::to_string(status), status);
        return result;
    }

    AudioBufferList* bufList = reinterpret_cast<AudioBufferList*>(buf.data());

    const int numStreams = static_cast<int>(bufList->mNumberBuffers);
    unsigned int hardwareChannelNumber = 1;

    for (int i = 0; i < numStreams; ++i) {
        auto& b = bufList->mBuffers[i];
        for (unsigned int j = 0; j < b.mNumberChannels; ++j) {
            ChannelBufferDetails details {
                .streamNumber = i,
                .hardwareChannelNumber = hardwareChannelNumber++,
                .dataOffsetSamples = j,
                .dataStrideSamples
                    =b.mNumberChannels };
            result.push_back(details);
        }
    }

    return result;
}

static bool isSupportedCallbackFormat(const AudioStreamBasicDescription& format)
{
    const bool isNativeEndian = (format.mFormatFlags & kAudioFormatFlagIsBigEndian) == kAudioFormatFlagsNativeEndian;
    const bool isNativeFloatPacked = (format.mFormatFlags & kAudioFormatFlagsNativeFloatPacked) == kAudioFormatFlagsNativeFloatPacked;

    return format.mFormatID == kAudioFormatLinearPCM
           && isNativeEndian
           && isNativeFloatPacked
           && format.mBitsPerChannel == sizeof(float) * 8
           && format.mFramesPerPacket == 1;
}

static bool validateOutputStreamFormats(OSXAudioDeviceID deviceId,
                                        const std::vector<ChannelBufferDetails>& details,
                                        const std::function<void(std::string, OSStatus)>& logError)
{
    AudioObjectPropertyAddress streamsAddress {
        kAudioDevicePropertyStreams,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    UInt32 streamsSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(deviceId, &streamsAddress, 0, nullptr, &streamsSize);
    if (status != noErr) {
        logError("Failed to get device " + std::to_string(deviceId) + " output streams size, err: ", status);
        return false;
    }

    std::vector<AudioStreamID> streamIds(streamsSize / sizeof(AudioStreamID));
    status = AudioObjectGetPropertyData(deviceId, &streamsAddress, 0, nullptr, &streamsSize, streamIds.data());
    if (status != noErr) {
        logError("Failed to get device " + std::to_string(deviceId) + " output streams, err: ", status);
        return false;
    }

    std::vector<int> checkedStreams;
    for (const ChannelBufferDetails& detail : details) {
        if (std::find(checkedStreams.cbegin(), checkedStreams.cend(), detail.streamNumber) != checkedStreams.cend()) {
            continue;
        }

        if (detail.streamNumber < 0 || static_cast<size_t>(detail.streamNumber) >= streamIds.size()) {
            LOGE() << "CoreAudio output stream index " << detail.streamNumber
                   << " is outside stream list for device " << deviceId;
            return false;
        }

        AudioStreamBasicDescription format {};
        UInt32 formatSize = sizeof(format);
        AudioObjectPropertyAddress formatAddress {
            kAudioStreamPropertyVirtualFormat,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster
        };

        status = AudioObjectGetPropertyData(streamIds[detail.streamNumber], &formatAddress, 0, nullptr, &formatSize, &format);
        if (status != noErr) {
            logError("Failed to get device " + std::to_string(deviceId) + " output stream format, err: ", status);
            return false;
        }

        if (!isSupportedCallbackFormat(format)) {
            LOGE() << "Unsupported CoreAudio output stream format for device " << deviceId
                   << ", stream " << detail.streamNumber
                   << ": expected native packed 32-bit float PCM";
            return false;
        }

        checkedStreams.push_back(detail.streamNumber);
    }

    return true;
}

inline static bool canBeDirectlyMapped(const std::vector<ChannelBufferDetails>& details,
                                       uint32_t callbackDataStride)
{
    std::optional<int> firstStreamNumber;
    std::optional<uint32_t> expectedOffset;
    for (int channel = 0; channel < (int)details.size(); ++channel) {
        const auto& detail = details[channel];
        if (detail.dataStrideSamples != callbackDataStride) {
            return false;
        }
        if (!firstStreamNumber.has_value()) {
            firstStreamNumber = detail.streamNumber;
        }
        if (firstStreamNumber.value() != detail.streamNumber) {
            return false;
        }
        if (!expectedOffset.has_value()) {
            expectedOffset = detail.dataOffsetSamples;
        }
        if (expectedOffset.value() != detail.dataOffsetSamples) {
            return false;
        }
        *expectedOffset += 1;
    }
    return true;
}

static std::vector<ChannelBufferDetails> getFittingChannelStreamsFromDevice(OSXAudioDeviceID deviceId, int channelCount,
                                                                            const std::function<void(std::string, OSStatus)>& logError)
{
    auto outputStreamInfos = getChannelBufferDetails(deviceId, logError);
    if (outputStreamInfos.size()
        < (unsigned int)channelCount) {
        logError("Not enough channels are available with current device", noErr);
        return {};
    }
    if (channelCount == 2) {
        filterAndReorderStereoHardwareChannels(deviceId, outputStreamInfos);
    }
    if (outputStreamInfos.size() < (unsigned int)channelCount) {
        logError("Not enough channels are available with current device after filtering for stereo preference", noErr);
        return {};
    }
    if (outputStreamInfos.size()
        > (unsigned int)channelCount) {
        // if there are more hardware channels than requested, we just use the first ones. This is a fallback if filterStereoHardwareChannels was not successful
        outputStreamInfos.resize(channelCount);
    }
    return outputStreamInfos;
}

static std::optional<Float64> closestAvailableSampleRate(OSXAudioDeviceID deviceId, Float64 requested,
                                                         const std::function<void(std::string, OSStatus)>& onError)
{
    AudioObjectPropertyAddress address {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 size = 0;
    if (auto status = AudioObjectGetPropertyDataSize(deviceId, &address, 0, nullptr, &size); status != noErr || size == 0) {
        onError("Failed to get device " + std::to_string(deviceId) + " available sample rates" + ", err: " + std::to_string(status),
                status);
        return std::nullopt;
    }

    std::vector<AudioValueRange> ranges(size / sizeof(AudioValueRange));
    if (auto status = AudioObjectGetPropertyData(deviceId, &address, 0, nullptr, &size, ranges.data()); status != noErr) {
        onError("Failed to get device " + std::to_string(deviceId) + " available sample rates" + ", err: " + std::to_string(status),
                status);
        return std::nullopt;
    }

    std::optional<Float64> best;
    Float64 bestDistance = std::numeric_limits<Float64>::max();

    for (const AudioValueRange& range : ranges) {
        Float64 candidate = requested;

        if (requested < range.mMinimum) {
            candidate = range.mMinimum;
        } else if (requested > range.mMaximum) {
            candidate = range.mMaximum;
        }

        Float64 distance = std::abs(candidate - requested);
        if (distance < bestDistance) {
            best = candidate;
            bestDistance = distance;
        }
    }

    return best;
}

static std::optional<OutputSpec> prepareDeviceWithOutputSpec(OSXAudioDeviceID deviceId, const OutputSpec& spec,
                                                             const std::function<void(std::string, OSStatus)>& logError)
{
    AudioObjectPropertyAddress requestedSampleRate{
        kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster };

    Float64 currentSampleRate = 0.0;
    UInt32 sampleRateSize = sizeof(currentSampleRate);

    OSStatus result
        =AudioObjectGetPropertyData(deviceId, &requestedSampleRate, 0, nullptr,
                                    &sampleRateSize, &currentSampleRate);

    if (result != noErr) {
        logError("Failed to get current sample rate for device " + std::to_string(deviceId) + ", err: ", result);
        return std::nullopt;
    }

    Float64 sampleRate = closestAvailableSampleRate(deviceId, spec.sampleRate, logError)
                         .value_or(currentSampleRate);

    if (std::abs(currentSampleRate - sampleRate) > 0.5) {
        result = AudioObjectSetPropertyData(
            deviceId,
            &requestedSampleRate,
            0,
            nullptr,
            sizeof(sampleRate),
            &sampleRate);

        if (result != noErr) {
            sampleRate = currentSampleRate;
        }
    } else {
        sampleRate = currentSampleRate;
    }

    AudioValueRange bufferSizeRange = { 0, 0 };
    UInt32 bufferSizeRangeSize = sizeof(AudioValueRange);
    AudioObjectPropertyAddress bufferSizeRangeAddress = {
        .mSelector = kAudioDevicePropertyBufferFrameSizeRange,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster
    };

    result = AudioObjectGetPropertyData(deviceId, &bufferSizeRangeAddress, 0, 0, &bufferSizeRangeSize, &bufferSizeRange);
    if (result != noErr) {
        logError("Failed to create Audio Queue Output, err: ", result);
        return std::nullopt;
    }

    samples_t minBufferSize = static_cast<samples_t>(bufferSizeRange.mMinimum);
    samples_t maxBufferSize = static_cast<samples_t>(bufferSizeRange.mMaximum);
    UInt32 bufferSizeOut = std::min(maxBufferSize, std::max(minBufferSize, spec.samplesPerChannel));

    AudioObjectPropertyAddress preferredBufferSizeAddress = {
        .mSelector = kAudioDevicePropertyBufferFrameSize,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster
    };

    result = AudioObjectSetPropertyData(deviceId, &preferredBufferSizeAddress, 0, 0, sizeof(bufferSizeOut), (void*)&bufferSizeOut);
    if (result != noErr) {
        logError("Failed to create Audio Queue Output, err: ", result);
        return std::nullopt;
    }

    UInt32 actualBufferSizeOut = bufferSizeOut;
    UInt32 actualBufferSizeOutSize = sizeof(actualBufferSizeOut);
    result = AudioObjectGetPropertyData(deviceId, &preferredBufferSizeAddress, 0, 0, &actualBufferSizeOutSize, &actualBufferSizeOut);
    if (result != noErr) {
        logError("Failed to get Audio Device bufferFrameSize, err: ", result);
        return std::nullopt;
    }
    bufferSizeOut = actualBufferSizeOut;

    return OutputSpec {
        .sampleRate = (uint64_t)sampleRate, .samplesPerChannel = bufferSizeOut, .audioChannelCount = spec.audioChannelCount
    };
}

AudioWorkGroup createAudioWorkgroup(OSXAudioDeviceID deviceId)
{
    AudioObjectPropertyAddress pa;
    pa.mSelector = kAudioDevicePropertyIOThreadOSWorkgroup;
    pa.mScope = kAudioObjectPropertyScopeWildcard;
    pa.mElement = kAudioObjectPropertyElementMaster;
    os_workgroup_t workgroup;
    uint32_t workgroupSize = sizeof(workgroup);
    if (AudioObjectGetPropertyData(deviceId, &pa, 0, nullptr, &workgroupSize,
                                   &workgroup) != noErr) {
        return {};
    }

    return makeAudioWorkgroup(workgroup);
}

bool OSXDirectAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    if (isOpened()) {
        return false;
    }

    m_data->clear();

    auto deviceId = getAudioDeviceId(spec.deviceId);
    if (!deviceId) {
        logError("Failed to find device " + spec.deviceId, noErr);
        return false;
    }
    auto bestOutputStreamInfos = getFittingChannelStreamsFromDevice(
        *deviceId, spec.output.audioChannelCount, &logError);
    if (bestOutputStreamInfos.empty()) {
        return false;
    }

    IF_ASSERT_FAILED_X(validateOutputStreamFormats(*deviceId, bestOutputStreamInfos, &logError),
                       "CoreAudio output stream format must be native packed 32-bit float PCM") {
        return false;
    }

    auto actualOutputSpec = prepareDeviceWithOutputSpec(*deviceId, spec.output, &logError);
    if (!actualOutputSpec) {
        return false;
    }

    m_data->canBeDirectlyMapped = canBeDirectlyMapped(bestOutputStreamInfos,
                                                      spec.output.audioChannelCount);
    m_data->format = spec;
    m_data->format.output = actualOutputSpec.value();
    m_data->format.deviceId = QString::number(*deviceId).toStdString();
    m_data->channelBufferOutputDetails = std::move(bestOutputStreamInfos);
    m_data->outBuffer.resize(m_data->format.output.samplesPerChannel
                             * m_data->format.output.audioChannelCount);
    m_data->deviceId = *deviceId;

    auto result = AudioDeviceCreateIOProcID(*deviceId, coreAudioIOProc, m_data.get(),
                                            &m_data->procId);
    if (result != noErr) {
        m_data->clear();
        logError("Failed to create Audio Device IO Proc, err: ", result);
        return false;
    }

    result = AudioDeviceStart(*deviceId, m_data->procId);
    if (result != noErr) {
        AudioDeviceDestroyIOProcID(*deviceId, m_data->procId);
        m_data->clear();
        logError("Failed to start Audio Device, err: ", result);
        return false;
    }

    //! NOTE From now on the device is running, so make sure the system knows whether
    //! it is allowed to go to sleep while that is the case
    OSXIdleSleepPolicy::setPreventIdleSleep(m_livePlaybackOngoing);

    m_audioWorkGroup = createAudioWorkgroup(*deviceId);
    m_currentWorkgroupChanged.notify();

    if (activeSpec) {
        *activeSpec = m_data->format;
    }

    m_activeSpecChanged.send(m_data->format);

    LOGI() << "Connected to " << m_data->format.deviceId
           << " with bufferSize " << m_data->format.output.samplesPerChannel
           << ", sampleRate " << m_data->format.output.sampleRate;

    return true;
}

void OSXDirectAudioDriver::close()
{
    doClose();
}

void OSXDirectAudioDriver::doClose()
{
    if (!isOpened()) {
        return;
    }

    m_data->stopPending = true;
    // we spin while we wait for the callback to stop the device. That way we can be sure that data will no longer be used by the callback
    for (int i = 0; i < 100 && !m_data->stopped; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!m_data->stopped) {
        [[maybe_unused]] auto _leaked = m_data.release(); // we let it leak since it might still be accessed by the callback
        m_data = std::make_unique<Data>();
    } else {
        AudioDeviceDestroyIOProcID(m_data->deviceId, m_data->procId);
        m_data->clear();
    }
    m_audioWorkGroup = {};
    m_currentWorkgroupChanged.notify();
}

bool OSXDirectAudioDriver::isOpened() const
{
    return m_data->procId != nullptr;
}

void OSXDirectAudioDriver::setLivePlaybackOngoing(bool ongoing)
{
    m_livePlaybackOngoing = ongoing;
    OSXIdleSleepPolicy::setPreventIdleSleep(ongoing);
}

const OSXDirectAudioDriver::Spec& OSXDirectAudioDriver::activeSpec() const
{
    return m_data->format;
}

async::Channel<OSXDirectAudioDriver::Spec> OSXDirectAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

AudioDeviceList OSXDirectAudioDriver::availableOutputDevices() const
{
    std::lock_guard lock(m_devicesMutex);

    AudioDeviceList deviceList;
    deviceList.push_back({ DEFAULT_DEVICE_ID, muse::trc("audio", "System default") });

    for (auto& device : m_outputDevices) {
        AudioDevice deviceInfo;
        deviceInfo.id = QString::number(device.first).toStdString();
        deviceInfo.name = device.second;

        deviceList.push_back(deviceInfo);
    }

    return deviceList;
}

async::Notification OSXDirectAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}

void OSXDirectAudioDriver::updateDeviceMap()
{
    std::lock_guard lock(m_devicesMutex);

    UInt32 propertySize;
    OSStatus result;
    std::vector<AudioObjectID> audioObjects = {};
    m_outputDevices.clear();
    m_inputDevices.clear();

    AudioObjectPropertyAddress devicesPropertyAddress = {
        .mSelector = kAudioHardwarePropertyDevices,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster,
    };

    AudioObjectPropertyAddress namePropertyAddress = {
        .mSelector = kAudioDevicePropertyDeviceNameCFString,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster,
    };

    auto getStreamsCount
        = [](const AudioObjectID& id, const AudioObjectPropertyScope& scope, const std::string& deviceName) -> unsigned int {
        AudioObjectPropertyAddress propertyAddress = {
            .mSelector  = kAudioDevicePropertyStreamConfiguration,
            .mScope     = scope,
            .mElement   = kAudioObjectPropertyElementWildcard
        };
        UInt32 propertySize = 0;
        OSStatus result = AudioObjectGetPropertyDataSize(id, &propertyAddress, 0, NULL, &propertySize);
        if (result != noErr) {
            logError("Failed to get device's (" + deviceName + ") streams size, err: ", result);
            return 0;
        }

        auto freeBufferList = [](AudioBufferList* list) { free(list); };
        std::unique_ptr<AudioBufferList, decltype(freeBufferList)> bufferList(reinterpret_cast<AudioBufferList*>(malloc(propertySize)),
                                                                              freeBufferList);
        result = AudioObjectGetPropertyData(id, &propertyAddress, 0, NULL, &propertySize, bufferList.get());
        if (result != noErr) {
            logError("Failed to get device's (" + deviceName + ") streams, err: ", result);
            return 0;
        }

        return bufferList->mNumberBuffers;
    };

    result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &devicesPropertyAddress, 0, NULL, &propertySize);
    if (result != noErr) {
        logError("Failed to get devices count, err: ", result);
        return;
    }

    audioObjects.resize(propertySize / sizeof(OSXAudioDeviceID));
    result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicesPropertyAddress, 0, NULL, &propertySize, audioObjects.data());
    if (result != noErr) {
        logError("Failed to get devices list, err: ", result);
        return;
    }

    for (auto&& deviceId : audioObjects) {
        CFStringRef nameRef;
        propertySize = sizeof(nameRef);

        result = AudioObjectGetPropertyData(deviceId, &namePropertyAddress, 0, NULL, &propertySize, &nameRef);
        if (result != noErr) {
            logError("Failed to get device's name, err: ", result);
            continue;
        }

        NSString* nsString = (NSString*)nameRef;
        std::string deviceName = [nsString UTF8String];

        if (getStreamsCount(deviceId, kAudioObjectPropertyScopeOutput, deviceName) > 0) {
            m_outputDevices[deviceId] = deviceName;
        }

        if (getStreamsCount(deviceId, kAudioObjectPropertyScopeInput, deviceName) > 0) {
            m_inputDevices[deviceId] = deviceName;
        }

        CFRelease(nameRef);
    }
    m_availableOutputDevicesChanged.notify();
}

std::vector<samples_t> OSXDirectAudioDriver::availableOutputDeviceBufferSizes() const
{
    OSXAudioDeviceID osxDeviceId = this->osxDeviceId();
    AudioObjectPropertyAddress bufferFrameSizePropertyAddress = {
        .mSelector = kAudioDevicePropertyBufferFrameSizeRange,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster
    };

    AudioValueRange range = { 0, 0 };
    UInt32 dataSize = sizeof(AudioValueRange);
    OSStatus rangeResult = AudioObjectGetPropertyData(osxDeviceId, &bufferFrameSizePropertyAddress, 0, NULL, &dataSize, &range);
    if (rangeResult != noErr) {
        logError("Failed to get device " + m_data->format.deviceId + " bufferFrameSize, err: ", rangeResult);
        return {};
    }

    samples_t minimum = std::max(static_cast<samples_t>(range.mMinimum), MINIMUM_BUFFER_SIZE);
    samples_t maximum = std::min(static_cast<samples_t>(range.mMaximum), MAXIMUM_BUFFER_SIZE);

    std::vector<samples_t> result;
    for (samples_t bufferSize = maximum; bufferSize >= minimum;) {
        result.push_back(bufferSize);
        bufferSize /= 2;
    }

    std::sort(result.begin(), result.end());

    return result;
}

//TODO: replace hardcoded values with truth
std::vector<sample_rate_t> OSXDirectAudioDriver::availableOutputDeviceSampleRates() const
{
    return {
        44100,
        48000,
        88200,
        96000,
    };
}

static std::optional<OSXAudioDeviceID> getDefaultDeviceId(const std::function<void(std::string, OSStatus)>& logError)
{
    OSXAudioDeviceID osxDeviceId = kAudioObjectUnknown;
    UInt32 deviceIdSize = sizeof(osxDeviceId);

    AudioObjectPropertyAddress deviceNamePropertyAddress = {
        .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
        .mScope = kAudioDevicePropertyScopeOutput,
        .mElement = kAudioObjectPropertyElementMaster
    };

    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &deviceNamePropertyAddress, 0, 0, &deviceIdSize, &osxDeviceId);
    if (result != noErr) {
        logError("Failed to get default device ID, err: ", result);
        return std::nullopt;
    }

    return osxDeviceId;
}

UInt32 OSXDirectAudioDriver::osxDeviceId() const
{
    AudioDeviceID deviceId = m_data->format.deviceId;
    if (deviceId == DEFAULT_DEVICE_ID) {
        auto defaultDeviceId = getDefaultDeviceId(&logError);
        if (!defaultDeviceId) {
            logError("Failed to get default device ID, err: ", noErr);
            return kAudioObjectUnknown;
        }
        return *defaultDeviceId;
    }

    return QString::fromStdString(deviceId).toInt();
}

void OSXDirectAudioDriver::logError(const std::string message, OSStatus error)
{
    if (error == noErr) {
        return;
    }

    char errorString[5];

    UInt32 errorBigEndian = CFSwapInt32HostToBig(error);
    errorString[0] = errorBigEndian & 0xFF;
    errorString[1] = (errorBigEndian >> 8) & 0xFF;
    errorString[2] = (errorBigEndian >> 16) & 0xFF;
    errorString[3] = (errorBigEndian >> 24) & 0xFF;
    errorString[4] = '\0';
    if (isprint(errorString[0]) && isprint(errorString[1]) && isprint(errorString[2]) && isprint(errorString[3])) {
        LOGE() << message << errorString << "(" << error << ")";
    } else {
        LOGE() << message << error;
    }
}

static OSStatus onDeviceListChanged(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses,
                                    void* inClientData)
{
    UNUSED(inObjectID);
    UNUSED(inNumberAddresses);
    UNUSED(inAddresses);
    auto driver = reinterpret_cast<OSXDirectAudioDriver*>(inClientData);
    driver->updateDeviceMap();

    return noErr;
}

void OSXDirectAudioDriver::initDeviceMapListener()
{
    AudioObjectPropertyAddress propertyAddress;
    propertyAddress.mSelector = kAudioHardwarePropertyDevices;
    propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement = kAudioObjectPropertyElementMaster;

    auto result = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &propertyAddress, &onDeviceListChanged, this);
    if (result != noErr) {
        logError("Failed to add devices list listener, err: ", result);
        return;
    }

    m_deviceMapListenerRegistered = true;
}

void OSXDirectAudioDriver::removeDeviceMapListener()
{
    if (!m_deviceMapListenerRegistered) {
        return;
    }

    AudioObjectPropertyAddress propertyAddress;
    propertyAddress.mSelector = kAudioHardwarePropertyDevices;
    propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement = kAudioObjectPropertyElementMaster;

    auto result = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &propertyAddress, &onDeviceListChanged, this);
    if (result != noErr) {
        logError("Failed to remove devices list listener, err: ", result);
        return;
    }

    m_deviceMapListenerRegistered = false;
}

std::optional<int> muse::audio::OSXDirectAudioDriver::getAudioDeviceId(
    const AudioDeviceID& deviceId) const
{
    if (deviceId.empty() || deviceId == DEFAULT_DEVICE_ID) {
        return getDefaultDeviceId(&logError); //default device used
    }

    std::lock_guard lock(m_devicesMutex);

    uint deviceIdInt = QString::fromStdString(deviceId).toInt();
    auto index = std::find_if(m_outputDevices.begin(), m_outputDevices.end(), [&deviceIdInt](auto& d) {
        return d.first == deviceIdInt;
    });

    if (index == m_outputDevices.end()) {
        return std::nullopt;
    }
    return index->first;
}
