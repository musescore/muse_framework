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
#pragma once

#include <algorithm>
#include <variant>
#include <optional>
#include <type_traits>

#include "types/number.h"

namespace muse::mpe {
struct AutomationPoint {
    //! NOTE: bends the segment through point (t, value)
    struct Bend {
        real_t t = real_t::make(0.5); // [0; 1]
        real_t value = real_t::make(0.5); // [0; 1]

        static constexpr Bend none() { return {}; }
        bool isNone() const { return *this == none(); }
        bool operator==(const Bend& b) const { return t == b.t && value == b.value; }
    };

    //! NOTE: arrival value equals whatever precedes this point in the curve
    struct ArrivalFromPrevious {
        bool operator==(const ArrivalFromPrevious&) const { return true; }
    };

    //! NOTE: arrival value is an explicit number
    struct ExplicitArrival {
        real_t value = 0.;
        Bend bend;

        bool operator==(const ExplicitArrival& o) const { return value == o.value && bend == o.bend; }
    };

    using InValue = std::variant<ArrivalFromPrevious, ExplicitArrival>;

    InValue inValue = ArrivalFromPrevious {};
    real_t outValue = 0.;

    bool operator==(const AutomationPoint& p) const
    {
        return inValue == p.inValue && outValue == p.outValue;
    }
};

//! NOTE: this point's bend, if it can have one; nullopt for ArrivalFromPrevious
inline std::optional<AutomationPoint::Bend> bend(const AutomationPoint& point) noexcept
{
    return std::visit([](const auto& v) -> std::optional<AutomationPoint::Bend> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AutomationPoint::ArrivalFromPrevious>) {
            return std::nullopt;
        } else {
            return v.bend;
        }
    }, point.inValue);
}

//! NOTE: resolves what this point's arrival value actually is, given prevOutValue
//! (the outValue of whatever precedes it in its curve, if any)
inline real_t resolveInValue(const AutomationPoint& point, std::optional<real_t> prevOutValue) noexcept
{
    return std::visit([&](const auto& v) -> real_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AutomationPoint::ArrivalFromPrevious>) {
            return prevOutValue ? *prevOutValue : real_t(0.0);
        } else {
            return v.value;
        }
    }, point.inValue);
}

//! NOTE: value at normalized position t ([0, 1]) across the segment arriving at point
//! The segment is split into two quadratic Bezier arcs meeting at the bend point. The arcs
//! share a tangent there, producing a smooth bend whose position follows bend.t. When there's
//! no bend, this reduces to a straight line
inline real_t evaluateAt(const AutomationPoint& point, std::optional<real_t> prevOutValue, real_t t) noexcept
{
    const real_t prevOut = prevOutValue ? *prevOutValue : real_t(0.0);
    const real_t thisIn = resolveInValue(point, prevOutValue);
    const real_t range = thisIn - prevOut;
    const std::optional<AutomationPoint::Bend> pointBend = bend(point);

    if (!pointBend || pointBend->isNone() || pointBend->t <= 0.0 || pointBend->t >= 1.0) {
        return std::clamp(prevOut + range * t, real_t(0.0), real_t(1.0));
    }

    auto quadraticBezier = [](real_t s, real_t p0, real_t p1, real_t p2) {
        const real_t u = 1.0 - s;
        return std::clamp(u * u * p0 + 2.0 * u * s * p1 + s * s * p2, real_t(0.0), real_t(1.0));
    };

    const real_t fraction = std::clamp(pointBend->value, real_t(0.0), real_t(1.0));
    const real_t bendValue = prevOut + fraction * range;
    const real_t lo = std::min(prevOut, thisIn);
    const real_t hi = std::max(prevOut, thisIn);
    const real_t halfSlope = 0.5 * range;
    const real_t remainder = 1.0 - pointBend->t;

    // Clamped control points (q1, q2) for each arc
    const real_t q1 = std::clamp(bendValue - pointBend->t * halfSlope, lo, hi);

    if (t <= pointBend->t) {
        return quadraticBezier(t / pointBend->t, prevOut, q1, bendValue);
    }

    const real_t q2 = std::clamp(bendValue + remainder * halfSlope, lo, hi);
    return quadraticBezier((t - pointBend->t) / remainder, bendValue, q2, thisIn);
}
}
