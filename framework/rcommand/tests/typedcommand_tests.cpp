/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
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

#include "rcommand/typedcommand.h"

using namespace muse;
using namespace muse::rcommand;

namespace {
enum class Mode {
    Fast = 0,
    Slow
};
}

namespace muse::rcommand {
//! A domain type becomes usable as a field by specializing ParamTraits
template<>
struct ParamTraits<Mode> {
    static constexpr DataType dataType = DataType::String;

    static Val toVal(Mode m) { return Val(m == Mode::Fast ? "fast" : "slow"); }

    static bool fromVal(const Val& v, Mode& out)
    {
        if (v.type() != Val::Type::String) {
            return false;
        }
        if (v.toString() == "fast") {
            out = Mode::Fast;
            return true;
        }
        if (v.toString() == "slow") {
            out = Mode::Slow;
            return true;
        }
        return false;
    }
};
}

namespace {
inline const Command SEEK_COMMAND("command://test/seek");

struct Seek {
    static inline const Command& id = SEEK_COMMAND;
    static inline const TranslatableString title = TranslatableString::untranslatable("Seek");
    static inline const TranslatableString description = TranslatableString::untranslatable("Seek to a position");

    double time = 0.0;
    bool play = false;
    int track = -1;
    std::string label;
    Mode mode = Mode::Fast;
    std::optional<int> loops;

    static constexpr auto fields()
    {
        return std::tuple {
            Field { "time", &Seek::time, u"Position in seconds", 0.0, 3600.0 },
            Field { "play", &Seek::play, u"Start playback" },
            Field { "track", &Seek::track, u"Track index" },
            Field { "label", &Seek::label, u"Label text" },
            Field { "mode", &Seek::mode, u"Seek mode" },
            Field { "loops", &Seek::loops, u"Loop count (optional)" },
        };
    }
};

struct NoFields {
    static inline const Command id { "command://test/no-fields" };
    static inline const TranslatableString title;
    static inline const TranslatableString description;
};
}

TEST(RCommand_TypedCommandTests, TypedCommand_ConceptRequiresIdTextsAndFields)
{
    static_assert(TypedCommand<Seek>);
    static_assert(!TypedCommand<NoFields>);
    static_assert(!TypedCommand<Command>);
}

TEST(RCommand_TypedCommandTests, InputSchema_IsGeneratedFromTheFieldList)
{
    const InputSchema schema = inputSchema<Seek>();

    ASSERT_EQ(schema.args.size(), 6u);
    EXPECT_EQ(schema.args.at("time").type, DataType::Float);
    EXPECT_EQ(schema.args.at("time").min.toDouble(), 0.0);
    EXPECT_EQ(schema.args.at("time").max.toDouble(), 3600.0);
    EXPECT_EQ(schema.args.at("play").type, DataType::Boolean);
    EXPECT_EQ(schema.args.at("track").type, DataType::Integer);
    EXPECT_EQ(schema.args.at("label").type, DataType::String);
    EXPECT_EQ(schema.args.at("mode").type, DataType::String);
    EXPECT_EQ(schema.args.at("loops").type, DataType::Integer);
    EXPECT_EQ(schema.args.at("loops").description, String(u"Loop count (optional)"));
    EXPECT_TRUE(schema.args.at("play").min.isNull());
}

TEST(RCommand_TypedCommandTests, ToParams_FromParams_RoundTrip)
{
    const Seek sent { 12.5, true, 3, "intro", Mode::Slow, 4 };

    const Params params = toParams(sent);
    ASSERT_EQ(params.size(), 6u);
    EXPECT_EQ(params.at("time").toDouble(), 12.5);
    EXPECT_EQ(params.at("play").toBool(), true);
    EXPECT_EQ(params.at("track").toInt(), 3);
    EXPECT_EQ(params.at("label").toString(), "intro");
    EXPECT_EQ(params.at("mode").toString(), "slow");
    EXPECT_EQ(params.at("loops").toInt(), 4);

    Seek received;
    std::string err;
    ASSERT_TRUE(fromParams(params, received, err)) << err;
    EXPECT_EQ(received.time, 12.5);
    EXPECT_EQ(received.play, true);
    EXPECT_EQ(received.track, 3);
    EXPECT_EQ(received.label, "intro");
    EXPECT_EQ(received.mode, Mode::Slow);
    ASSERT_TRUE(received.loops.has_value());
    EXPECT_EQ(*received.loops, 4);
}

TEST(RCommand_TypedCommandTests, OptionalField_MayBeAbsent)
{
    const Seek sent { 1.0, false, 0, "", Mode::Fast, std::nullopt };

    const Params params = toParams(sent);
    EXPECT_FALSE(params.contains("loops"));

    Seek received;
    std::string err;
    ASSERT_TRUE(fromParams(params, received, err)) << err;
    EXPECT_FALSE(received.loops.has_value());
}

TEST(RCommand_TypedCommandTests, FromParams_AcceptsValuesEncodedAsStrings)
{
    //! NOTE: values of a CommandQuery parsed from a URI arrive as strings
    const Params params {
        { "time", Val("2.5") },
        { "play", Val("true") },
        { "track", Val("7") },
        { "label", Val("x") },
        { "mode", Val("fast") },
    };

    Seek received;
    std::string err;
    ASSERT_TRUE(fromParams(params, received, err)) << err;
    EXPECT_EQ(received.time, 2.5);
    EXPECT_EQ(received.play, true);
    EXPECT_EQ(received.track, 7);
}

TEST(RCommand_TypedCommandTests, FromParams_RejectsMissingParameter)
{
    const Params params {
        { "time", Val(1.0) },
        { "play", Val(true) },
        { "track", Val(0) },
        // "label" and "mode" missing
    };

    Seek received;
    std::string err;
    EXPECT_FALSE(fromParams(params, received, err));
    EXPECT_NE(err.find("missing parameter"), std::string::npos) << err;
}

TEST(RCommand_TypedCommandTests, FromParams_RejectsWrongType)
{
    Params params = toParams(Seek {});
    params["track"] = Val("not a number");

    Seek received;
    std::string err;
    EXPECT_FALSE(fromParams(params, received, err));
    EXPECT_NE(err.find("wrong type"), std::string::npos) << err;
}

TEST(RCommand_TypedCommandTests, FromParams_RejectsUnknownParameter)
{
    Params params = toParams(Seek {});
    params["typo"] = Val(1);

    Seek received;
    std::string err;
    EXPECT_FALSE(fromParams(params, received, err));
    EXPECT_NE(err.find("unknown parameter"), std::string::npos) << err;
}

TEST(RCommand_TypedCommandTests, FromParams_RejectsInvalidDomainValue)
{
    Params params = toParams(Seek {});
    params["mode"] = Val("sideways");

    Seek received;
    std::string err;
    EXPECT_FALSE(fromParams(params, received, err));
}
