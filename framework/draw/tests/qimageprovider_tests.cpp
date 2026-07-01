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

#include <QImage>

#include "draw/types/pixmap.h"
#include "draw/internal/qimageprovider.h"

using namespace muse;
using namespace muse::draw;

class Draw_QImageProviderTests : public ::testing::Test
{
public:
};

static Pixmap makeTestPixmap(int w, int h)
{
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::blue);
    return Pixmap::fromQImage(img);
}

TEST_F(Draw_QImageProviderTests, Scaled_BelowLimit_NoClamp)
{
    QImageProvider provider;
    Pixmap origin = makeTestPixmap(100, 80);

    Size requested(2000, 1600);
    Pixmap result = provider.scaled(origin, requested);

    EXPECT_EQ(result.size(), requested);
    EXPECT_EQ(result.pixelSize(), requested);
    EXPECT_FALSE(result.isNull());
}

TEST_F(Draw_QImageProviderTests, Scaled_AboveLimit_ClampPreservesLogicalSize)
{
    QImageProvider provider;
    Pixmap origin = makeTestPixmap(100, 80);

    Size requested(8000, 6000);
    Pixmap result = provider.scaled(origin, requested);

    // logical size = requested
    EXPECT_EQ(result.size(), requested);

    // pixel size clamped: max side <= 4096
    Size ps = result.pixelSize();
    EXPECT_LE(ps.width(), 4096);
    EXPECT_LE(ps.height(), 4096);
    EXPECT_EQ(std::max(ps.width(), ps.height()), 4096);

    // pixel size != logical size
    EXPECT_NE(result.pixelSize(), result.size());

    EXPECT_FALSE(result.isNull());
}
