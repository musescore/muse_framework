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

#include "sectionproxymodel.h"

#include "sortfilterproxymodel.h"

using namespace muse::uicomponents;

SectionProxyModel::SectionProxyModel(QObject* parent)
    : QAbstractProxyModel(parent)
{
}

QString SectionProxyModel::sectionRoleName() const
{
    return m_sectionRoleName;
}

void SectionProxyModel::setSectionRoleName(const QString& roleName)
{
    if (m_sectionRoleName == roleName) {
        return;
    }

    m_sectionRoleName = roleName;
    rebuild();

    emit sectionRoleNameChanged();
}

QStringList SectionProxyModel::collapsedSections() const
{
    return m_collapsedSections;
}

void SectionProxyModel::setCollapsedSections(const QStringList& sections)
{
    if (m_collapsedSections == sections) {
        return;
    }

    m_collapsedSections = sections;
    rebuild();

    emit collapsedSectionsChanged();
}

void SectionProxyModel::toggleSection(const QString& section)
{
    setSectionCollapsed(section, !isCollapsed(section));
}

void SectionProxyModel::setSectionCollapsed(const QString& section, bool collapsed)
{
    if (isCollapsed(section) == collapsed) {
        return;
    }

    const int headerRow = sectionRow(section);

    if (collapsed) {
        m_collapsedSections.append(section);

        if (headerRow >= 0) {
            int lastRow = headerRow;
            while (lastRow + 1 < m_rows.size() && !m_rows.at(lastRow + 1).isSection) {
                ++lastRow;
            }

            if (lastRow > headerRow) {
                beginRemoveRows(QModelIndex(), headerRow + 1, lastRow);
                m_rows.remove(headerRow + 1, lastRow - headerRow);
                updateRowIndexes();
                endRemoveRows();

                emit rowCountChanged();
            }
        }
    } else {
        m_collapsedSections.removeAll(section);

        const int roleId = sectionRoleId();
        if (headerRow >= 0 && roleId != INVALID_ROLE_ID && sourceModel()) {
            QList<Row> newRows;
            const int sourceRowCount = sourceModel()->rowCount();

            for (int i = 0; i < sourceRowCount; ++i) {
                if (sectionOfSourceRow(i, roleId) != section) {
                    continue;
                }

                Row row;
                row.section = section;
                row.sourceRow = i;
                newRows.append(row);
            }

            if (!newRows.isEmpty()) {
                beginInsertRows(QModelIndex(), headerRow + 1, headerRow + newRows.size());
                for (int i = 0; i < newRows.size(); ++i) {
                    m_rows.insert(headerRow + 1 + i, newRows.at(i));
                }
                updateRowIndexes();
                endInsertRows();

                emit rowCountChanged();
            }
        }
    }

    if (headerRow >= 0) {
        const QModelIndex idx = index(headerRow, 0);
        emit dataChanged(idx, idx, { RoleSectionExpanded });
    }

    emit collapsedSectionsChanged();
}

void SectionProxyModel::expandAll()
{
    setCollapsedSections(QStringList());
}

void SectionProxyModel::collapseAll()
{
    QStringList sections;
    for (const Row& row : std::as_const(m_rows)) {
        if (row.isSection) {
            sections.append(row.section);
        }
    }

    setCollapsedSections(sections);
}

bool SectionProxyModel::isSectionRow(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return false;
    }

    return m_rows.at(row).isSection;
}

int SectionProxyModel::sourceRowOf(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return -1;
    }

    return m_rows.at(row).sourceRow;
}

void SectionProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    for (const QMetaObject::Connection& connection : std::as_const(m_sourceConnections)) {
        disconnect(connection);
    }
    m_sourceConnections.clear();

    QAbstractProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        const auto rebuildOnChange = [this]() { rebuild(); };

        m_sourceConnections << connect(sourceModel, &QAbstractItemModel::modelReset, this, rebuildOnChange);
        m_sourceConnections << connect(sourceModel, &QAbstractItemModel::layoutChanged, this, rebuildOnChange);
        m_sourceConnections << connect(sourceModel, &QAbstractItemModel::rowsInserted, this, rebuildOnChange);
        m_sourceConnections << connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, rebuildOnChange);
        m_sourceConnections << connect(sourceModel, &QAbstractItemModel::rowsMoved, this, rebuildOnChange);
        m_sourceConnections << connect(sourceModel, &QAbstractItemModel::dataChanged, this,
                                       &SectionProxyModel::onSourceDataChanged);

        if (auto sortFilterModel = qobject_cast<SortFilterProxyModel*>(sourceModel)) {
            m_sourceConnections << connect(sortFilterModel, &SortFilterProxyModel::sourceModelRoleNamesChanged,
                                           this, rebuildOnChange);
        }
    }

    rebuild();
}

QModelIndex SectionProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || !sourceModel()) {
        return QModelIndex();
    }

    const int sourceRow = sourceRowOf(proxyIndex.row());
    if (sourceRow < 0) {
        return QModelIndex();
    }

    return sourceModel()->index(sourceRow, proxyIndex.column());
}

QModelIndex SectionProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid()) {
        return QModelIndex();
    }

    const int proxyRow = m_sourceRowToProxyRow.value(sourceIndex.row(), -1);
    if (proxyRow < 0) {
        return QModelIndex();
    }

    return index(proxyRow, sourceIndex.column());
}

QModelIndex SectionProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() || row < 0 || row >= m_rows.size() || column < 0 || column >= columnCount()) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex SectionProxyModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

int SectionProxyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_rows.size();
}

int SectionProxyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !sourceModel()) {
        return 0;
    }

    return sourceModel()->columnCount();
}

QVariant SectionProxyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const Row& row = m_rows.at(index.row());

    switch (role) {
    case RoleIsSection: return row.isSection;
    case RoleSectionName: return row.section;
    case RoleSectionExpanded: return !isCollapsed(row.section);
    case RoleSectionItemCount: return row.itemCount;
    case RoleIndexInSection: return row.indexInSection;
    default: break;
    }

    if (row.isSection) {
        return QVariant();
    }

    return QAbstractProxyModel::data(index, role);
}

QHash<int, QByteArray> SectionProxyModel::roleNames() const
{
    QHash<int, QByteArray> roles = sourceModel() ? sourceModel()->roleNames() : QHash<int, QByteArray>();

    roles.insert(RoleIsSection, "isSection");
    roles.insert(RoleSectionName, "sectionName");
    roles.insert(RoleSectionExpanded, "sectionExpanded");
    roles.insert(RoleSectionItemCount, "sectionItemCount");
    roles.insert(RoleIndexInSection, "indexInSection");

    return roles;
}

void SectionProxyModel::rebuild()
{
    beginResetModel();

    m_rows.clear();
    m_sourceRowToProxyRow.clear();

    if (sourceModel()) {
        const int roleId = sectionRoleId();
        const int sourceRowCount = sourceModel()->rowCount();

        if (roleId == INVALID_ROLE_ID) {
            for (int i = 0; i < sourceRowCount; ++i) {
                Row itemRow;
                itemRow.sourceRow = i;
                m_rows.append(itemRow);
            }
        } else {
            QStringList sectionOrder;
            QHash<QString, QList<int> > sourceRowsBySection;

            for (int i = 0; i < sourceRowCount; ++i) {
                const QString section = sectionOfSourceRow(i, roleId);

                auto it = sourceRowsBySection.find(section);
                if (it == sourceRowsBySection.end()) {
                    sectionOrder.append(section);
                    it = sourceRowsBySection.insert(section, QList<int>());
                }

                it.value().append(i);
            }

            for (const QString& section : std::as_const(sectionOrder)) {
                const QList<int>& sourceRows = sourceRowsBySection.value(section);

                Row headerRow;
                headerRow.isSection = true;
                headerRow.section = section;
                headerRow.itemCount = sourceRows.size();
                m_rows.append(headerRow);

                if (isCollapsed(section)) {
                    continue;
                }

                for (int sourceRow : sourceRows) {
                    Row itemRow;
                    itemRow.section = section;
                    itemRow.sourceRow = sourceRow;
                    m_rows.append(itemRow);
                }
            }
        }
    }

    updateRowIndexes();

    endResetModel();

    emit rowCountChanged();
}

void SectionProxyModel::updateRowIndexes()
{
    m_sourceRowToProxyRow.clear();

    int indexInSection = 0;

    for (int i = 0; i < m_rows.size(); ++i) {
        Row& row = m_rows[i];

        if (row.isSection) {
            indexInSection = 0;
            continue;
        }

        row.indexInSection = indexInSection++;
        m_sourceRowToProxyRow.insert(row.sourceRow, i);
    }
}

void SectionProxyModel::onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                            const QList<int>& roles)
{
    const int roleId = sectionRoleId();
    if (roleId != INVALID_ROLE_ID) {
        if (roles.contains(roleId)) {
            rebuild();
            return;
        }

        if (roles.isEmpty() && isSectionOfRowsChanged(topLeft.row(), bottomRight.row(), roleId)) {
            rebuild();
            return;
        }
    }

    for (int sourceRow = topLeft.row(); sourceRow <= bottomRight.row(); ++sourceRow) {
        const QModelIndex proxyIndex = mapFromSource(sourceModel()->index(sourceRow, topLeft.column()));
        if (proxyIndex.isValid()) {
            emit dataChanged(proxyIndex, proxyIndex, roles);
        }
    }
}

bool SectionProxyModel::isSectionOfRowsChanged(int firstSourceRow, int lastSourceRow, int sectionRoleId) const
{
    for (int sourceRow = firstSourceRow; sourceRow <= lastSourceRow; ++sourceRow) {
        const int proxyRow = m_sourceRowToProxyRow.value(sourceRow, -1);
        if (proxyRow < 0) {
            return true;
        }

        if (m_rows.at(proxyRow).section != sectionOfSourceRow(sourceRow, sectionRoleId)) {
            return true;
        }
    }

    return false;
}

int SectionProxyModel::sectionRoleId() const
{
    if (m_sectionRoleName.isEmpty() || !sourceModel()) {
        return INVALID_ROLE_ID;
    }

    const QByteArray roleName = m_sectionRoleName.toUtf8();
    const QHash<int, QByteArray> roles = sourceModel()->roleNames();

    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value() == roleName) {
            return it.key();
        }
    }

    return INVALID_ROLE_ID;
}

QString SectionProxyModel::sectionOfSourceRow(int sourceRow, int sectionRoleId) const
{
    if (sectionRoleId == INVALID_ROLE_ID || !sourceModel()) {
        return QString();
    }

    return sourceModel()->index(sourceRow, 0).data(sectionRoleId).toString();
}

bool SectionProxyModel::isCollapsed(const QString& section) const
{
    return m_collapsedSections.contains(section);
}

int SectionProxyModel::sectionRow(const QString& section) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row& row = m_rows.at(i);
        if (row.isSection && row.section == section) {
            return i;
        }
    }

    return -1;
}
