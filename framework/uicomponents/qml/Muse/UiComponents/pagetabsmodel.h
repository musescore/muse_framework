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

#include <QAbstractListModel>
#include <QStringList>
#include <qqmlintegration.h>

namespace muse::uicomponents {
struct PageTab {
    QString title;
    QString uri;

    //! NOTE The other pages on which the tab is also checked
    QStringList alsoCheckedOnUris;

    bool isTitleBold = false;
};

//! NOTE The model of the tabs that switch the pages of the window.
//! The app shell sets the tabs, see PageTabBar.qml
class PageTabsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString currentUri READ currentUri WRITE setCurrentUri NOTIFY currentUriChanged)

    QML_ELEMENT;
    QML_UNCREATABLE("Use the model of the app shell")

public:
    explicit PageTabsModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE virtual void load();

    QString currentUri() const;
    void setCurrentUri(const QString& uri);

signals:
    void currentUriChanged();

protected:
    void setTabs(const QList<PageTab>& tabs);
    void setTab(int index, const PageTab& tab);

private:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        UriRole,
        IsTitleBoldRole,
        IsCheckedRole
    };

    bool isChecked(const PageTab& tab) const;

    QList<PageTab> m_tabs;
    QString m_currentUri;
};
}
