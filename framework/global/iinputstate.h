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

#include "modularity/imoduleinterface.h"

#include "types/inputtypes.h"

namespace muse {
//! NOTE The current state of the input devices
class IInputState : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IInputState)

public:
    virtual ~IInputState() = default;

    //! The modifiers as of the last processed event
    virtual KeyboardModifiers keyboardModifiers() const = 0;

    //! The modifiers right now
    virtual KeyboardModifiers queryKeyboardModifiers() const = 0;

    virtual MouseButtons mouseButtons() const = 0;
};
}
