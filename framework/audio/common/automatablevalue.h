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

#include "global/types/secs.h"
#include "global/types/number.h"
#include "global/types/sharedmap.h"
#include "global/containers.h"

#include "mpe/automationpoint.h"

namespace muse::audio {
using AutomationEnvelope = SharedMap<muse::secs_t, mpe::AutomationPoint>;

//! NOTE: a parameter that is either a fixed value, or driven by an AutomationEnvelope over time
template<typename T>
class AutomatableValue
{
public:
    AutomatableValue() = default;
    AutomatableValue(const T& value)
        : m_value(value) {}
    AutomatableValue(const AutomationEnvelope& envelope)
        : m_value(envelope) {}

    bool hasAutomation() const noexcept { return std::holds_alternative<AutomationEnvelope>(m_value); }

    T evaluateAt(muse::secs_t pos, T min, T max) const noexcept
    {
        if (const T* v = std::get_if<T>(&m_value)) {
            return std::clamp(*v, min, max);
        }

        const AutomationEnvelope& envelope = std::get<AutomationEnvelope>(m_value);
        if (envelope.empty()) {
            return T {};
        }

        auto it = findLessOrEqual(envelope, pos);
        if (it == envelope.end()) {
            //! NOTE: hold the first point's value backwards in time, matching standard envelope semantics
            it = envelope.begin();
        }

        real_t normalized = it->second.outValue;
        const auto next = std::next(it);
        if (next != envelope.end()) {
            const real_t t = static_cast<real_t>(pos - it->first) / static_cast<real_t>(next->first - it->first);
            normalized = mpe::evaluateAt(next->second, it->second.outValue, t);
        }

        const T mapped = min + static_cast<T>(normalized.raw()) * (max - min);
        return std::clamp(mapped, min, max);
    }

    const std::variant<T, AutomationEnvelope>& value() const noexcept { return m_value; }

    bool operator==(const AutomatableValue& other) const noexcept { return m_value == other.m_value; }

private:
    std::variant<T, AutomationEnvelope> m_value;
};
}
