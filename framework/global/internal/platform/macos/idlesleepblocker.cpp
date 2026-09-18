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

#include "../../../idlesleepblocker.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "log.h"

using namespace muse;

struct IdleSleepBlocker::Data {
    IOPMAssertionID assertionId = kIOPMNullAssertionID;
};

IdleSleepBlocker::IdleSleepBlocker(const std::string& reason)
    : m_data(std::make_unique<Data>())
{
    CFStringRef reasonRef = CFStringCreateWithCString(kCFAllocatorDefault, reason.c_str(), kCFStringEncodingUTF8);
    if (!reasonRef) {
        LOGW() << "Failed to create reason string, the system may go to sleep";
        return;
    }

    IOPMAssertionID assertionId = kIOPMNullAssertionID;
    IOReturn result = IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep,
                                                  kIOPMAssertionLevelOn, reasonRef, &assertionId);
    CFRelease(reasonRef);

    if (result != kIOReturnSuccess) {
        LOGW() << "Failed to create power assertion, err: " << result << ", the system may go to sleep";
        return;
    }

    m_data->assertionId = assertionId;
}

IdleSleepBlocker::~IdleSleepBlocker()
{
    if (m_data->assertionId != kIOPMNullAssertionID) {
        IOPMAssertionRelease(m_data->assertionId);
    }
}
