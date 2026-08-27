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

#pragma once

#include <memory>
#include <string>

namespace muse {
//! NOTE Prevents the system from going to idle sleep for as long as this object exists.
//! Meant for tasks that can take a long time while the user is not interacting with the
//! application and nothing is audible, like exporting an audio file: without this, the
//! system can suspend halfway through such a task.
//!
//! It does not prevent the display from sleeping, and it does not prevent sleep that the
//! user or the system asks for explicitly (closing the lid, low battery, etc.).
//!
//! Blocking is best effort: if the platform refuses, or has no way to express this, the
//! failure is logged and the object simply does nothing.
class IdleSleepBlocker
{
public:
    //! NOTE `reason` is shown to the user by some systems, so it should describe the task
    //! being performed, and be translated
    explicit IdleSleepBlocker(const std::string& reason);
    ~IdleSleepBlocker();

    IdleSleepBlocker(const IdleSleepBlocker&) = delete;
    IdleSleepBlocker& operator=(const IdleSleepBlocker&) = delete;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};

using IdleSleepBlockerPtr = std::shared_ptr<IdleSleepBlocker>;
}
