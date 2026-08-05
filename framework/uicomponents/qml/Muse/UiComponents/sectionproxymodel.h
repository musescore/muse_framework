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

#include <qqmlintegration.h>

#include <QAbstractProxyModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

namespace muse::uicomponents {
class SectionProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(QString sectionRoleName READ sectionRoleName WRITE setSectionRoleName NOTIFY sectionRoleNameChanged)
    Q_PROPERTY(QStringList collapsedSections READ collapsedSections WRITE setCollapsedSections NOTIFY collapsedSectionsChanged)

public:
    explicit SectionProxyModel(QObject* parent = nullptr);

    enum Roles {
        RoleIsSection = Qt::UserRole + 1000,
        RoleSectionName,
        RoleSectionExpanded,
        RoleSectionItemCount,
        RoleIndexInSection
    };

    QString sectionRoleName() const;
    void setSectionRoleName(const QString& roleName);

    QStringList collapsedSections() const;
    void setCollapsedSections(const QStringList& sections);

    Q_INVOKABLE void toggleSection(const QString& section);
    Q_INVOKABLE void setSectionCollapsed(const QString& section, bool collapsed);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    Q_INVOKABLE bool isSectionRow(int row) const;
    Q_INVOKABLE int sourceRowOf(int row) const;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void rowCountChanged();
    void sectionRoleNameChanged();
    void collapsedSectionsChanged();

private:
    static constexpr int INVALID_ROLE_ID = -1;

    struct Row {
        bool isSection = false;
        QString section;
        int sourceRow = -1;
        int indexInSection = 0;
        int itemCount = 0;
    };

    void rebuild();
    void updateRowIndexes();

    void onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles);

    bool isSectionOfRowsChanged(int firstSourceRow, int lastSourceRow, int sectionRoleId) const;

    int sectionRoleId() const;
    QString sectionOfSourceRow(int sourceRow, int sectionRoleId) const;
    bool isCollapsed(const QString& section) const;
    int sectionRow(const QString& section) const;

    QString m_sectionRoleName;
    QStringList m_collapsedSections;

    QList<Row> m_rows;
    QHash<int, int> m_sourceRowToProxyRow;

    QList<QMetaObject::Connection> m_sourceConnections;
};
}
