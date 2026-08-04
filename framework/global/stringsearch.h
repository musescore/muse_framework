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

#pragma once

#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>
#include <vector>

namespace muse {
struct FuzzyMatch {
    std::size_t beginPos = 0;
    std::size_t endPos = 0;
    std::size_t editDistance = 0;
};

bool operator==(const FuzzyMatch&, const FuzzyMatch&);
bool operator!=(const FuzzyMatch&, const FuzzyMatch&);

class FuzzyMatcher
{
public:
    using value_type = FuzzyMatch;

    static constexpr auto UNLIMITED_DISTANCE = static_cast<std::size_t>(-1);

    FuzzyMatcher() = default;

    FuzzyMatcher& operator()(std::u32string_view text, std::u32string_view pattern, std::size_t maxDistance = UNLIMITED_DISTANCE);
    FuzzyMatcher& match(std::u32string_view text, std::u32string_view pattern, std::size_t maxDistance = UNLIMITED_DISTANCE);

    bool empty() const;
    std::optional<FuzzyMatch> findMatch(std::size_t textBeginPos) const;

    void clear();

    class Iterator
    {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = FuzzyMatcher::value_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::input_iterator_tag;

        Iterator() = default;
        explicit Iterator(const FuzzyMatcher&);

        reference operator*() const;
        pointer operator->() const;

        Iterator& operator++();
        Iterator operator++(int);

        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;

    private:
        void next(std::size_t textBeginPos);

        const FuzzyMatcher* m_matcher = nullptr;
        std::optional<FuzzyMatch> m_match;
    };

    using const_iterator = Iterator;

    const_iterator begin() const;
    const_iterator end() const;

private:
    struct SuffixMatch {
        std::size_t endPos = 0;
        std::size_t editDistance = 0;

        friend bool operator<(const SuffixMatch& left, const SuffixMatch& right)
        {
            return left.editDistance < right.editDistance;
        }
    };

    std::size_t index(std::size_t patternSuffixSize, std::size_t textSuffixSize) const;

    std::size_t m_textSize = 0;
    std::size_t m_patternSize = 0;
    std::size_t m_maxDistance = 0;
    std::vector<SuffixMatch> m_suffixTable;
};
}
