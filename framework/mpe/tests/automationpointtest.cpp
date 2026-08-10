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

#include "mpe/automationpoint.h"

using namespace muse;
using namespace muse::mpe;

static constexpr double ALLOWED_ERROR = 1e-9;

class MPE_AutomationPointTest : public ::testing::Test
{
};

TEST_F(MPE_AutomationPointTest, EvaluateAt_None_IsLinear)
{
    // [GIVEN] A point arriving at 1.0 from 0.0, with no ease
    AutomationPoint point;
    point.inValue = AutomationPoint::ExplicitArrival { 1.0, AutomationPoint::Ease::none() };
    point.outValue = 1.0;

    // [THEN] The value at any t is a straight line between the two endpoints
    EXPECT_NEAR(evaluateAt(point, real_t(0.0), real_t::make(0.25)).raw(), 0.25, ALLOWED_ERROR);
    EXPECT_NEAR(evaluateAt(point, real_t(0.0), real_t::make(0.5)).raw(), 0.5, ALLOWED_ERROR);
    EXPECT_NEAR(evaluateAt(point, real_t(0.0), real_t::make(0.75)).raw(), 0.75, ALLOWED_ERROR);
}

TEST_F(MPE_AutomationPointTest, EvaluateAt_Eased_DiffersFromLinear)
{
    // [GIVEN] A point arriving at 1.0 from 0.0, eased so it reaches 0.9 already at the midpoint
    AutomationPoint::Ease ease;
    ease.t = real_t::make(0.5);
    ease.value = real_t::make(0.9);

    AutomationPoint point;
    point.inValue = AutomationPoint::ExplicitArrival { 1.0, ease };
    point.outValue = 1.0;

    // [THEN] The eased values (0.55, 0.9) follow the ease, not linear interpolation's (0.25, 0.5)
    EXPECT_NEAR(evaluateAt(point, real_t(0.0), real_t::make(0.25)).raw(), 0.55, ALLOWED_ERROR);
    EXPECT_NEAR(evaluateAt(point, real_t(0.0), real_t::make(0.5)).raw(), 0.9, ALLOWED_ERROR);
}

TEST_F(MPE_AutomationPointTest, EvaluateCurveAt_Eased_DiffersFromLinear)
{
    // [GIVEN] A two-point curve where the arrival at key 100 is eased so it reaches 0.9
    // already at key 50
    AutomationPoint::Ease ease;
    ease.t = real_t::make(0.5);
    ease.value = real_t::make(0.9);

    AutomationPoint start;
    start.outValue = 0.0;

    AutomationPoint end;
    end.inValue = AutomationPoint::ExplicitArrival { 1.0, ease };
    end.outValue = 1.0;

    AutomationCurve<int> curve { { 0, start }, { 100, end } };

    // [THEN] The interior values (0.55, 0.9) follow the ease, not linear interpolation's (0.25, 0.5)
    EXPECT_NEAR(evaluateCurveAt(curve, 25).raw(), 0.55, ALLOWED_ERROR);
    EXPECT_NEAR(evaluateCurveAt(curve, 50).raw(), 0.9, ALLOWED_ERROR);
}

TEST_F(MPE_AutomationPointTest, EvaluateCurveAt_HoldsFirstValueBeforeFirstKey)
{
    // [GIVEN] A curve whose first key is not at position 0, with a nonzero first value
    AutomationPoint start;
    start.outValue = 0.3;

    AutomationPoint end;
    end.outValue = 0.8;

    AutomationCurve<int> curve { { 10, start }, { 20, end } };

    // [THEN] Positions before the first key hold the first point's value
    EXPECT_NEAR(evaluateCurveAt(curve, 0).raw(), 0.3, ALLOWED_ERROR);
}
