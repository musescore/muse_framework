/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include <map>
#include <string>
#include <vector>

#ifndef NO_QT_SUPPORT
#include <QColor>
#endif

#include "types/val.h"

using namespace muse;

class Global_ValTests : public ::testing::Test
{
public:
};

// Undefined = 0,
// Bool,
// Int,
// Int64,
// Double,
// String,
// List,
// Map,
// Color

TEST_F(Global_ValTests, Val_Undefined)
{
    //! GIVEN Undefined value

    Val v;

    EXPECT_EQ(v.type(), Val::Type::Undefined);
    EXPECT_EQ(v.isNull(), true);
    EXPECT_EQ(v.toBool(), false);
    EXPECT_EQ(v.toInt(), 0);
    EXPECT_EQ(v.toInt64(), 0);
    EXPECT_EQ(v.toDouble(), 0.0);
    EXPECT_EQ(v.toString(), std::string());
    EXPECT_EQ(v.toList(), ValList());
    EXPECT_EQ(v.toMap(), ValMap());
}

TEST_F(Global_ValTests, Val_Bool)
{
    //! GIVEN Value as bool

    {
        Val v(true);

        EXPECT_EQ(v.type(), Val::Type::Bool);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 1);
        EXPECT_EQ(v.toInt64(), 1);
        EXPECT_EQ(v.toDouble(), 1.0);
        EXPECT_EQ(v.toString(), "true");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v(false);

        EXPECT_EQ(v.type(), Val::Type::Bool);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "false");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
}

TEST_F(Global_ValTests, Val_Int)
{
    //! GIVEN Value as int

    {
        Val v(42);

        EXPECT_EQ(v.type(), Val::Type::Int);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 42);
        EXPECT_EQ(v.toInt64(), 42);
        EXPECT_EQ(v.toDouble(), 42.0);
        EXPECT_EQ(v.toString(), "42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
    {
        Val v(-42);

        EXPECT_EQ(v.type(), Val::Type::Int);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), -42);
        EXPECT_EQ(v.toInt64(), -42);
        EXPECT_EQ(v.toDouble(), -42.0);
        EXPECT_EQ(v.toString(), "-42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
    {
        Val v(0);

        EXPECT_EQ(v.type(), Val::Type::Int);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "0");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
}

TEST_F(Global_ValTests, Val_Int64)
{
    //! GIVEN Value as int

    {
        Val v(int64_t(42));

        EXPECT_EQ(v.type(), Val::Type::Int64);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 42);
        EXPECT_EQ(v.toInt64(), 42);
        EXPECT_EQ(v.toDouble(), 42.0);
        EXPECT_EQ(v.toString(), "42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
    {
        Val v(int64_t(-42));

        EXPECT_EQ(v.type(), Val::Type::Int64);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), -42);
        EXPECT_EQ(v.toInt64(), -42);
        EXPECT_EQ(v.toDouble(), -42.0);
        EXPECT_EQ(v.toString(), "-42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
    {
        Val v(int64_t(0));

        EXPECT_EQ(v.type(), Val::Type::Int64);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "0");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
}

TEST_F(Global_ValTests, Val_Double)
{
    //! GIVEN Value as double

    {
        Val v(22.12);

        EXPECT_EQ(v.type(), Val::Type::Double);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 22);
        EXPECT_EQ(v.toInt64(), 22);
        EXPECT_EQ(v.toDouble(), 22.12);
        EXPECT_EQ(v.toString(), "22.12");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
    {
        Val v(-22.12);

        EXPECT_EQ(v.type(), Val::Type::Double);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), -22);
        EXPECT_EQ(v.toInt64(), -22);
        EXPECT_EQ(v.toDouble(), -22.12);
        EXPECT_EQ(v.toString(), "-22.12");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
    {
        Val v(0.0);

        EXPECT_EQ(v.type(), Val::Type::Double);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "0");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v(42.0);

        EXPECT_EQ(v.type(), Val::Type::Double);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 42);
        EXPECT_EQ(v.toInt64(), 42);
        EXPECT_EQ(v.toDouble(), 42.0);
        EXPECT_EQ(v.toString(), "42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
}

TEST_F(Global_ValTests, Val_String)
{
    //! GIVEN Value as string

    {
        Val v("hello");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "hello");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("true");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 1);
        EXPECT_EQ(v.toInt64(), 1);
        EXPECT_EQ(v.toDouble(), 1.0);
        EXPECT_EQ(v.toString(), "true");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("false");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "false");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("42");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 42);
        EXPECT_EQ(v.toInt64(), 42);
        EXPECT_EQ(v.toDouble(), 42.0);
        EXPECT_EQ(v.toString(), "42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("-42");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), -42);
        EXPECT_EQ(v.toInt64(), -42);
        EXPECT_EQ(v.toDouble(), -42.0);
        EXPECT_EQ(v.toString(), "-42");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("0");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "0");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("22.12");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 22);
        EXPECT_EQ(v.toInt64(), 22);
        EXPECT_EQ(v.toDouble(), 22.12);
        EXPECT_EQ(v.toString(), "22.12");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("-22.12");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), -22);
        EXPECT_EQ(v.toInt64(), -22);
        EXPECT_EQ(v.toDouble(), -22.12);
        EXPECT_EQ(v.toString(), "-22.12");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("0.0");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "0.0");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("[1, 2, 3]");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "[1, 2, 3]");
        EXPECT_EQ(v.toList(), ValList({ Val(1), Val(2), Val(3) }));
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("[]");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "[]");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        Val v("{a: 1, b: 2, c: 3}");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "{a: 1, b: 2, c: 3}");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap({ { "a", Val(1) }, { "b", Val(2) }, { "c", Val(3) } }));
    }

    {
        Val v("{}");

        EXPECT_EQ(v.type(), Val::Type::String);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "{}");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
    }
}

