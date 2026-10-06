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

#include "draw/internal/fontsdatabase.h"
#include "global/io/file.h"
#include "global/io/path.h"

using namespace muse;
using namespace muse::draw;

class Draw_FontsDatabaseTests : public ::testing::Test
{
public:
};

static io::path_t freeSerifPath()
{
    return io::path_t(MUSE_DRAW_TEST_FONTS_DIR) + "/FreeSerif.ttf";
}

static io::path_t edwinPath()
{
    return io::path_t(MUSE_DRAW_TEST_FONTS_DIR) + "/edwin/Edwin-Roman.otf";
}

static ByteArray readFontData(const io::path_t& path)
{
    ByteArray data;
    EXPECT_TRUE(io::File::readFile(path, data));
    return data;
}

TEST_F(Draw_FontsDatabaseTests, FontFromData)
{
    FontsDatabase db;
    const FontDataKey key(u"Test");
    const ByteArray data = readFontData(freeSerifPath());

    EXPECT_GE(db.addFontFromData(key, data), 0);

    EXPECT_EQ(db.actualFont(key, Font::Type::Text), key);

    const FontData font = db.fontData(key, Font::Type::Text);
    EXPECT_EQ(font.key, key);
    EXPECT_EQ(font.data, data);
}

TEST_F(Draw_FontsDatabaseTests, SourceReplacedUnderSameKey)
{
    FontsDatabase db;
    const FontDataKey key(u"Test");
    const ByteArray fileData = readFontData(edwinPath());
    const ByteArray memoryData = readFontData(freeSerifPath());

    EXPECT_GE(db.addFont(key, edwinPath()), 0);
    EXPECT_EQ(db.fontData(key, Font::Type::Text).data, fileData);

    EXPECT_GE(db.addFontFromData(key, memoryData), 0);
    EXPECT_EQ(db.fontData(key, Font::Type::Text).data, memoryData);

    EXPECT_GE(db.addFont(key, edwinPath()), 0);
    EXPECT_EQ(db.fontData(key, Font::Type::Text).data, fileData);
}

TEST_F(Draw_FontsDatabaseTests, RemovedFontFallsBackToDefault)
{
    FontsDatabase db;
    const FontDataKey defaultKey(u"Default");
    const FontDataKey key(u"Test");
    db.setDefaultFont(Font::Type::Unknown, defaultKey);

    EXPECT_GE(db.addFontFromData(key, readFontData(freeSerifPath())), 0);
    db.removeFont(key);

    EXPECT_EQ(db.actualFont(key, Font::Type::Text), defaultKey);
}

TEST_F(Draw_FontsDatabaseTests, EmptyDataIsRefused)
{
    FontsDatabase db;
    const FontDataKey defaultKey(u"Default");
    const FontDataKey key(u"Test");
    db.setDefaultFont(Font::Type::Unknown, defaultKey);

    EXPECT_EQ(db.addFontFromData(key, ByteArray()), -1);

    EXPECT_EQ(db.actualFont(key, Font::Type::Text), defaultKey);
}

TEST_F(Draw_FontsDatabaseTests, MissingFileFallsBackToDefault)
{
    FontsDatabase db;
    const FontDataKey defaultKey(u"Default");
    const FontDataKey key(u"Test");
    db.setDefaultFont(Font::Type::Unknown, defaultKey);

    const io::path_t path = io::path_t(::testing::TempDir()) + "Draw_FontsDatabaseTests_Edwin-Roman.otf";
    ASSERT_TRUE(io::File::copy(edwinPath(), path, true));

    EXPECT_GE(db.addFont(key, path), 0);
    EXPECT_EQ(db.actualFont(key, Font::Type::Text), key);

    ASSERT_TRUE(io::File::remove(path));
    EXPECT_EQ(db.actualFont(key, Font::Type::Text), defaultKey);
}
