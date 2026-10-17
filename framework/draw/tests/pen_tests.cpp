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

#include <regex>

#include <QPen>

#include "draw/types/pen.h"
#include "draw/utils/drawdatajson.h"

using namespace muse;
using namespace muse::draw;

class Draw_PenTests : public ::testing::Test
{
public:
};

static DrawDataPtr drawDataWithPen(const Pen& pen)
{
    DrawDataPtr data = std::make_shared<DrawData>();
    data->states[0].pen = pen;

    DrawData::Data d;
    d.state = 0;
    DrawPath path;
    path.pen = pen;
    d.paths.push_back(path);
    data->item.datas.push_back(d);

    return data;
}

TEST_F(Draw_PenTests, MiterLimit_DefaultMatchesQPen)
{
    //! CHECK Existing callers keep Qt's default
    EXPECT_DOUBLE_EQ(Pen().miterLimit(), QPen().miterLimit());
}

TEST_F(Draw_PenTests, MiterLimit_Equality)
{
    //! GIVEN Two pens that differ only in their miter limit
    Pen a;
    Pen b;
    b.setMiterLimit(3.0);

    //! CHECK
    EXPECT_DOUBLE_EQ(b.miterLimit(), 3.0);
    EXPECT_FALSE(a == b);

    b.setMiterLimit(a.miterLimit());
    EXPECT_TRUE(a == b);
}

TEST_F(Draw_PenTests, MiterLimit_QPenRoundTrip)
{
    //! GIVEN Pen with a non-default miter limit
    Pen pen;
    pen.setJoinStyle(PenJoinStyle::MiterJoin);
    pen.setMiterLimit(3.0);

    //! DO Convert to QPen and back
    QPen qpen = Pen::toQPen(pen);
    Pen back = Pen::fromQPen(qpen);

    //! CHECK
    EXPECT_DOUBLE_EQ(qpen.miterLimit(), 3.0);
    EXPECT_DOUBLE_EQ(back.miterLimit(), 3.0);
    EXPECT_TRUE(back == pen);
}

TEST_F(Draw_PenTests, MiterLimit_JsonRoundTrip)
{
    //! GIVEN Draw data whose state and path pens have a non-default miter limit
    Pen pen;
    pen.setJoinStyle(PenJoinStyle::MiterJoin);
    pen.setMiterLimit(3.0);
    DrawDataPtr data = drawDataWithPen(pen);

    //! DO Write to JSON and read back
    RetVal<DrawDataPtr> back = DrawDataJson::fromJson(DrawDataJson::toJson(data));

    //! CHECK
    ASSERT_TRUE(back.ret);
    EXPECT_DOUBLE_EQ(back.val->states.at(0).pen.miterLimit(), 3.0);
    ASSERT_EQ(back.val->item.datas.size(), 1u);
    ASSERT_EQ(back.val->item.datas.at(0).paths.size(), 1u);
    EXPECT_DOUBLE_EQ(back.val->item.datas.at(0).paths.at(0).pen.miterLimit(), 3.0);
}

TEST_F(Draw_PenTests, MiterLimit_JsonWithoutKeyUsesDefault)
{
    //! GIVEN JSON written before pens had a miter limit
    Pen pen;
    pen.setMiterLimit(3.0);
    ByteArray json = DrawDataJson::toJson(drawDataWithPen(pen), false);
    std::string text = std::regex_replace(std::string(json.constChar(), json.size()), std::regex("\"miterLimit\":[^,}]*,?"), "");
    ASSERT_EQ(text.find("miterLimit"), std::string::npos);

    //! DO Read it
    RetVal<DrawDataPtr> back = DrawDataJson::fromJson(ByteArray(text.c_str(), text.size()));

    //! CHECK The pens get the default miter limit
    ASSERT_TRUE(back.ret);
    EXPECT_DOUBLE_EQ(back.val->states.at(0).pen.miterLimit(), Pen().miterLimit());
    EXPECT_DOUBLE_EQ(back.val->item.datas.at(0).paths.at(0).pen.miterLimit(), Pen().miterLimit());
}
