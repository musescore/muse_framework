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

#include "fuzzyfilter.h"

#include <QChar>

#include "sortfilterproxymodel.h"

namespace muse::uicomponents {
FuzzyFilter::FuzzyFilter(QObject* parent)
    : Filter(parent)
{
}

bool FuzzyFilter::acceptsRow(int sourceRow, const QModelIndex& sourceParent, const SortFilterProxyModel& proxyModel)
{
    const QModelIndex sourceIndex = proxyModel.sourceModel()->index(sourceRow, 0, sourceParent);
    const std::optional<double> score = getOrCalcScore(sourceIndex, proxyModel);

    return score.has_value();
}

void FuzzyFilter::invalidate()
{
    clearScoreCache();
}

QString FuzzyFilter::fuzzyPattern() const
{
    return m_fuzzyPattern;
}

void FuzzyFilter::setFuzzyPattern(const QString& fuzzyPattern)
{
    const QString simplifiedPattern = fuzzyPattern.simplified();
    if (m_fuzzyPattern == simplifiedPattern) {
        return;
    }

    m_fuzzyPattern = simplifiedPattern;
    compilePattern();

    emit fuzzyPatternChanged();
    emit dataChanged();
}

QString FuzzyFilter::roleName() const
{
    return m_roleName;
}

void FuzzyFilter::setRoleName(const QString& roleName)
{
    if (m_roleName == roleName) {
        return;
    }

    clearScoreCache();

    m_roleName = roleName;
    emit roleNameChanged();
    emit dataChanged();
}

std::optional<double> FuzzyFilter::getScore(const QModelIndex& sourceIndex) const
{
    const auto scoreIt = m_scoreCache.find(sourceIndex);
    if (scoreIt != m_scoreCache.end()) {
        return scoreIt.value();
    }

    return std::nullopt;
}

void FuzzyFilter::compilePattern()
{
    clearScoreCache();

    const QStringList tokens = m_fuzzyPattern.toLower().split(u' ');

    m_patternTokens.clear();
    m_patternTokens.reserve(tokens.size());
    for (const auto& token : tokens) {
        m_patternTokens.push_back(token.toStdU32String());
    }
}

std::optional<double> FuzzyFilter::getOrCalcScore(const QModelIndex& sourceIndex, const SortFilterProxyModel& proxyModel)
{
    if (const std::optional<double> score = getScore(sourceIndex)) {
        return score;
    }

    const std::optional<double> score = calcScore(sourceIndex, proxyModel);
    // don't cache score of filtered out items because the cache
    // is always reset before filtering and therefore only used for sorting
    // already filtered items
    if (!score) {
        return score;
    }

    m_scoreCache.try_emplace(sourceIndex, *score);

    return score;
}

std::optional<double> FuzzyFilter::calcScore(const QModelIndex& sourceIndex, const SortFilterProxyModel& proxyModel)
{
    const int role = proxyModel.roleIdFromName(m_roleName);
    if (role == -1) {
        return std::nullopt;
    }

    const QString rawText = proxyModel.sourceModel()->data(sourceIndex, role)
                            .toString();
    const std::u32string text = rawText.toLower().toStdU32String();

    double score = 0.0;
    for (const auto& patternToken : m_patternTokens) {
        const std::size_t tokenSize = patternToken.size();
        if (tokenSize == 0) {
            continue;
        }

        constexpr std::size_t MIN_TOKEN_SIZE_FOR_FUZZY_MATCH = 4;
        constexpr std::size_t CHARS_PER_ERROR = 8;
        const std::size_t maxDistance = tokenSize >= MIN_TOKEN_SIZE_FOR_FUZZY_MATCH
                                        ? 1 + (tokenSize / CHARS_PER_ERROR)
                                        : 0;

        const double perCharScore = 1.0 / tokenSize;
        std::optional<double> tokenScore;
        for (const auto& match : m_matcher(text, patternToken, maxDistance)) {
            const double matchSimilarity = 1.0 - (match.editDistance * perCharScore);

            const double scoreBonus = [&] {
                const bool isFullMatch = (match.endPos - match.beginPos) == text.size();
                if (isFullMatch) {
                    return 3.0 * perCharScore;
                }

                const bool isMatchStartAtStartOfWord = match.beginPos == 0
                                                       || !QChar::isLetter(text[match.beginPos - 1]);
                if (isMatchStartAtStartOfWord) {
                    const bool isMatchEndAtEndOfWord = match.endPos == text.size()
                                                       || !QChar::isLetter(text[match.endPos]);
                    if (isMatchEndAtEndOfWord) {
                        return 2.0 * perCharScore;
                    } else {
                        return perCharScore;
                    }
                }

                return 0.0;
            }();

            const double matchScore = 5.0 * matchSimilarity + scoreBonus;
            if (tokenScore < matchScore) {
                tokenScore = matchScore;
            }
        }

        // no match for token found -> no score for entire pattern
        if (!tokenScore) {
            return std::nullopt;
        }

        score += *tokenScore;
    }

    return score;
}

void FuzzyFilter::clearScoreCache()
{
    m_scoreCache.clear();
}
}
