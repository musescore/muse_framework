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

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QtQml/QQmlParserStatus>
#include <QtQmlIntegration/qqmlintegration.h>

#include "async/asyncable.h"

#include "modularity/ioc.h"
#include "interactive/iinteractive.h"
#include "extensions/iextensioninstaller.h"
#include "extensions/iextensionsconfiguration.h"
#include "extensions/iextensionsregister.h"
#include "shortcuts/ishortcutsregister.h"

namespace muse::extensions {
class ExtensionsListModel : public QAbstractListModel, public QQmlParserStatus, public Contextable, public async::Asyncable
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    QML_ELEMENT

    GlobalInject<IExtensionsConfiguration> configuration;
    GlobalInject<IExtensionsRegister> extensionsRegister;
    ContextInject<IExtensionInstaller> installer = { this };
    ContextInject<IInteractive> interactive = { this };
    ContextInject<shortcuts::IShortcutsRegister> shortcutsRegister = { this };

public:
    explicit ExtensionsListModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setEnabled(const QString& uri, bool enabled);
    Q_INVOKABLE void editShortcut(const QString& uri);
    Q_INVOKABLE void reloadPlugins();
    Q_INVOKABLE void removeExtension(const QString& uri);

    Q_INVOKABLE QVariantList categories() const;

signals:
    void finished();

private:
    enum Roles {
        rUri = Qt::UserRole + 1,
        rName,
        rDescription,
        rThumbnailUrl,
        rEnabled,
        rCategory,
        rVersion,
        rShortcuts,
        rIsRemovable
    };

    void classBegin() override;
    void componentComplete() override {}
    void init();
    void load();

    void updateExtension(const Uri& uri);
    int itemIndexByUri(const QString& uri) const;

    QHash<int, QByteArray> m_roles;
    ManifestList m_extensions;
};
}
