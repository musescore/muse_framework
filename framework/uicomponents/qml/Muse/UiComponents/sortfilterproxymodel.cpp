/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include "sortfilterproxymodel.h"

#include <algorithm>

#include <QTimer>
#include <QtVersionChecks>

#include "global/log.h"

#include "uicomponents/view/modelutils.h"

using namespace muse::uicomponents;

SortFilterProxyModel::SortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent), m_filters(this), m_sorters(this)
{
    ModelUtils::connectRowCountChangedSignal(this, &SortFilterProxyModel::rowCountChanged);
    connect(this, &SortFilterProxyModel::sourceModelRoleNamesChanged, this, &SortFilterProxyModel::updateRoleIds);

    const auto invalidateRows = [this] {
        beginFilterChange();
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        endFilterChange(Direction::Rows);
#else
        invalidateFilter();
#endif
    };

    auto onFilterChanged = [this, invalidateRows](Filter* changedFilter) {
        if (changedFilter->async()) {
            QTimer::singleShot(0, this, invalidateRows);
        } else {
            invalidateRows();
        }
    };

    connect(m_filters.notifier(), &QmlListPropertyNotifier::appended, this, [this, onFilterChanged](int index) {
        Filter* filter = m_filters.at(index);
        if (filter->enabled()) {
            onFilterChanged(filter);
        }

        connect(filter, &Filter::dataChanged, this, [onFilterChanged, filter] { onFilterChanged(filter); });
    });

    connect(m_sorters.notifier(), &QmlListPropertyNotifier::appended, this, [this](int index) {
        Sorter* sorter = m_sorters.at(index);
        if (sorter->enabled()) {
            updateSorting();
        }

        connect(sorter, &Sorter::dataChanged, this, &SortFilterProxyModel::updateSorting);
    });

    connect(this, &SortFilterProxyModel::sourceModelRoleNamesChanged, this, [this]() {
        invalidate();
    });
}

QQmlListProperty<Filter> SortFilterProxyModel::filters()
{
    return m_filters.property();
}

QQmlListProperty<Sorter> SortFilterProxyModel::sorters()
{
    return m_sorters.property();
}

QList<int> SortFilterProxyModel::alwaysIncludeIndices() const
{
    return m_alwaysIncludeIndices;
}

void SortFilterProxyModel::setAlwaysIncludeIndices(const QList<int>& indices)
{
    if (m_alwaysIncludeIndices == indices) {
        return;
    }

    beginFilterChange();
    m_alwaysIncludeIndices = indices;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif

    emit alwaysIncludeIndicesChanged();
}

QList<int> SortFilterProxyModel::alwaysExcludeIndices() const
{
    return m_alwaysExcludeIndices;
}

void SortFilterProxyModel::setAlwaysExcludeIndices(const QList<int>& indices)
{
    if (m_alwaysExcludeIndices == indices) {
        return;
    }

    beginFilterChange();
    m_alwaysExcludeIndices = indices;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif

    emit alwaysExcludeIndicesChanged();
}

QString SortFilterProxyModel::sectionRoleName() const
{
    return m_sectionRoleName;
}

void SortFilterProxyModel::setSectionRoleName(const QString& roleName)
{
    if (m_sectionRoleName == roleName) {
        return;
    }

    m_sectionRoleName = roleName;
    m_sectionRoleId = roleIdFromName(m_sectionRoleName);
    updateSorting();

    emit sectionRoleNameChanged();
}

int SortFilterProxyModel::roleIdFromName(const QString& roleName) const
{
    return m_roleIds.value(roleName.toUtf8(), INVALID_ROLE_ID);
}

QHash<int, QByteArray> SortFilterProxyModel::roleNames() const
{
    if (!sourceModel()) {
        return {};
    }

    return sourceModel()->roleNames();
}

void SortFilterProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    disconnect(m_subSourceModelConnection);
    disconnect(m_sourceModelAboutToBeResetConnection);
    disconnect(m_sourceDataChangedConnection);
    invalidateFilters();

    QSortFilterProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        m_sourceDataChangedConnection = connect(sourceModel, &QAbstractItemModel::dataChanged,
                                                this, &SortFilterProxyModel::invalidateFilters);
        m_sourceModelAboutToBeResetConnection = connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset,
                                                        this, &SortFilterProxyModel::invalidateFilters);
    }

    emit sourceModelRoleNamesChanged();

    if (auto sourceSortFilterModel = qobject_cast<SortFilterProxyModel*>(sourceModel)) {
        m_subSourceModelConnection = connect(sourceSortFilterModel, &SortFilterProxyModel::sourceModelRoleNamesChanged,
                                             this, &SortFilterProxyModel::sourceModelRoleNamesChanged);
    }
}

bool SortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_alwaysIncludeIndices.contains(sourceRow)) {
        return true;
    }

    if (m_alwaysExcludeIndices.contains(sourceRow)) {
        return false;
    }

    const QList<Filter*> filters = m_filters.list();
    return std::all_of(filters.begin(), filters.end(), [&] (Filter* filter) {
        return !filter->enabled() || filter->acceptsRow(sourceRow, sourceParent, *this);
    });
}

bool SortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    if (m_sectionRoleId != INVALID_ROLE_ID) {
        const QString leftSection = left.data(m_sectionRoleId).toString();
        const QString rightSection = right.data(m_sectionRoleId).toString();

        if (leftSection != rightSection) {
            bool isLess = false;
            if (leftSection.isEmpty()) {
                isLess = false;
            } else if (rightSection.isEmpty()) {
                isLess = true;
            } else {
                isLess = QString::localeAwareCompare(leftSection, rightSection) < 0;
            }

            return sortOrder() == Qt::DescendingOrder ? !isLess : isLess;
        }
    }

    Sorter* sorter = currentSorter();
    if (!sorter) {
        return left < right;
    }

    return sorter->lessThan(left, right, *this);
}

Sorter* SortFilterProxyModel::currentSorter() const
{
    const QList<Sorter*> sorterList = m_sorters.list();
    for (Sorter* sorter : sorterList) {
        if (sorter->enabled()) {
            return sorter;
        }
    }

    return nullptr;
}

void SortFilterProxyModel::updateRoleIds()
{
    m_roleIds.clear();

    const QHash<int, QByteArray> roleNames = this->roleNames();
    for (const auto& [roleId, roleName] : roleNames.asKeyValueRange()) {
        const auto [it, didInsert] = m_roleIds.try_emplace(roleName, roleId);
        DO_ASSERT_X(didInsert, "duplicate role name");
    }

    m_sectionRoleId = roleIdFromName(m_sectionRoleName);
}

void SortFilterProxyModel::invalidateFilters()
{
    const QList<Filter*> filters = m_filters.list();
    for (auto* filter : filters) {
        filter->invalidate();
    }
}

void SortFilterProxyModel::updateSorting()
{
    Sorter* sorter = currentSorter();
    invalidate();

    if (!sorter) {
        sort(m_sectionRoleName.isEmpty() ? -1 : 0, Qt::AscendingOrder);
        return;
    }

    sort(0, sorter->sortOrder());
}
