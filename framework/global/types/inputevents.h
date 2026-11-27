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

#include "draw/types/geometry.h"

#include "types/string.h"
#include "types/inputtypes.h"

//! NOTE Toolkit-independent input events. The view layer (Qt, web, ...)
//! converts its native events to these, so the rest of the code does not depend on the toolkit.
//! The numeric values of keys, modifiers and buttons match Qt's ones.
namespace muse {
class InputEvent
{
public:
    bool isAccepted() const { return m_accepted; }
    void setAccepted(bool accepted) { m_accepted = accepted; }
    void accept() { m_accepted = true; }
    void ignore() { m_accepted = false; }

private:
    bool m_accepted = false;
};

struct KeyEvent : public InputEvent
{
    enum class Type {
        Press,
        Release
    };

    Type type = Type::Press;
    int key = 0; // KeyboardKey or a character code
    KeyboardModifiers modifiers;
    String text;
    bool isAutoRepeat = false;
};

//! NOTE Input method (IME) event: the text being composed and the text to be committed
struct TextInputEvent : public InputEvent
{
    String commitString;
    String preeditString;

    //! The part of the text around the cursor that is replaced by the commit string:
    //! the start is relative to the cursor (may be negative), the zero length means the insertion
    int replacementStart = 0;
    int replacementLength = 0;
};

//! NOTE Positions of the pointer events are in the view coordinates (pixels)
struct PointerEvent : public InputEvent
{
    enum class Type {
        Press,
        Move,
        Release,
        DoubleClick
    };

    Type type = Type::Press;
    PointF position;
    MouseButton button = MouseButton::NoButton; // the button that caused the event
    MouseButtons buttons; // the buttons that are pressed
    KeyboardModifiers modifiers;
};

struct HoverEvent : public InputEvent
{
    PointF position;
    PointF oldPosition;
    KeyboardModifiers modifiers;
};

struct WheelEvent : public InputEvent
{
    //! NOTE The same as the Qt's: the delta of a mouse wheel is 120 per step
    static constexpr int DefaultDeltasPerStep = 120;

    PointF position;
    Point pixelDelta; // zero if the device doesn't provide it
    Point angleDelta; // in the eighths of a degree
    KeyboardModifiers modifiers;
};
}