TEST_F(Global_ValTests, Val_List)
{
    //! GIVEN Value as list

    {
        ValList list{ Val(1), Val(2), Val(3) };
        Val v(list);

        EXPECT_EQ(v.type(), Val::Type::List);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "[1, 2, 3]");
        EXPECT_EQ(v.toList(), list);
        EXPECT_EQ(v.toMap(), ValMap());
    }

    {
        ValList list{};
        Val v(list);

        EXPECT_EQ(v.type(), Val::Type::List);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "[]");
        EXPECT_EQ(v.toList(), list);
        EXPECT_EQ(v.toMap(), ValMap());
    }
}

TEST_F(Global_ValTests, Val_Map)
{
    //! GIVEN Value as map

    {
        ValMap map{ { "a", Val(1) }, { "b", Val(2) }, { "c", Val(3) } };
        Val v(map);

        EXPECT_EQ(v.type(), Val::Type::Map);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "{\"a\": 1, \"b\": 2, \"c\": 3}");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), map);
    }

    {
        ValMap map{};
        Val v(map);

        EXPECT_EQ(v.type(), Val::Type::Map);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), false);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "{}");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), map);
    }
}

#ifndef NO_QT_SUPPORT
TEST_F(Global_ValTests, Val_Color)
{
    //! GIVEN Value as color

    {
        Val v(QColor("#800000"));

        EXPECT_EQ(v.type(), Val::Type::Color);
        EXPECT_EQ(v.isNull(), false);
        EXPECT_EQ(v.toBool(), true);
        EXPECT_EQ(v.toInt(), 0);
        EXPECT_EQ(v.toInt64(), 0);
        EXPECT_EQ(v.toDouble(), 0.0);
        EXPECT_EQ(v.toString(), "#800000");
        EXPECT_EQ(v.toList(), ValList());
        EXPECT_EQ(v.toMap(), ValMap());
        EXPECT_EQ(v.toQColor(), QColor("#800000"));
    }
}
#endif

