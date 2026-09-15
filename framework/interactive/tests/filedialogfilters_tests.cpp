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

#include "interactive/internal/filedialogfilters.h"

using namespace muse::interactive;

class Interactive_FileDialogFiltersTests : public ::testing::Test
{
};

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveGlob_ExpandsLetters)
{
    EXPECT_EQ(caseInsensitiveGlob("*.mp3"), "*.[mM][pP]3");
    EXPECT_EQ(caseInsensitiveGlob("*.3gp"), "*.3[gG][pP]");
    EXPECT_EQ(caseInsensitiveGlob("*.film_cpk"), "*.[fF][iI][lL][mM]_[cC][pP][kK]");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveGlob_NormalisesUpperCaseInput)
{
    EXPECT_EQ(caseInsensitiveGlob("*.MTV"), "*.[mM][tT][vV]");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveGlob_LeavesNonLettersUntouched)
{
    EXPECT_EQ(caseInsensitiveGlob("*"), "*");
    EXPECT_EQ(caseInsensitiveGlob("*.302"), "*.302");
    EXPECT_EQ(caseInsensitiveGlob(""), "");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveGlob_LeavesBracketExpressionsUntouched)
{
    EXPECT_EQ(caseInsensitiveGlob("*.[mM][pP]3"), "*.[mM][pP]3");
    EXPECT_EQ(caseInsensitiveGlob("*.[0-9]"), "*.[0-9]");
    EXPECT_EQ(caseInsensitiveGlob("[a-z]*"), "[a-z]*");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveGlob_ExpandsLettersAroundBracketExpressions)
{
    EXPECT_EQ(caseInsensitiveGlob("*.m[34]a"), "*.[mM][34][aA]");
    EXPECT_EQ(caseInsensitiveGlob("*.[mM]p[34]"), "*.[mM][pP][34]");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_SingleGroup)
{
    EXPECT_EQ(caseInsensitiveNameFilter("Audio files (*.mp3 *.wav)"),
              "Audio files (*.[mM][pP]3 *.[wW][aA][vV])");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_GlobWithBracketExpression)
{
    EXPECT_EQ(caseInsensitiveNameFilter("MPEG-4 audio (*.m4a *.m[34]a)"),
              "MPEG-4 audio (*.[mM]4[aA] *.[mM][34][aA])");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_RewritesOnlyTrailingGroup)
{
    EXPECT_EQ(caseInsensitiveNameFilter("All supported files (*.mp3,*.aac, ...) (*.aac *.ac3 *.mp2)"),
              "All supported files (*.mp3,*.aac, ...) (*.[aA][aA][cC] *.[aA][cC]3 *.[mM][pP]2)");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_AllFiles)
{
    EXPECT_EQ(caseInsensitiveNameFilter("All files (*)"), "All files (*)");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_EmptyGroup)
{
    EXPECT_EQ(caseInsensitiveNameFilter("Name ()"), "Name ()");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_CollapsesWhitespaceBetweenGlobs)
{
    EXPECT_EQ(caseInsensitiveNameFilter("Audio (*.mp3   *.wav )"), "Audio (*.[mM][pP]3 *.[wW][aA][vV])");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_LeavesBareGlobListUntouched)
{
    EXPECT_EQ(caseInsensitiveNameFilter("*.mp3 *.wav"), "*.mp3 *.wav");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_EmptyString)
{
    EXPECT_EQ(caseInsensitiveNameFilter(""), "");
}

TEST_F(Interactive_FileDialogFiltersTests, CaseInsensitiveNameFilter_IsIdempotent)
{
    const std::string once = caseInsensitiveNameFilter("Audio files (*.mp3 *.wav *.m[34]a)");
    EXPECT_EQ(caseInsensitiveNameFilter(once), once);
}
