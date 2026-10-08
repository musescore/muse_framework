/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "osxidlesleeppolicy.h"

#include <mutex>

#include <CoreAudio/AudioHardware.h>

#include "log.h"

using namespace muse::audio;

//! NOTE The policy is set from the thread that starts and stops playback, but reapplied
//! from a CoreAudio owned thread, so reading it and applying it has to be serialized
static std::mutex s_mutex;
static bool s_preventIdleSleep = false;

static void applyPolicy()
{
    std::lock_guard lock(s_mutex);

    //! NOTE 1 means: allow the CPU to idle sleep even if there is audio IO in progress
    UInt32 sleepingIsAllowed = s_preventIdleSleep ? 0 : 1;

    AudioObjectPropertyAddress address = {
        .mSelector = kAudioHardwarePropertySleepingIsAllowed,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMain
    };

    OSStatus result = AudioObjectSetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr,
                                                 sizeof(sleepingIsAllowed), &sleepingIsAllowed);
    if (result != noErr) {
        LOGE() << "Failed to set idle sleep policy, err: " << result;
    }
}

static OSStatus onServiceRestarted(AudioObjectID, UInt32, const AudioObjectPropertyAddress*, void*)
{
    //! NOTE The policy belongs to our HAL client, which is recreated when coreaudiod restarts
    applyPolicy();
    return noErr;
}

static void initServiceRestartListener()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        AudioObjectPropertyAddress address = {
            .mSelector = kAudioHardwarePropertyServiceRestarted,
            .mScope = kAudioObjectPropertyScopeGlobal,
            .mElement = kAudioObjectPropertyElementMain
        };

        OSStatus result = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, &onServiceRestarted, nullptr);
        if (result != noErr) {
            LOGE() << "Failed to add service restart listener, err: " << result;
        }
    });
}

void OSXIdleSleepPolicy::setPreventIdleSleep(bool prevent)
{
    initServiceRestartListener();

    {
        std::lock_guard lock(s_mutex);
        s_preventIdleSleep = prevent;
    }

    applyPolicy();
}
