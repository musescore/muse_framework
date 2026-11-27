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

#ifndef NO_QT_SUPPORT

#include <gtest/gtest.h>

#include <QInputMethodEvent>
#include <QMouseEvent>

#include "global/qt/qtinputconvert.h"

using namespace muse;

TEST(Global_QtInputConvertTests, FromQtInputMethodEvent_CopiesTheCommitAndPreeditStrings)
{
    //! [GIVEN] An input method event with a preedit and a commit string
    QInputMethodEvent qevent(QString("pre"), {});
    qevent.setCommitString(QString("commit"));

    //! [WHEN] Converting it
    const TextInputEvent event = fromQtInputMethodEvent(&qevent);

    //! [THEN] The strings are copied, there is no replacement: the commit string is inserted
    EXPECT_EQ(event.commitString, String(u"commit"));
    EXPECT_EQ(event.preeditString, String(u"pre"));
    EXPECT_EQ(event.replacementStart, 0);
    EXPECT_EQ(event.replacementLength, 0);
}

TEST(Global_QtInputConvertTests, FromQtInputMethodEvent_CopiesTheReplacement)
{
    //! [GIVEN] An input method event that replaces 3 characters before the cursor
    QInputMethodEvent qevent;
    qevent.setCommitString(QString("commit"), -3, 3);

    //! [WHEN] Converting it
    const TextInputEvent event = fromQtInputMethodEvent(&qevent);

    //! [THEN] The replacement is copied
    EXPECT_EQ(event.commitString, String(u"commit"));
    EXPECT_EQ(event.replacementStart, -3);
    EXPECT_EQ(event.replacementLength, 3);
}

TEST(Global_QtInputConvertTests, FromQtMouseEvent_BackAndForwardButtons_AreKept)
{
    //! [GIVEN] A mouse event of the back button, the forward button is pressed too
    const Qt::MouseButtons pressed = Qt::BackButton | Qt::ForwardButton;
    QMouseEvent qevent(QEvent::MouseButtonPress, QPointF(10, 20), Qt::BackButton, pressed, Qt::NoModifier);

    //! [WHEN] Converting it
    const PointerEvent event = fromQtMouseEvent(&qevent);

    //! [THEN] The button and the pressed buttons are the same as in Qt
    EXPECT_EQ(int(event.button), int(Qt::BackButton));
    EXPECT_EQ(int(event.buttons), int(pressed));
    EXPECT_EQ(event.type, PointerEvent::Type::Press);
}

#endif // NO_QT_SUPPORT
