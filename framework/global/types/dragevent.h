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
#include "types/flags.h"
#include "types/inputevents.h"
#include "types/mimedata.h"

namespace muse {
//! NOTE The values match Qt's ones
enum class DropAction {
    Ignore = 0,
    Copy = 1,
    Move = 2
};
DECLARE_FLAGS(DropActions, DropAction)
DECLARE_OPERATORS_FOR_FLAGS(DropActions)

//! NOTE The event of a drag that is over the view, and of the drop.
//! The receiver accepts or ignores the event and sets the drop action.
struct DragEvent : public InputEvent
{
    PointF position; // in the view coordinates (pixels)
    KeyboardModifiers modifiers;
    MimeData mimeData; // the formats that the receiver asked for

    DropActions possibleActions = DropActions(DropAction::Copy) | DropAction::Move;
    DropAction dropAction = DropAction::Copy;
    bool isFromThisView = false;

    void setDropAction(DropAction action)
    {
        //! NOTE Ignore is always possible, it is not a flag of possibleActions
        if (action == DropAction::Ignore || (possibleActions & action)) {
            dropAction = action;
        }
    }
};
}
