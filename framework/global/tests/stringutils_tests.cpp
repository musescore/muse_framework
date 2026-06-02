/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include <string>
#include <vector>

#include "global/stringutils.h"

using namespace muse;

class Global_StringUtilsTests : public ::testing::Test
{
public:
};

TEST_F(Global_StringUtilsTests, Join_NoEscape)
{
    EXPECT_EQ(strings::join({ "a", "b", "c" }, "_"), "a_b_c");
    EXPECT_EQ(strings::join({ "a" }, "_"), "a");
    EXPECT_EQ(strings::join({}, "_"), "");
    EXPECT_EQ(strings::join({ "", "" }, "_"), "_");
}

TEST_F(Global_StringUtilsTests, Join_DefaultSeparator)
{
    EXPECT_EQ(strings::join({ "a", "b", "c" }), "a,b,c");
}

TEST_F(Global_StringUtilsTests, Join_NoEscape_SeparatorInElementIsAmbiguous)
{
    //! GIVEN an element that contains the separator, and no escape character
    //! THEN the separator is emitted verbatim (the result cannot be split back)
    EXPECT_EQ(strings::join({ "a_b", "c" }, "_"), "a_b_c");
}

TEST_F(Global_StringUtilsTests, Join_EscapesSeparatorInElement)
{
    //! GIVEN an element containing the separator, with an escape character
    //! THEN the embedded separator is prefixed with the escape character
    EXPECT_EQ(strings::join({ "a_b", "c" }, "_", "\\"), "a\\_b_c");
}

TEST_F(Global_StringUtilsTests, Join_EscapesEscapeCharacterItself)
{
    //! GIVEN an element containing the escape character
    //! THEN the escape character is itself escaped
    EXPECT_EQ(strings::join({ "a\\b", "c" }, "_", "\\"), "a\\\\b_c");
}

TEST_F(Global_StringUtilsTests, Join_EscapesBothEscapeAndSeparator)
{
    EXPECT_EQ(strings::join({ "a\\_b" }, "_", "\\"), "a\\\\\\_b");
}

TEST_F(Global_StringUtilsTests, Split_NoEscape)
{
    std::vector<std::string> out;
    strings::split("a_b_c", out, "_");
    EXPECT_EQ(out, (std::vector<std::string> { "a", "b", "c" }));
}

TEST_F(Global_StringUtilsTests, Split_NoEscape_EmptyFields)
{
    std::vector<std::string> out;
    strings::split("a__c", out, "_");
    EXPECT_EQ(out, (std::vector<std::string> { "a", "", "c" }));
}

TEST_F(Global_StringUtilsTests, Split_NoEscape_TrailingAndLeadingDelim)
{
    std::vector<std::string> out;
    strings::split("_a_", out, "_");
    EXPECT_EQ(out, (std::vector<std::string> { "", "a", "" }));
}

TEST_F(Global_StringUtilsTests, Split_EmptyString)
{
    std::vector<std::string> out;
    strings::split("", out, "_");
    EXPECT_EQ(out, (std::vector<std::string> { "" }));
}

TEST_F(Global_StringUtilsTests, Split_HonoursEscapedSeparator)
{
    std::vector<std::string> out;
    strings::split("a\\_b_c", out, "_", "\\");
    EXPECT_EQ(out, (std::vector<std::string> { "a_b", "c" }));
}

TEST_F(Global_StringUtilsTests, Split_HonoursEscapedEscapeCharacter)
{
    std::vector<std::string> out;
    strings::split("a\\\\b_c", out, "_", "\\");
    EXPECT_EQ(out, (std::vector<std::string> { "a\\b", "c" }));
}

TEST_F(Global_StringUtilsTests, Split_TrailingEscapeIsDropped)
{
    //! GIVEN a string ending with a lone (dangling) escape character
    //! THEN the escape consumes nothing and is dropped
    std::vector<std::string> out;
    strings::split("ab\\", out, "_", "\\");
    EXPECT_EQ(out, (std::vector<std::string> { "ab" }));
}

TEST_F(Global_StringUtilsTests, RoundTrip_PlainComponents)
{
    const std::vector<std::string> parts { "Effect", "VST3", "Acme", "Reverb", "/usr/lib/reverb.vst3" };
    const std::string joined = strings::join(parts, "_", "\\");

    std::vector<std::string> out;
    strings::split(joined, out, "_", "\\");
    EXPECT_EQ(out, parts);
}

TEST_F(Global_StringUtilsTests, RoundTrip_ComponentsContainingSeparator)
{
    //! GIVEN components that themselves contain the separator
    //! THEN join + split round-trips them back to the original count and values
    const std::vector<std::string> parts { "Effect", "VST_3", "Ac_me", "Re_verb", "/usr/lib/re_verb.vst3" };
    const std::string joined = strings::join(parts, "_", "\\");

    std::vector<std::string> out;
    strings::split(joined, out, "_", "\\");
    EXPECT_EQ(out.size(), 5u);
    EXPECT_EQ(out, parts);
}

TEST_F(Global_StringUtilsTests, RoundTrip_ComponentsContainingEscapeAndSeparator)
{
    const std::vector<std::string> parts { "a\\b", "c_d", "e\\_f", "\\", "_" };
    const std::string joined = strings::join(parts, "_", "\\");

    std::vector<std::string> out;
    strings::split(joined, out, "_", "\\");
    EXPECT_EQ(out, parts);
}

TEST_F(Global_StringUtilsTests, RoundTrip_MultiCharSeparatorAndEscape)
{
    const std::vector<std::string> parts { "a::b", "c", "d##::e" };
    const std::string joined = strings::join(parts, "::", "##");

    std::vector<std::string> out;
    strings::split(joined, out, "::", "##");
    EXPECT_EQ(out, parts);
}

TEST_F(Global_StringUtilsTests, Join_EmptySeparatorWithEscape)
{
    const std::string result = strings::join({ "a", "b", "c" }, "", "\\");
    EXPECT_EQ(result, "abc");
}

TEST_F(Global_StringUtilsTests, Split_EmptyDelimiter)
{
    std::vector<std::string> out;
    strings::split("abc", out, "");
    EXPECT_EQ(out, (std::vector<std::string> { "abc" }));
}
