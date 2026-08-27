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

#include <windows.h>

#include "log.h"

using namespace muse;

struct IdleSleepBlocker::Data {
    //! NOTE Kept alive because the reason string is referenced by the request, not copied
    std::wstring reason;
    HANDLE request = INVALID_HANDLE_VALUE;
};

IdleSleepBlocker::IdleSleepBlocker(const std::string& reason)
    : m_data(std::make_unique<Data>())
{
    int size = MultiByteToWideChar(CP_UTF8, 0, reason.c_str(), static_cast<int>(reason.size()), nullptr, 0);
    m_data->reason.resize(static_cast<size_t>(size));
    MultiByteToWideChar(CP_UTF8, 0, reason.c_str(), static_cast<int>(reason.size()), m_data->reason.data(), size);

    REASON_CONTEXT context = {};
    context.Version = POWER_REQUEST_CONTEXT_VERSION;
    context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
    context.Reason.SimpleReasonString = m_data->reason.data();

    HANDLE request = PowerCreateRequest(&context);
    if (request == INVALID_HANDLE_VALUE) {
        LOGW() << "Failed to create power request, err: " << GetLastError() << ", the system may go to sleep";
        return;
    }

    if (!PowerSetRequest(request, PowerRequestSystemRequired)) {
        LOGW() << "Failed to set power request, err: " << GetLastError() << ", the system may go to sleep";
        CloseHandle(request);
        return;
    }

    m_data->request = request;
}

IdleSleepBlocker::~IdleSleepBlocker()
{
    if (m_data->request == INVALID_HANDLE_VALUE) {
        return;
    }

    PowerClearRequest(m_data->request, PowerRequestSystemRequired);
    CloseHandle(m_data->request);
}
