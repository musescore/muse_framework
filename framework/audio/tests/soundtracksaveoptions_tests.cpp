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

#include <limits>

#include "global/async/channel.h"

#pragma pack(push, 1)
#include "audio/common/audiotypes.h"
#pragma pack(pop)

using namespace muse;
using namespace muse::audio;

TEST(Audio_SoundTrackSaveOptionsTests, DefaultOptionsAreValid)
{
    EXPECT_TRUE(SoundTrackSaveOptions().isValid());
}

TEST(Audio_SoundTrackSaveOptionsTests, ValidatesRangeAndFades)
{
    SoundTrackSaveOptions options;
    options.hasTimeRange = true;
    options.startTime = 10.0;
    options.endTime = 16.0;
    options.fadeInDuration = 1.5;
    options.fadeOutDuration = 2.5;

    EXPECT_TRUE(options.isValid());
}

TEST(Audio_SoundTrackSaveOptionsTests, RejectsInvalidRange)
{
    SoundTrackSaveOptions options;
    options.hasTimeRange = true;
    options.startTime = 5.0;
    options.endTime = 5.0;
    EXPECT_FALSE(options.isValid());

    options.endTime = 4.0;
    EXPECT_FALSE(options.isValid());

    options.startTime = -1.0;
    options.endTime = 4.0;
    EXPECT_FALSE(options.isValid());
}

TEST(Audio_SoundTrackSaveOptionsTests, RejectsFadesLongerThanMusicalRange)
{
    SoundTrackSaveOptions options;
    options.hasTimeRange = true;
    options.startTime = 1.0;
    options.endTime = 4.0;
    options.fadeInDuration = 2.0;
    options.fadeOutDuration = 1.1;

    EXPECT_FALSE(options.isValid());
}

TEST(Audio_SoundTrackSaveOptionsTests, AcceptsFadesThatExactlyFillMusicalRange)
{
    SoundTrackSaveOptions options;
    options.hasTimeRange = true;
    options.startTime = 1.0;
    options.endTime = 4.0;
    options.fadeInDuration = 2.0;
    options.fadeOutDuration = 1.0;

    EXPECT_TRUE(options.isValid());
}

TEST(Audio_SoundTrackSaveOptionsTests, RejectsNegativeDurations)
{
    SoundTrackSaveOptions options;
    options.fadeInDuration = -0.1;
    EXPECT_FALSE(options.isValid());

    options = SoundTrackSaveOptions();
    options.fadeOutDuration = -0.1;
    EXPECT_FALSE(options.isValid());
}

TEST(Audio_SoundTrackSaveOptionsTests, RejectsNonFiniteValues)
{
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();

    SoundTrackSaveOptions options;
    options.fadeInDuration = nan;
    EXPECT_FALSE(options.isValid());

    options = SoundTrackSaveOptions();
    options.hasTimeRange = true;
    options.startTime = 0.0;
    options.endTime = infinity;
    EXPECT_FALSE(options.isValid());
}
