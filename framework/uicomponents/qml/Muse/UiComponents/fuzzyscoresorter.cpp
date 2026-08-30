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

#include "fuzzyscoresorter.h"

#include "sortfilterproxymodel.h"

#include <algorithm>

namespace muse::uicomponents {
namespace {
std::optional<double> getMaxScoreFromChildren(const FuzzyFilter& fuzzyFilter, const QAbstractItemModel& model,
                                              const QModelIndex& parentIndex)
{
    const int childRowCount = model.rowCount(parentIndex);

    std::optional<double> maxChildScore;
    for (int i = 0; i < childRowCount; ++i) {
        const QModelIndex childIndex = model.index(i, 0, parentIndex);
        const std::optional<double> maxGrandChildrenScore = getMaxScoreFromChildren(fuzzyFilter, model, childIndex);
        const std::optional<double> childScore = std::max(maxGrandChildrenScore, fuzzyFilter.getScore(childIndex));

        maxChildScore = std::max(maxChildScore, childScore);
    }

    return maxChildScore;
}
}

FuzzyScoreSorter::FuzzyScoreSorter(QObject* parent)
    : Sorter(parent)
{
    setSortOrder(Qt::DescendingOrder);
}

bool FuzzyScoreSorter::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight,
                                const SortFilterProxyModel& proxyModel)
{
    if (!m_fuzzyFilter) {
        return sourceLeft < sourceRight;
    }

    const std::optional<double> leftScore = m_fuzzyFilter->getScore(sourceLeft);
    const std::optional<double> rightScore = m_fuzzyFilter->getScore(sourceRight);

    if (!proxyModel.isRecursiveFilteringEnabled()) {
        return leftScore < rightScore;
    }

    // rank parent items according to the highest child score

    const QAbstractItemModel& srcModel = *proxyModel.sourceModel();

    const std::optional<double> maxLeftChildScore = getMaxScoreFromChildren(*m_fuzzyFilter, srcModel, sourceLeft);
    const std::optional<double> maxRightChildScore = getMaxScoreFromChildren(*m_fuzzyFilter, srcModel, sourceRight);

    return std::max(leftScore, maxLeftChildScore) < std::max(rightScore, maxRightChildScore);
}

FuzzyFilter* FuzzyScoreSorter::fuzzyFilter() const
{
    return m_fuzzyFilter;
}

void FuzzyScoreSorter::setFuzzyFilter(FuzzyFilter* fuzzyFilter)
{
    if (m_fuzzyFilter == fuzzyFilter) {
        return;
    }

    disconnect(m_filterChangedConnection);

    m_fuzzyFilter = fuzzyFilter;

    if (m_fuzzyFilter) {
        m_filterChangedConnection = connect(m_fuzzyFilter, &Filter::dataChanged, this, &Sorter::dataChanged);
    }

    emit fuzzyFilterChanged();
    emit dataChanged();
}
}
