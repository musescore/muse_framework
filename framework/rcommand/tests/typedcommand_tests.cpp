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

struct TestCommand {
    static const inline auto id = muse::rcommand::Command("command://rcommand/test");
    static const inline auto title = MnemonicString("action", "Test command");
    static const inline auto description = TranslatableString("action", "Test command");

    bool bVal = false;
    int iVal = 0;
    unsigned int uVal = 0;
    int64_t i64Val = 0;
    uint64_t u64Val = 0;
    float fVal = 0.0f;
    double dVal = 0.0;
    std::string sVal = "";
    TestEnum eVal = TestEnum::Value1;

    struct Output {
        bool bVal = false;
        int iVal = 0;
        unsigned int uVal = 0;
        int64_t i64Val = 0;
        uint64_t u64Val = 0;
        float fVal = 0.0f;
        double dVal = 0.0;
        std::string sVal = "";
        TestEnum eVal = TestEnum::Value1;
    };

    static constexpr auto input = std::tuple {
        Field("b_val", &TestCommand::bVal, "Boolean value"),
        Field("i_val", &TestCommand::iVal, "Integer value", -100, 100),
        Field("u_val", &TestCommand::uVal, "Unsigned integer value", FieldType::Optional),
        Field("i64_val", &TestCommand::i64Val, "64-bit integer value", FieldType::Optional),
        Field("u64_val", &TestCommand::u64Val, "64-bit unsigned integer value", FieldType::Optional),
        Field("f_val", &TestCommand::fVal, "Float value", 0.0f, 100.0f, FieldType::Optional),
        Field("d_val", &TestCommand::dVal, "Double value", FieldType::Optional),
        Field("s_val", &TestCommand::sVal, "String value", FieldType::Optional),
        Field("e_val", &TestCommand::eVal, "Enum value", FieldType::Optional),
    };

    using O = TestCommand::Output;
    static constexpr auto output = std::tuple {
        Field("b_val", &O::bVal, "Boolean value"),
        Field("i_val", &O::iVal, "Integer value", -100, 100),
        Field("u_val", &O::uVal, "Unsigned integer value", FieldType::Optional),
        Field("i64_val", &O::i64Val, "64-bit integer value", FieldType::Optional),
        Field("u64_val", &O::u64Val, "64-bit unsigned integer value", FieldType::Optional),
        Field("f_val", &O::fVal, "Float value", 0.0f, 100.0f, FieldType::Optional),
        Field("d_val", &O::dVal, "Double value", FieldType::Optional),
        Field("s_val", &O::sVal, "String value", FieldType::Optional),
        Field("e_val", &O::eVal, "Enum value", FieldType::Optional),
    };
};

//! A command without parameters that declares its decoration and its availabilities
struct TestInternalCommand {
    static const inline auto id = muse::rcommand::Command("command://rcommand/test-internal");
    static const inline auto title = MnemonicString("action", "Internal test command");
    static const inline auto description = TranslatableString("action", "Dispatch only");
    static const inline auto decoration = Decoration(Checkable::Yes);
    static const inline Availabilities availabilities = Availability::Dispatch;
};

TEST_F(RCommand_TypedCommandTests, TypedCommand_Convert)
{
    //! GIVEN Valid test command
    const auto original = TestCommand{
        .bVal = true,
        .iVal = -1,
        .uVal = 1,
        .i64Val = -1LL,
        .u64Val = 1ULL,
        .fVal = 1.0f,
        .dVal = 1.0,
        .sVal = "test",
        .eVal = TestEnum::Value2
    };

    //! DO convert to params
    Params params = toInputParams(original);

    //! CHECK params
    EXPECT_EQ(params.size(), 9);
    EXPECT_EQ(params["b_val"].toBool(), true);
    EXPECT_EQ(params["i_val"].toInt(), -1);
    EXPECT_EQ(params["u_val"].toInt(), 1);
    EXPECT_EQ(params["i64_val"].toInt64(), -1LL);
    EXPECT_EQ(params["u64_val"].toInt64(), 1ULL);
    EXPECT_EQ(params["f_val"].toDouble(), 1.0);
    EXPECT_EQ(params["d_val"].toDouble(), 1.0);
    EXPECT_EQ(params["s_val"].toString(), "test");
    EXPECT_EQ(params["e_val"].toString(), "value2");

    //! DO convert from params
    TestCommand result;
    EXPECT_TRUE(fromInputParams(params, result));

    //! CHECK result
    EXPECT_EQ(result.bVal, true);
    EXPECT_EQ(result.iVal, -1);
    EXPECT_EQ(result.uVal, 1);
    EXPECT_EQ(result.i64Val, -1LL);
    EXPECT_EQ(result.u64Val, 1ULL);
    EXPECT_EQ(result.fVal, 1.0f);
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
    };

    //! DO convert from params
    TestCommand result;
    EXPECT_TRUE(fromInputParams(params, result));

    //! CHECK result
    EXPECT_EQ(result.bVal, true);
    EXPECT_EQ(result.iVal, 1);
    EXPECT_EQ(result.uVal, 0);
    EXPECT_EQ(result.i64Val, 0);
    EXPECT_EQ(result.u64Val, 0);
    EXPECT_EQ(result.fVal, 0.0f);
    EXPECT_EQ(result.dVal, 2.0);
    EXPECT_EQ(result.sVal, "");
    EXPECT_EQ(result.eVal, TestEnum::Value1);
}

