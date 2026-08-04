/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include <ostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "global/stringsearch.h"

using ::testing::ElementsAre;

namespace muse {
void PrintTo(const FuzzyMatch& match, std::ostream* os)
{
    *os << "(beginPos: " << match.beginPos
        << ", endPos: " << match.endPos
        << ", editDistance: " << match.editDistance << ")";
}

TEST(Global_StringSearchTests, testFuzzyMatcherEmpty)
{
    FuzzyMatcher matcher;
    EXPECT_TRUE(matcher.empty());
    EXPECT_FALSE(matcher.findMatch(0).has_value());
}

TEST(Global_StringSearchTests, testFuzzyMatcherEdgeCases)
{
    FuzzyMatcher matcher;
    EXPECT_THAT(matcher(U"", U""), ElementsAre(FuzzyMatch { 0, 0, 0 }));
    EXPECT_THAT(matcher(U"hay", U""), ElementsAre(FuzzyMatch { 0, 0, 0 }, FuzzyMatch { 1, 1, 0 },
                                                  FuzzyMatch { 2, 2, 0 }, FuzzyMatch { 3, 3, 0 }));
    EXPECT_THAT(matcher(U"", U"needle"), ElementsAre(FuzzyMatch { 0, 0, 6 }));
}

TEST(Global_StringSearchTests, testFuzzyMatcher)
{
    FuzzyMatcher matcher;
    EXPECT_THAT(matcher(U"surgery", U"survey", 2), ElementsAre(FuzzyMatch { 0, 7, 2 }));

    // exact
    EXPECT_THAT(matcher(U"hayhayneedlehayhayhay", U"needle", 0), ElementsAre(FuzzyMatch { 6, 12, 0 }));

    // 1 insertion
    EXPECT_THAT(matcher(U"hayhayneedlehayhayhay", U"neele", 1), ElementsAre(FuzzyMatch { 6, 12, 1 }));
    // 1 change
    EXPECT_THAT(matcher(U"hayhayneedlehayhayhay", U"nnedle", 1), ElementsAre(FuzzyMatch { 6, 12, 1 }));
    // 1 deletion
    EXPECT_THAT(matcher(U"hayhayneedlehayhayhay", U"neoedle", 1), ElementsAre(FuzzyMatch { 6, 12, 1 }));
    // 1 transposition
    EXPECT_THAT(matcher(U"hayhayneedlehayhayhay", U"neelde", 1), ElementsAre(FuzzyMatch { 6, 12, 1 }));

    // multiple matches with different edit distance
    EXPECT_THAT(matcher(U"haynedlehayneedlehayhayhay", U"needle", 1),
                ElementsAre(FuzzyMatch { 3, 8, 1 }, FuzzyMatch { 10, 17, 1 }, FuzzyMatch { 11, 17, 0 },
                            FuzzyMatch { 12, 17, 1 }));
    // return best match
    EXPECT_THAT(matcher(U"haynedlehayneedlehayhayhay", U"needle", 0), ElementsAre(FuzzyMatch { 11, 17, 0 }));
    // correct start pos
    EXPECT_THAT(matcher(U"aaab", U"ab", 0), ElementsAre(FuzzyMatch { 2, 4, 0 }));
}
}
