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

#include "stringsearch.h"

#include <algorithm>

bool muse::operator==(const FuzzyMatch& left, const FuzzyMatch& right)
{
    return left.beginPos == right.beginPos
           && left.endPos == right.endPos
           && left.editDistance == right.editDistance;
}

bool muse::operator!=(const FuzzyMatch& left, const FuzzyMatch& right)
{
    return !(left == right);
}

namespace muse {
FuzzyMatcher& FuzzyMatcher::operator()(const std::u32string_view text, const std::u32string_view pattern, const std::size_t maxDistance)
{
    return match(text, pattern, maxDistance);
}

FuzzyMatcher& FuzzyMatcher::match(const std::u32string_view text, const std::u32string_view pattern, const std::size_t maxDistance)
{
    m_textSize = text.size();
    m_patternSize = pattern.size();
    m_suffixTable.clear();
    m_suffixTable.resize((m_patternSize + 1) * (m_textSize + 1));
    m_maxDistance = maxDistance;

    for (std::size_t textSuffixSize = 0; textSuffixSize <= m_textSize; ++textSuffixSize) {
        const std::size_t endPos = text.size() - textSuffixSize;
        // 0 insertions to transform empty pattern to text suffix.
        // This allows the match to be at any position in text.
        m_suffixTable[index(0, textSuffixSize)] = SuffixMatch{ endPos, 0 };
    }

    for (std::size_t patternSuffixSize = 1; patternSuffixSize <= m_patternSize; ++patternSuffixSize) {
        // patternSuffixSize deletions to transform pattern suffix to empty text
        m_suffixTable[index(patternSuffixSize, 0)] = SuffixMatch{ 0, patternSuffixSize };

        const std::size_t patternIdx = pattern.size() - patternSuffixSize;
        for (std::size_t textSuffixSize = 1; textSuffixSize <= m_textSize; ++textSuffixSize) {
            const std::size_t textIdx = text.size() - textSuffixSize;

            if (pattern[patternIdx] == text[textIdx]) {
                m_suffixTable[index(patternSuffixSize, textSuffixSize)] = m_suffixTable[index(patternSuffixSize - 1, textSuffixSize - 1)];
                continue;
            }

            const SuffixMatch insertMatch = m_suffixTable[index(patternSuffixSize, textSuffixSize - 1)];
            const SuffixMatch replaceMatch = m_suffixTable[index(patternSuffixSize - 1, textSuffixSize - 1)];
            const SuffixMatch deleteMatch = m_suffixTable[index(patternSuffixSize - 1, textSuffixSize)];
            SuffixMatch minMatch = std::min(insertMatch, std::min(replaceMatch, deleteMatch));

            if (patternSuffixSize > 1 && textSuffixSize > 1
                && pattern[patternIdx] == text[textIdx + 1]
                && pattern[patternIdx + 1] == text[textIdx]) {
                const SuffixMatch transpositionMatch = m_suffixTable[index(patternSuffixSize - 2, textSuffixSize - 2)];
                minMatch = std::min(transpositionMatch, minMatch);
            }

            m_suffixTable[index(patternSuffixSize, textSuffixSize)] = SuffixMatch{ minMatch.endPos, minMatch.editDistance + 1 };
        }
    }

    return *this;
}

std::optional<FuzzyMatch> FuzzyMatcher::findMatch(const std::size_t textBeginPos) const
{
    if (m_suffixTable.empty() || textBeginPos > m_textSize) {
        return std::nullopt;
    }

    for (std::size_t beginPos = textBeginPos; beginPos <= m_textSize; ++beginPos) {
        const std::size_t textSuffixSize = m_textSize - beginPos;
        const SuffixMatch match = m_suffixTable[index(m_patternSize, textSuffixSize)];
        if (match.editDistance <= m_maxDistance) {
            return FuzzyMatch{ beginPos, match.endPos, match.editDistance };
        }
    }

    return std::nullopt;
}

bool FuzzyMatcher::empty() const
{
    return !findMatch(0).has_value();
}

void FuzzyMatcher::clear()
{
    m_textSize = 0;
    m_patternSize = 0;
    m_suffixTable.clear();
}

FuzzyMatcher::const_iterator FuzzyMatcher::begin() const
{
    return Iterator(*this);
}

FuzzyMatcher::const_iterator FuzzyMatcher::end() const
{
    return Iterator();
}

std::size_t FuzzyMatcher::index(const std::size_t patternSuffixSize, const std::size_t textSuffixSize) const
{
    return (m_textSize + 1) * patternSuffixSize + textSuffixSize;
}

FuzzyMatcher::Iterator::Iterator(const FuzzyMatcher& matcher)
    : m_matcher{&matcher}
{
    next(0);
}

FuzzyMatcher::Iterator::reference FuzzyMatcher::Iterator::operator*() const
{
    return *m_match;
}

FuzzyMatcher::Iterator::pointer FuzzyMatcher::Iterator::operator->() const
{
    return m_match.operator->();
}

FuzzyMatcher::Iterator& FuzzyMatcher::Iterator::operator++()
{
    next(m_match->beginPos + 1);
    return *this;
}

FuzzyMatcher::Iterator FuzzyMatcher::Iterator::operator++(int)
{
    Iterator prev = *this;
    operator++();

    return prev;
}

bool FuzzyMatcher::Iterator::operator==(const Iterator& right) const
{
    return m_matcher == right.m_matcher && m_match == right.m_match;
}

bool FuzzyMatcher::Iterator::operator!=(const Iterator& right) const
{
    return !(*this == right);
}

void FuzzyMatcher::Iterator::next(const std::size_t textBeginPos)
{
    m_match = m_matcher->findMatch(textBeginPos);
    if (!m_match) {
        m_matcher = nullptr;
    }
}
}
