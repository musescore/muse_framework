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

 #include <string>

#include "rcommand/typedcommand.h"
#include "rcommand/enumcontract.h"

using namespace muse;
using namespace muse::rcommand;

class RCommand_TypedCommandTests : public ::testing::Test
{
public:
};

enum class TestEnum {
    Value1,
    Value2,
    Value3,
};

struct TestEnumContract {
    static constexpr std::string_view key = "e_val";
    static constexpr TestEnum def = TestEnum::Value1;
    static constexpr Table<TestEnum, 3> table = { {
        { "value1", TestEnum::Value1, "value 1" },
        { "value2", TestEnum::Value2, "value 2" },
        { "value3", TestEnum::Value3, "value 3" },
    } };
};

namespace muse::rcommand {
DECLARE_CONTRACT(TestEnum, TestEnumContract);
}

struct TestWithInputCommand {
    static const inline auto command = muse::rcommand::Command("command://rcommand/test");
    static const inline auto title = MnemonicString("action", "Test command");
    static const inline auto description = TranslatableString("action", "Test command");

    bool bVal = false;
    int iVal = 0;
    double dVal = 0.0;
    std::string sVal = "";
    TestEnum eVal = TestEnum::Value1;

    static constexpr auto input = std::tuple {
        Field("b_val", &TestWithInputCommand::bVal, "Boolean value"),
        Field("i_val", &TestWithInputCommand::iVal, "Integer value", -100, 100),
        Field("d_val", &TestWithInputCommand::dVal, "Double value", FieldType::Optional),
        Field("s_val", &TestWithInputCommand::sVal, "String value", FieldType::Optional),
        Field("e_val", &TestWithInputCommand::eVal, "Enum value", FieldType::Optional),
    };
};

struct TestWithOutCommand {
    static const inline auto command = muse::rcommand::Command("command://rcommand/test");
    static const inline auto title = MnemonicString("action", "Test command");
    static const inline auto description = TranslatableString("action", "Test command");

    struct Output {
        bool bVal = false;
        int iVal = 0;
        double dVal = 0.0;
        std::string sVal = "";
        TestEnum eVal = TestEnum::Value1;
    };

    using _O = TestWithOutCommand::Output;
    static constexpr auto output = std::tuple {
        Field("b_val", &_O::bVal, "Boolean value"),
        Field("i_val", &_O::iVal, "Integer value", -100, 100),
        Field("d_val", &_O::dVal, "Double value", FieldType::Optional),
        Field("s_val", &_O::sVal, "String value", FieldType::Optional),
        Field("e_val", &_O::eVal, "Enum value", FieldType::Optional),
    };
};

TEST_F(RCommand_TypedCommandTests, TypedCommand_Convert)
{
    //! GIVEN Valid test command
    const auto original = TestWithInputCommand{ .bVal = true, .iVal = 1, .dVal = 1.0, .sVal = "test", .eVal = TestEnum::Value2 };

    //! DO convert to params
    Params params = toInputParams(original);

    //! CHECK params
    EXPECT_EQ(params.size(), 5);
    EXPECT_EQ(params["b_val"].toBool(), true);
    EXPECT_EQ(params["i_val"].toInt(), 1);
    EXPECT_EQ(params["d_val"].toDouble(), 1.0);
    EXPECT_EQ(params["s_val"].toString(), "test");
    EXPECT_EQ(params["e_val"].toString(), "value2");

    //! DO convert from params
    TestWithInputCommand result;
    fromInputParams(params, result);

    //! CHECK result
    EXPECT_EQ(result.bVal, true);
    EXPECT_EQ(result.iVal, 1);
    EXPECT_EQ(result.dVal, 1.0);
    EXPECT_EQ(result.sVal, "test");
    EXPECT_EQ(result.eVal, TestEnum::Value2);
}

TEST_F(RCommand_TypedCommandTests, TypedCommand_Convert_Optional)
{
    //! GIVEN Params
    Params params = {
        { "b_val", Val(true) },
        { "i_val", Val(1) },
        { "d_val", Val(2.0) } // optional
        //{ "s_val", Val("test") } // optional
        //{ "e_val", Val("value2") } // optional
    };

    //! DO convert from params
    TestWithInputCommand result;
    fromInputParams(params, result);

    //! CHECK result
    EXPECT_EQ(result.bVal, true);
    EXPECT_EQ(result.iVal, 1);
    EXPECT_EQ(result.dVal, 2.0);
    EXPECT_EQ(result.sVal, ""); // default value
    EXPECT_EQ(result.eVal, TestEnum::Value1); // default value
}

TEST_F(RCommand_TypedCommandTests, TypedCommand_InputSchema)
{
    //! DO make input schema
    const InputSchema schema = inputSchema<TestWithInputCommand>();

    //! CHECK schema
    EXPECT_EQ(schema.args.size(), 5);

    EXPECT_EQ(schema.args.at("b_val").type, DataType::Bool);
    EXPECT_EQ(schema.args.at("b_val").description, String::fromUtf8("Boolean value"));

    EXPECT_EQ(schema.args.at("i_val").type, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").description, String::fromUtf8("Integer value"));
    EXPECT_EQ(schema.args.at("i_val").min.toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max.toInt(), 100);

    EXPECT_EQ(schema.args.at("d_val").type, DataType::Double);
    EXPECT_EQ(schema.args.at("d_val").description, String::fromUtf8("Double value"));

    EXPECT_EQ(schema.args.at("s_val").type, DataType::String);
    EXPECT_EQ(schema.args.at("s_val").description, String::fromUtf8("String value"));

    EXPECT_EQ(schema.args.at("e_val").type, DataType::String);
    EXPECT_EQ(schema.args.at("e_val").description, String::fromUtf8("Enum value"));
}

TEST_F(RCommand_TypedCommandTests, TypedCommand_OutputSchema)
{
    //! DO make output schema
    const OutputSchema schema = outputSchema<TestWithOutCommand>();

    //! CHECK schema
    EXPECT_EQ(schema.args.size(), 5);

    EXPECT_EQ(schema.args.at("b_val").type, DataType::Bool);
    EXPECT_EQ(schema.args.at("b_val").description, String::fromUtf8("Boolean value"));

    EXPECT_EQ(schema.args.at("i_val").type, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").description, String::fromUtf8("Integer value"));
    EXPECT_EQ(schema.args.at("i_val").min.toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max.toInt(), 100);

    EXPECT_EQ(schema.args.at("d_val").type, DataType::Double);
    EXPECT_EQ(schema.args.at("d_val").description, String::fromUtf8("Double value"));

    EXPECT_EQ(schema.args.at("s_val").type, DataType::String);
    EXPECT_EQ(schema.args.at("s_val").description, String::fromUtf8("String value"));

    EXPECT_EQ(schema.args.at("e_val").type, DataType::String);
    EXPECT_EQ(schema.args.at("e_val").description, String::fromUtf8("Enum value"));
}