TEST_F(Global_ValTests, ValJson_RoundTrip)
{
    //! GIVEN List and map values whose text contains separators or looks like another type

    auto expectList = [](const ValList& list, const std::string& text) {
        std::string written;
        ASSERT_TRUE(ValJson::toString(list, written));
        EXPECT_EQ(written, text);
        ValList parsed;
        ASSERT_TRUE(ValJson::fromString(written, parsed));
        EXPECT_EQ(parsed, list);
    };

    auto expectMap = [](const ValMap& map, const std::string& text) {
        std::string written;
        ASSERT_TRUE(ValJson::toString(map, written));
        EXPECT_EQ(written, text);
        ValMap parsed;
        ASSERT_TRUE(ValJson::fromString(written, parsed));
        EXPECT_EQ(parsed, map);
    };

    expectList(ValList{ Val("a, b") }, "[\"a, b\"]");
    expectList(ValList{ Val("1") }, "[\"1\"]");
    expectList(ValList{ Val("true") }, "[\"true\"]");
    expectList(ValList{ Val("false") }, "[\"false\"]");
    expectList(ValList{ Val("22.12") }, "[\"22.12\"]");
    expectList(ValList{ Val("  x  ") }, "[\"  x  \"]");
    expectList(ValList{ Val("a:b") }, "[\"a:b\"]");
    expectList(ValList{ Val("a[b]") }, "[\"a[b]\"]");
    expectList(ValList{ Val("a{b}") }, "[\"a{b}\"]");
    expectList(ValList{ Val("a\"b\\c") }, "[\"a\\\"b\\\\c\"]");
    expectList(ValList{ Val("a\nb") }, "[\"a\\nb\"]");
    expectList(ValList{ Val("") }, "[\"\"]");
    expectList(ValList{ Val(1), Val("1"), Val(true), Val("true") }, "[1, \"1\", true, \"true\"]");
    expectList(ValList{ Val(ValList{ Val("a, b") }), Val("x:y") }, "[[\"a, b\"], \"x:y\"]");

    expectMap(ValMap{ { "k", Val("x, y") } }, "{\"k\": \"x, y\"}");
    expectMap(ValMap{ { "a, b", Val(1) } }, "{\"a, b\": 1}");
    expectMap(ValMap{ { "a:b", Val(1) } }, "{\"a:b\": 1}");
    expectMap(ValMap{ { "  k  ", Val(1) } }, "{\"  k  \": 1}");
    expectMap(ValMap{ { "a[b]", Val("{c}") } }, "{\"a[b]\": \"{c}\"}");
    expectMap(ValMap{ { "k", Val(ValList{ Val("a, b"), Val("1") }) } }, "{\"k\": [\"a, b\", \"1\"]}");
}

TEST_F(Global_ValTests, Val_To_From_Type)
{
    //! GIVEN Value of a supported type

    {
        bool value = true;
        Val v = Val::fromType(value);
        auto opt = v.toType<bool>();
        EXPECT_EQ(v.type(), Val::Type::Bool);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        bool value = false;
        Val v = Val::fromType(value);
        auto opt = v.toType<bool>();
        EXPECT_EQ(v.type(), Val::Type::Bool);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        int value = 42;
        Val v = Val::fromType(value);
        auto opt = v.toType<int>();
        EXPECT_EQ(v.type(), Val::Type::Int);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        int64_t value = 5000000000;
        Val v = Val::fromType(value);
        auto opt = v.toType<int64_t>();
        EXPECT_EQ(v.type(), Val::Type::Int64);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        double value = 22.12;
        Val v = Val::fromType(value);
        auto opt = v.toType<double>();
        EXPECT_EQ(v.type(), Val::Type::Double);
        ASSERT_TRUE(opt.has_value());
        EXPECT_DOUBLE_EQ(*opt, value);
    }

    {
        integer_t value = 42;
        Val v = Val::fromType(value);
        auto opt = v.toType<integer_t>();
        EXPECT_EQ(v.type(), Val::Type::Int);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        real_t value = 22.12;
        Val v = Val::fromType(value);
        auto opt = v.toType<real_t>();
        EXPECT_EQ(v.type(), Val::Type::Double);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        std::string value = "hello";
        Val v = Val::fromType(value);
        auto opt = v.toType<std::string>();
        EXPECT_EQ(v.type(), Val::Type::String);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        io::path_t value = "path/to/file name.ext";
        Val v = Val::fromType(value);
        auto opt = v.toType<io::path_t>();
        EXPECT_EQ(v.type(), Val::Type::String);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        ValList value{ Val(1), Val(2), Val(3) };
        Val v = Val::fromType(value);
        auto opt = v.toType<ValList>();
        EXPECT_EQ(v.type(), Val::Type::List);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        ValMap value{ { "a", Val(1) }, { "b", Val(2) } };
        Val v = Val::fromType(value);
        auto opt = v.toType<ValMap>();
        EXPECT_EQ(v.type(), Val::Type::Map);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        std::vector<int> value{ 1, 2, 3 };
        Val v = Val::fromType(value);
        auto opt = v.toType<std::vector<int> >();
        EXPECT_EQ(v.type(), Val::Type::List);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }

    {
        std::map<std::string, int> value{ { "a", 1 }, { "b", 2 } };
        Val v = Val::fromType(value);
        auto opt = v.toType<std::map<std::string, int> >();
        EXPECT_EQ(v.type(), Val::Type::Map);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(*opt, value);
    }
}
