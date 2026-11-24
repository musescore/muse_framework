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

 #include <array>
 #include <string>
 #include <type_traits>

#include "rcommand/typedcommand.h"
#include "rcommand/enumcontract.h"

#include "mocks/commanddispatchermock.h"

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
    static constexpr EnumTable<TestEnum, 3> table = { {
        { "value1", TestEnum::Value1, "value 1" },
        { "value2", TestEnum::Value2, "value 2" },
        { "value3", TestEnum::Value3, "value 3" },
    } };
};

namespace muse::rcommand {
DECLARE_ENUM_CONTRACT(TestEnumContract);
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

    static const inline auto input = std::tuple {
        Field("b_val", &TestCommand::bVal, "Boolean value"),
        Field("i_val", &TestCommand::iVal, "Integer value", -100, 100),
        Field("u_val", &TestCommand::uVal, "Unsigned integer value", Presence::Optional),
        Field("i64_val", &TestCommand::i64Val, "64-bit integer value", Presence::Optional),
        Field("u64_val", &TestCommand::u64Val, "64-bit unsigned integer value", Presence::Optional),
        Field("f_val", &TestCommand::fVal, "Float value", 0.0f, 100.0f, Presence::Optional),
        Field("d_val", &TestCommand::dVal, "Double value", Presence::Optional),
        Field("s_val", &TestCommand::sVal, "String value", Presence::Optional),
        Field("e_val", &TestCommand::eVal, "Enum value", Presence::Optional),
    };

    using O = TestCommand::Output;
    static const inline auto output = std::tuple {
        Field("b_val", &O::bVal, "Boolean value"),
        Field("i_val", &O::iVal, "Integer value", -100, 100),
        Field("u_val", &O::uVal, "Unsigned integer value", Presence::Optional),
        Field("i64_val", &O::i64Val, "64-bit integer value", Presence::Optional),
        Field("u64_val", &O::u64Val, "64-bit unsigned integer value", Presence::Optional),
        Field("f_val", &O::fVal, "Float value", 0.0f, 100.0f, Presence::Optional),
        Field("d_val", &O::dVal, "Double value", Presence::Optional),
        Field("s_val", &O::sVal, "String value", Presence::Optional),
        Field("e_val", &O::eVal, "Enum value", Presence::Optional),
    };
};

//! A command that delegates to another command, fixing some of its input values
struct TestAliasCommand {
    static const inline auto id = muse::rcommand::Command("command://rcommand/test-alias");
    static const inline auto title = MnemonicString("action", "Alias test command");
    static const inline auto description = TranslatableString("action", "Delegates to the test command");

    static const inline auto redirect = TestCommand {
        .bVal = true,
        .iVal = 1,
    };
};

//! A command whose first two fields every sender has to set, and a third one it may leave out
struct TestRequiredCommand {
    static const inline auto id = muse::rcommand::Command("command://rcommand/test-required");
    static const inline auto title = MnemonicString("action", "Required test command");
    static const inline auto description = TranslatableString("action", "Required test command");

    Required<int> iVal;
    Required<std::string> sVal;
    double dVal = 0.0;

    static const inline auto input = std::tuple {
        Field("i_val", &TestRequiredCommand::iVal, "Integer value", -100, 100),
        Field("s_val", &TestRequiredCommand::sVal, "String value"),
        Field("d_val", &TestRequiredCommand::dVal, "Double value", Presence::Optional),
    };
};

//! A command with constructors: not an aggregate, so its members cannot be counted
struct TestConstructedCommand {
    static const inline auto id = muse::rcommand::Command("command://rcommand/test-constructed");
    static const inline auto title = MnemonicString("action", "Constructed test command");
    static const inline auto description = TranslatableString("action", "Constructed test command");

    int iVal = 0;
    int notInTheScheme = 0;

    static const inline auto input = std::tuple {
        Field("i_val", &TestConstructedCommand::iVal, "Integer value"),
    };

    TestConstructedCommand() = default;
    TestConstructedCommand(int v)
        : iVal(v) {}
};

//! A command without parameters that declares its decoration and its availabilities
struct TestInternalCommand {
    static const inline auto id = muse::rcommand::Command("command://rcommand/test-internal");
    static const inline auto title = MnemonicString("action", "Internal test command");
    static const inline auto description = TranslatableString("action", "Dispatch only");
    static const inline auto decoration = Decoration(Checkable::Yes);
    static const inline Availabilities availabilities = Availability::Internal;
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

    EXPECT_EQ(schema.args.at("b_val").dataType, DataType::Bool);
    EXPECT_EQ(schema.args.at("b_val").description.str.toStdString(), std::string("Boolean value"));

