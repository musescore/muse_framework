/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
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

 #pragma once

#include <functional>

namespace muse::audio::engine {
//! NOTE When commands arrive at the engine, it exec them.
//! These can be quick commands like changing the volume,
//! or longer commands like add a new track.
enum class OperationType {
    Undefined = -1,
    NoOperation,
    QuickOperation,
    LongOperation,
};

using Operation = std::function<void ()>;

class IExecOperation
{
public:
    virtual ~IExecOperation() = default;

    virtual void execOperation(OperationType type, const Operation& func) = 0;
};
}
