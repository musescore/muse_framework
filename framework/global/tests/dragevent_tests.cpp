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
#include <gtest/gtest.h>

#include "global/types/dragevent.h"

using namespace muse;

TEST(Global_DragEventTests, SetDropAction_PossibleAction_IsApplied)
{
    //! [GIVEN] Both copy and move are possible
    DragEvent event;
    event.possibleActions = DropActions(DropAction::Copy) | DropAction::Move;

    //! [WHEN] Setting move
    event.setDropAction(DropAction::Move);

    //! [THEN] The drop action is move
    EXPECT_EQ(event.dropAction, DropAction::Move);
}

TEST(Global_DragEventTests, SetDropAction_NotPossibleAction_IsNotApplied)
{
    //! [GIVEN] Only copy is possible
    DragEvent event;
    event.possibleActions = DropActions(DropAction::Copy);
    event.dropAction = DropAction::Copy;

    //! [WHEN] Setting move
    event.setDropAction(DropAction::Move);

    //! [THEN] The drop action is still copy
    EXPECT_EQ(event.dropAction, DropAction::Copy);
}

TEST(Global_DragEventTests, SetDropAction_Ignore_IsAlwaysApplied)
{
    //! [GIVEN] Only copy is possible
    DragEvent event;
    event.possibleActions = DropActions(DropAction::Copy);
    event.dropAction = DropAction::Copy;

    //! [WHEN] Setting ignore
    event.setDropAction(DropAction::Ignore);

    //! [THEN] The drop action is ignore
    EXPECT_EQ(event.dropAction, DropAction::Ignore);
}