    EXPECT_EQ(schema.args.at("i_val").dataType, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").description.str.toStdString(), std::string("Integer value"));
    EXPECT_EQ(schema.args.at("i_val").min->toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max->toInt(), 100);

    EXPECT_EQ(schema.args.at("u_val").dataType, DataType::Int);
    EXPECT_EQ(schema.args.at("u_val").description.str.toStdString(), std::string("Unsigned integer value"));

    EXPECT_EQ(schema.args.at("i64_val").dataType, DataType::Int64);
    EXPECT_EQ(schema.args.at("i64_val").description.str.toStdString(), std::string("64-bit integer value"));

    EXPECT_EQ(schema.args.at("u64_val").dataType, DataType::Int64);
    EXPECT_EQ(schema.args.at("u64_val").description.str.toStdString(), std::string("64-bit unsigned integer value"));

    EXPECT_EQ(schema.args.at("f_val").dataType, DataType::Float);
    EXPECT_EQ(schema.args.at("f_val").description.str.toStdString(), std::string("Float value"));
    EXPECT_EQ(schema.args.at("f_val").min->toFloat(), 0.0f);
    EXPECT_EQ(schema.args.at("f_val").max->toFloat(), 100.0f);

    EXPECT_EQ(schema.args.at("d_val").dataType, DataType::Double);
    EXPECT_EQ(schema.args.at("d_val").description.str.toStdString(), std::string("Double value"));

    EXPECT_EQ(schema.args.at("s_val").dataType, DataType::String);
    EXPECT_EQ(schema.args.at("s_val").description.str.toStdString(), std::string("String value"));

    EXPECT_EQ(schema.args.at("e_val").dataType, DataType::Enum);
    EXPECT_EQ(schema.args.at("e_val").description.str.toStdString(), std::string("Enum value"));
}

TEST_F(RCommand_TypedCommandTests, TypedCommand_OutputSchema)
{
    //! DO make output schema
    const OutputSchema schema = outputSchema<TestCommand>();

    //! CHECK schema
    EXPECT_EQ(schema.args.size(), 9);

    EXPECT_EQ(schema.args.at("b_val").dataType, DataType::Bool);
    EXPECT_EQ(schema.args.at("b_val").description.str.toStdString(), std::string("Boolean value"));

    EXPECT_EQ(schema.args.at("i_val").dataType, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").description.str.toStdString(), std::string("Integer value"));
    EXPECT_EQ(schema.args.at("i_val").min->toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max->toInt(), 100);

    EXPECT_EQ(schema.args.at("u_val").dataType, DataType::Int);
    EXPECT_EQ(schema.args.at("u_val").description.str.toStdString(), std::string("Unsigned integer value"));

    EXPECT_EQ(schema.args.at("i64_val").dataType, DataType::Int64);
    EXPECT_EQ(schema.args.at("i64_val").description.str.toStdString(), std::string("64-bit integer value"));

    EXPECT_EQ(schema.args.at("u64_val").dataType, DataType::Int64);
    EXPECT_EQ(schema.args.at("u64_val").description.str.toStdString(), std::string("64-bit unsigned integer value"));

    EXPECT_EQ(schema.args.at("f_val").dataType, DataType::Float);
    EXPECT_EQ(schema.args.at("f_val").description.str.toStdString(), std::string("Float value"));
    EXPECT_EQ(schema.args.at("f_val").min->toFloat(), 0.0f);
    EXPECT_EQ(schema.args.at("f_val").max->toFloat(), 100.0f);

    EXPECT_EQ(schema.args.at("d_val").dataType, DataType::Double);
    EXPECT_EQ(schema.args.at("d_val").description.str.toStdString(), std::string("Double value"));

    EXPECT_EQ(schema.args.at("s_val").dataType, DataType::String);
    EXPECT_EQ(schema.args.at("s_val").description.str.toStdString(), std::string("String value"));

    EXPECT_EQ(schema.args.at("e_val").dataType, DataType::Enum);
    EXPECT_EQ(schema.args.at("e_val").description.str.toStdString(), std::string("Enum value"));
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

    //! redirect
    static_assert(HasRedirect<TestAliasCommand>);
    static_assert(!HasRedirect<TestCommand>);
    static_assert(!HasRedirect<TestInternalCommand>);

    static_assert(!TypedCommand<Command>);

    //! the type of a static data member is const-qualified,
    //! so the underlying command is checked through TypedCommandType
    static_assert(TypedCommandType<decltype(TestAliasCommand::redirect)>);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(TestAliasCommand::redirect)>, TestCommand>);
    static_assert(!TypedCommandType<decltype(TestAliasCommand::id)>);
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
    EXPECT_TRUE(internal.availabilities.testFlag(Availability::Internal));
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

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsValueBelowMinimum)
{
    //! GIVEN Valid params with the bounded f_val below its minimum
    Params params = toInputParams(TestCommand {});
    params.set("f_val", Val(-1.0));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsValueAboveMaximum)
{
    //! GIVEN Valid params with the bounded f_val above its maximum
    Params params = toInputParams(TestCommand {});
    params.set("f_val", Val(101.0));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsNaNInBoundedField)
{
    //! GIVEN Valid params with a NaN value for the bounded f_val,
    //! arriving as a string, since string inputs can reach float conversion
    Params params = toInputParams(TestCommand {});
    params.set("f_val", Val("nan"));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected: NaN compares false against both bounds, so it must not be accepted
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_RejectsInfinityInBoundedField)
{
    //! GIVEN Valid params with an infinity value for the bounded f_val
    Params params = toInputParams(TestCommand {});
    params.set("f_val", Val("inf"));

    //! DO convert from params
    TestCommand result;

    //! CHECK rejected
    EXPECT_FALSE(fromInputParams(params, result));
}

TEST_F(RCommand_TypedCommandTests, FromInputParams_AcceptsValuesOnTheBounds)
{
    //! GIVEN Valid params with f_val exactly on the minimum, then on the maximum
    Params params = toInputParams(TestCommand {});
    params.set("f_val", Val(0.0));

    //! DO convert from params
    TestCommand result;
    ASSERT_TRUE(fromInputParams(params, result));
    EXPECT_EQ(result.fVal, 0.0f);

    params.set("f_val", Val(100.0));
    ASSERT_TRUE(fromInputParams(params, result));
    EXPECT_EQ(result.fVal, 100.0f);
}

TEST_F(RCommand_TypedCommandTests, Required_MustBeSetBySenders)
{
    //! CHECK a sender cannot leave a Required<T> field out, but can set it from a plain value
    static_assert(std::is_aggregate_v<TestRequiredCommand>);
    static_assert(!std::is_default_constructible_v<TestRequiredCommand>);
    static_assert(!std::is_default_constructible_v<Required<int> >);
    static_assert(std::is_constructible_v<Required<int>, int>);
    static_assert(std::is_constructible_v<Required<std::string>, const char*>);

    //! GIVEN A command built the way senders do
    const TestRequiredCommand command { .iVal = 7, .sVal = "test" };

    //! CHECK the fields read as plain values
    EXPECT_EQ(command.iVal, 7);
    EXPECT_EQ(command.sVal, "test");
    EXPECT_EQ(command.dVal, 0.0);

    const int i = command.iVal;
    const std::string& s = command.sVal;
    EXPECT_EQ(i, 7);
    EXPECT_EQ(s, "test");
}

TEST_F(RCommand_TypedCommandTests, Required_IsAPlainValueInTheSchemaAndTheParams)
{
    //! DO make the input schema
    const InputSchema schema = inputSchema<TestRequiredCommand>();

    //! CHECK Required<T> fields are typed as T and required, with their bounds
    EXPECT_EQ(schema.args.size(), 3);
    EXPECT_EQ(schema.args.at("i_val").dataType, DataType::Int);
    EXPECT_EQ(schema.args.at("i_val").presence, Presence::Required);
    EXPECT_EQ(schema.args.at("i_val").min->toInt(), -100);
    EXPECT_EQ(schema.args.at("i_val").max->toInt(), 100);
    EXPECT_EQ(schema.args.at("s_val").dataType, DataType::String);
    EXPECT_EQ(schema.args.at("s_val").presence, Presence::Required);
    EXPECT_EQ(schema.args.at("d_val").presence, Presence::Optional);

    //! DO convert to params
    const Params params = toInputParams(TestRequiredCommand { .iVal = 7, .sVal = "test" });

    //! CHECK
    EXPECT_EQ(params.size(), 3);
    EXPECT_EQ(params.at("i_val").toInt(), 7);
    EXPECT_EQ(params.at("s_val").toString(), "test");
    EXPECT_EQ(params.at("d_val").toDouble(), 0.0);
}

TEST_F(RCommand_TypedCommandTests, Required_IsFilledFromParamsOnTheHandlerSide)
{
    //! GIVEN A command built the way the dispatcher does, before the params are known
    TestRequiredCommand result = internal::Construct::make<TestRequiredCommand>();

    //! DO convert from params
    const Params params = {
        { "i_val", Val("7") },
        { "s_val", Val("test") },
    };
    ASSERT_TRUE(fromInputParams(params, result));

    //! CHECK
    EXPECT_EQ(result.iVal, 7);
    EXPECT_EQ(result.sVal, "test");
    EXPECT_EQ(result.dVal, 0.0);
}

TEST_F(RCommand_TypedCommandTests, Required_IsRejectedWhenMissingOrOutOfBounds)
{
    TestRequiredCommand result = internal::Construct::make<TestRequiredCommand>();

    //! CHECK a missing Required<T> field is rejected
    EXPECT_FALSE(fromInputParams(Params { { "s_val", Val("test") } }, result));

    //! CHECK the bounds apply to the value inside
    EXPECT_FALSE(fromInputParams(Params { { "i_val", Val(101) }, { "s_val", Val("test") } }, result));
}

TEST_F(RCommand_TypedCommandTests, Dispatcher_BuildsARequiredCommandFromTheRequest)
{
    //! GIVEN A handler of a command with Required<T> fields, registered through the typed overload
    testing::NiceMock<CommandDispatcherMock> mock;
    ICommandDispatcher::CallBack registered;
    EXPECT_CALL(mock, onRequest(testing::_, TestRequiredCommand::id, testing::A<const ICommandDispatcher::CallBack&>()))
    .WillOnce(testing::SaveArg<2>(&registered));

    std::optional<TestRequiredCommand> received;
    ICommandDispatcher& dispatcher = mock;
    dispatcher.onRequest<TestRequiredCommand>(nullptr, [&received](const TestRequiredCommand& command) {
        received = command;
        return make_ok();
    });
    ASSERT_TRUE(registered);

    //! DO deliver a request carrying the params of a query
    const Params params = {
        { "i_val", Val("7") },
        { "s_val", Val("test") },
    };
    const Response response = registered(make_request(TestRequiredCommand::id, params));

    //! CHECK the handler received the filled command
    EXPECT_TRUE(response.ret);
    ASSERT_TRUE(received);
    EXPECT_EQ(received->iVal, 7);
    EXPECT_EQ(received->sVal, "test");

    //! DO deliver a request without a Required<T> field
    received.reset();
    const Response rejected = registered(make_request(TestRequiredCommand::id, Params { { "s_val", Val("test") } }));

    //! CHECK the handler is not called and the request is answered with BadArgs
    EXPECT_FALSE(received);
    EXPECT_EQ(rejected.ret.code(), static_cast<int>(Ret::Code::BadArgs));
}

TEST_F(RCommand_TypedCommandTests, MemberCount_MatchesTheDeclaredFields)
{
    //! CHECK the count the `input`/`output` completeness check relies on
    static_assert(internal::memberCount<TestCommand>() == 9);
    static_assert(internal::memberCount<TestCommand::Output>() == 9);
    static_assert(internal::memberCount<TestRequiredCommand>() == 3);
    static_assert(internal::memberCount<TestInternalCommand>() == 0);
    static_assert(fieldsCoverAllMembers<TestRequiredCommand, decltype(TestRequiredCommand::input)>());

    //! CHECK a command with constructors is not checked, since its members cannot be counted
    static_assert(!std::is_aggregate_v<TestConstructedCommand>);
    static_assert(fieldsCoverAllMembers<TestConstructedCommand, decltype(TestConstructedCommand::input)>());
    EXPECT_EQ(inputSchema<TestConstructedCommand>().args.size(), 1);
}

TEST_F(RCommand_TypedCommandTests, EnumContract_RoundTrip)
{
    EXPECT_EQ(EnumContract<TestEnum>::str_val(TestEnum::Value2), "value2");
    EXPECT_EQ(EnumContract<TestEnum>::enum_val("value3"), TestEnum::Value3);
    EXPECT_FALSE(EnumContract<TestEnum>::enum_val("value4").has_value());
}

TEST_F(RCommand_TypedCommandTests, EnumContract_Concepts)
{
    static_assert(IsEnumTable<decltype(TestEnumContract::table)>);
    static_assert(IsEnumTable<const EnumTable<TestEnum, 3>&>);
    static_assert(!IsEnumTable<std::array<int, 3> >);
    static_assert(!IsEnumTable<int>);
    static_assert(std::is_same_v<EnumTableEnum<decltype(TestEnumContract::table)>, TestEnum>);

    static_assert(IsEnumContract<TestEnumContract>);
    static_assert(!IsEnumContract<TestCommand>);
    SUCCEED();
}