TEST_F(RCommand_TypedCommandTests, TypedCommand_InputSchema)
{
    //! DO make input schema
    const InputSchema schema = inputSchema<TestCommand>();

    //! CHECK schema
    EXPECT_EQ(schema.args.size(), 9);

    EXPECT_EQ(schema.args.at("b_val").type, DataType::Bool);
    EXPECT_EQ(schema.args.at("b_val").description, String::fromUtf8("Boolean value"));

    EXPECT_EQ(schema.args.at("i_val").type, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").description, String::fromUtf8("Integer value"));
    EXPECT_EQ(schema.args.at("i_val").min.toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max.toInt(), 100);

    EXPECT_EQ(schema.args.at("u_val").type, DataType::Int);
    EXPECT_EQ(schema.args.at("u_val").description, String::fromUtf8("Unsigned integer value"));

    EXPECT_EQ(schema.args.at("i64_val").type, DataType::Int64);
    EXPECT_EQ(schema.args.at("i64_val").description, String::fromUtf8("64-bit integer value"));

    EXPECT_EQ(schema.args.at("u64_val").type, DataType::Int64);
    EXPECT_EQ(schema.args.at("u64_val").description, String::fromUtf8("64-bit unsigned integer value"));

    EXPECT_EQ(schema.args.at("f_val").type, DataType::Float);
    EXPECT_EQ(schema.args.at("f_val").description, String::fromUtf8("Float value"));
    EXPECT_EQ(schema.args.at("f_val").min.toFloat(), 0.0f);
    EXPECT_EQ(schema.args.at("f_val").max.toFloat(), 100.0f);

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
    const OutputSchema schema = outputSchema<TestCommand>();

    //! CHECK schema
    EXPECT_EQ(schema.args.size(), 9);

    EXPECT_EQ(schema.args.at("b_val").type, DataType::Bool);
    EXPECT_EQ(schema.args.at("b_val").description, String::fromUtf8("Boolean value"));

    EXPECT_EQ(schema.args.at("i_val").type, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").description, String::fromUtf8("Integer value"));
    EXPECT_EQ(schema.args.at("i_val").min.toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max.toInt(), 100);

    EXPECT_EQ(schema.args.at("u_val").type, DataType::Int);
    EXPECT_EQ(schema.args.at("u_val").description, String::fromUtf8("Unsigned integer value"));

    EXPECT_EQ(schema.args.at("i64_val").type, DataType::Int64);
    EXPECT_EQ(schema.args.at("i64_val").description, String::fromUtf8("64-bit integer value"));

    EXPECT_EQ(schema.args.at("u64_val").type, DataType::Int64);
    EXPECT_EQ(schema.args.at("u64_val").description, String::fromUtf8("64-bit unsigned integer value"));

    EXPECT_EQ(schema.args.at("f_val").type, DataType::Float);
    EXPECT_EQ(schema.args.at("f_val").description, String::fromUtf8("Float value"));
    EXPECT_EQ(schema.args.at("f_val").min.toFloat(), 0.0f);
    EXPECT_EQ(schema.args.at("f_val").max.toFloat(), 100.0f);

    EXPECT_EQ(schema.args.at("d_val").type, DataType::Double);
    EXPECT_EQ(schema.args.at("d_val").description, String::fromUtf8("Double value"));

    EXPECT_EQ(schema.args.at("s_val").type, DataType::String);
    EXPECT_EQ(schema.args.at("s_val").description, String::fromUtf8("String value"));

    EXPECT_EQ(schema.args.at("e_val").type, DataType::String);
    EXPECT_EQ(schema.args.at("e_val").description, String::fromUtf8("Enum value"));
}

TEST_F(RCommand_TypedCommandTests, TypedCommand_Concepts)
{
    static_assert(TypedCommand<TestCommand>);
    static_assert(HasInput<TestCommand>);
    static_assert(HasOutput<TestCommand>);

    static_assert(TypedCommand<TestInternalCommand>);
    static_assert(!HasInput<TestInternalCommand>);
    static_assert(!HasOutput<TestInternalCommand>);
    static_assert(HasDecoration<TestInternalCommand>);
    static_assert(HasAvailabilities<TestInternalCommand>);

    static_assert(!TypedCommand<Command>);
}

TEST_F(RCommand_TypedCommandTests, MakeCommandInfo_TakesEverythingFromTheStruct)
{
    //! DO make the info of a command with input and output
    const CommandInfo info = makeCommandInfo<TestCommand>();

    //! CHECK
    EXPECT_EQ(info.command, TestCommand::id);
    EXPECT_EQ(info.inputSchema.args.size(), 9);
    EXPECT_EQ(info.outputSchema.args.size(), 9);
    EXPECT_EQ(info.decoration.checkable, Checkable::No);

    //! DO make the info of a command without parameters
    const CommandInfo internal = makeCommandInfo<TestInternalCommand>();

    //! CHECK decoration and availabilities come from the struct
    EXPECT_EQ(internal.command, TestInternalCommand::id);
    EXPECT_TRUE(internal.inputSchema.args.empty());
    EXPECT_TRUE(internal.outputSchema.args.empty());
    EXPECT_EQ(internal.decoration.checkable, Checkable::Yes);
    EXPECT_TRUE(internal.availabilities.testFlag(Availability::Dispatch));
    EXPECT_FALSE(internal.availabilities.testFlag(Availability::MCP));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_AcceptsValuesEncodedAsStrings)
{
    //! GIVEN Params with every value supplied as a string, as a script or the API may send them
    const Params params = {
        { "b_val", Val("true") },
        { "i_val", Val("7") },
        { "d_val", Val("2.5") },
        { "e_val", Val("value3") },
    };

    //! DO convert from params
    TestCommand result;
    ASSERT_TRUE(fromInputParams(params, result));

    //! CHECK
    EXPECT_EQ(result.bVal, true);
    EXPECT_EQ(result.iVal, 7);
    EXPECT_EQ(result.dVal, 2.5);
    EXPECT_EQ(result.eVal, TestEnum::Value3);
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_AcceptsTheParamsOfACommandQuery)
{
    //! GIVEN A CommandQuery parsed from a URI: true/false arrive as bools, every other value as a string
    const CommandQuery query("command://rcommand/test?b_val=true&i_val=7&d_val=2.5&s_val=hello&e_val=value3");

    //! DO convert from its params
    TestCommand result;
    ASSERT_TRUE(fromInputParams(query.params(), result));

    //! CHECK
    EXPECT_EQ(result.bVal, true);
    EXPECT_EQ(result.iVal, 7);
    EXPECT_EQ(result.dVal, 2.5);
    EXPECT_EQ(result.sVal, "hello");
    EXPECT_EQ(result.eVal, TestEnum::Value3);
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsMissingRequiredParameter)
{
    //! GIVEN Params without the required "i_val"
    const Params params = {
        { "b_val", Val(true) },
    };

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsWrongType)
{
    //! GIVEN Valid params with a non-numeric value for an integer parameter
    Params params = toInputParams(TestCommand {});
    params.set("i_val", Val("not a number"));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsWrongTypeOfOptionalParameter)
{
    //! GIVEN Valid params with a non-numeric value for an optional double parameter
    Params params = toInputParams(TestCommand {});
    params.set("d_val", Val("not a number"));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected: optional means it may be absent, not that it may be invalid
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsUnknownEnumValue)
{
    //! GIVEN Valid params with a value outside the enum contract
    Params params = toInputParams(TestCommand {});
    params.set("e_val", Val("value4"));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, EnumContract_RoundTrip)
{
    EXPECT_EQ(Contract<TestEnum>::str_val(TestEnum::Value2), "value2");
    EXPECT_EQ(Contract<TestEnum>::enum_val("value3"), TestEnum::Value3);
    EXPECT_FALSE(Contract<TestEnum>::enum_val("value4").has_value());
}
