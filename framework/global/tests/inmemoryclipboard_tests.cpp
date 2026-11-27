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

#include "global/internal/inmemoryclipboard.h"

using namespace muse;

TEST(Global_InMemoryClipboardTests, MimeData_Empty_ByDefault)
{
    //! [GIVEN] A new clipboard
    InMemoryClipboard clipboard;

    //! [THEN] It has no data
    EXPECT_TRUE(clipboard.mimeData().formats().empty());
    EXPECT_TRUE(clipboard.text().empty());
}

TEST(Global_InMemoryClipboardTests, SetMimeData_ReplacesTheContent)
{
    //! [GIVEN] A clipboard with data of one format
    InMemoryClipboard clipboard;
    MimeData first;
    first.setData("test/a", ByteArray("1"));
    clipboard.setMimeData(first);

    //! [WHEN] Setting data of another format
    MimeData second;
    second.setData("test/b", ByteArray("2"));
    clipboard.setMimeData(second);

    //! [THEN] Only the new data is there
    const MimeData result = clipboard.mimeData();
    EXPECT_FALSE(result.hasFormat("test/a"));
    EXPECT_EQ(result.data("test/b"), ByteArray("2"));
}

TEST(Global_InMemoryClipboardTests, MimeData_IsACopy)
{
    //! [GIVEN] A clipboard with data
    InMemoryClipboard clipboard;
    MimeData data;
    data.setData("test/a", ByteArray("1"));
    clipboard.setMimeData(data);

    //! [WHEN] Changing the returned data
    MimeData copy = clipboard.mimeData();
    copy.setData("test/a", ByteArray("2"));

    //! [THEN] The clipboard is not changed
    EXPECT_EQ(clipboard.mimeData().data("test/a"), ByteArray("1"));
}

TEST(Global_InMemoryClipboardTests, SetText_TextIsReturned)
{
    //! [GIVEN] A clipboard
    InMemoryClipboard clipboard;

    //! [WHEN] Setting text
    clipboard.setText(u"abc");

    //! [THEN] The text is returned
    EXPECT_EQ(clipboard.text(), u"abc");
}
