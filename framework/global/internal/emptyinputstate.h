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

#include "global/iinputstate.h"

namespace muse {
//! NOTE The default input state for the builds without Qt, it has no modifiers and no pressed buttons.
//! The host can replace it with its own one
class EmptyInputState : public IInputState
{
public:
    KeyboardModifiers keyboardModifiers() const override { return KeyboardModifiers(); }
    KeyboardModifiers queryKeyboardModifiers() const override { return KeyboardModifiers(); }
    MouseButtons mouseButtons() const override { return MouseButtons(); }
};
}
