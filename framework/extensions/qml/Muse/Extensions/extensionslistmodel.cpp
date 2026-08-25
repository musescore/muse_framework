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

#include "extensionslistmodel.h"

#include "global/translation.h"

#include "shortcuts/shortcutstypes.h"

#include "extensionscommands.h"

#include "log.h"

using namespace muse::extensions;
using namespace muse::async;

static constexpr int INVALID_INDEX = -1;

ExtensionsListModel::ExtensionsListModel(QObject* parent)
    : QAbstractListModel(parent), Contextable(muse::iocCtxForQmlObject(this))
{
    m_roles = {
        { rUri, "uri" },
        { rName, "name" },
        { rDescription, "description" },
        { rThumbnailUrl, "thumbnailUrl" },
        { rEnabled, "enabled" },
        { rCategory, "category" },
        { rVersion, "version" },
        { rShortcuts, "shortcuts" },
        { rIsRemovable, "isRemovable" }
    };
}

void ExtensionsListModel::classBegin()
{
    init();
}

void ExtensionsListModel::init()
{
    extensionsRegister()->manifestListChanged().onNotify(this, [this]() {
        load();
    });

    extensionsRegister()->enabledChanged().onReceive(this, [this](const Uri& uri) {
        updateExtension(uri);
    });

    load();
}

void ExtensionsListModel::load()
{
    beginResetModel();

    m_extensions = extensionsRegister()->manifestList();
    if (m_extensions.empty()) {
        LOGE() << "Not found plugins";
        endResetModel();
        return;
    }

    std::sort(m_extensions.begin(), m_extensions.end(), [](const Manifest& l, const Manifest& r) {
        return l.title < r.title;
    });

    endResetModel();
}

QVariant ExtensionsListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Manifest manifest = m_extensions.at(index.row());

    switch (role) {
    case rUri:
        return QString::fromStdString(manifest.uri.toString());
    case rName:
        return manifest.title.toQString();
    case rDescription:
        return manifest.description.toQString();
    case rThumbnailUrl:
        if (manifest.thumbnail.empty()) {
            return "qrc:/qt/qml/Muse/Extensions/internal/resources/placeholder.png";
        }

        return QUrl::fromLocalFile(manifest.thumbnail.toQString());
    case rEnabled:
        return extensionsRegister()->isEnabled(manifest.uri);
    case rCategory:
        return manifest.category.toQString();
    case rVersion:
        if (manifest.version.empty()) {
            //: No version is specified for this plugin.
            return muse::qtrc("extensions", "Not specified");
        }
        return QString::fromStdString(manifest.version);
    case rShortcuts: {
        std::vector<std::string> shortcuts;
        IF_ASSERT_FAILED(!manifest.actions.empty()) {
            return muse::qtrc("extensions", "Not defined");
        }

        //! TODO add actions support
        std::string code = makeCommand(manifest.uri, manifest.actions.at(0).code).toString();
        shortcuts::Shortcut shortcut = shortcutsRegister()->shortcut(code);
        return shortcuts::sequencesToNativeText(shortcut.sequences);

        //: No keyboard shortcut is assigned to this plugin.
        return muse::qtrc("extensions", "Not defined");
    }
    case rIsRemovable: {
        return manifest.isRemovable;
    }
    }

    return QVariant();
}

int ExtensionsListModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_extensions.size());
}

QHash<int, QByteArray> ExtensionsListModel::roleNames() const
{
    return m_roles;
}

void ExtensionsListModel::setEnabled(const QString& uri, bool enabled)
{
    extensionsRegister()->setEnabled(Uri(uri.toStdString()), enabled);
}

void ExtensionsListModel::editShortcut(const QString& extensionUri)
{
    int index = itemIndexByUri(extensionUri);
    if (index == INVALID_INDEX) {
        return;
    }

    const Manifest& manifest = m_extensions.at(index);
    IF_ASSERT_FAILED(!manifest.actions.empty()) {
        return;
    }

    //! TODO add actions support
    QString commandCode = QString::fromStdString(makeCommand(manifest.uri, manifest.actions.at(0).code).toString());

    UriQuery preferencesUri("muse://preferences");
    preferencesUri.addParam("currentPageId", Val("shortcuts"));

    QVariantMap params;
    params["shortcutCodeKey"] = commandCode;
    preferencesUri.addParam("params", Val::fromQVariant(params));

    RetVal<Val> retVal = interactive()->openSync(preferencesUri);

    if (!retVal.ret) {
        LOGE() << retVal.ret.toString();
    }
}

void ExtensionsListModel::reloadPlugins()
{
    extensionsRegister()->reload();
}

void ExtensionsListModel::removeExtension(const QString& uri)
{
    installer()->uninstallExtension(Uri(uri.toStdString()));
}

QVariantList ExtensionsListModel::categories() const
{
    QVariantList result;

    for (const auto& category : extensionsRegister()->knownCategories()) {
        QVariantMap obj;
        obj["code"] = QString::fromStdString(category.first);
        obj["title"] = category.second.qTranslated();

        result << obj;
    }

    return result;
}

void ExtensionsListModel::updateExtension(const Uri& uri)
{
    for (size_t i = 0; i < m_extensions.size(); ++i) {
        if (m_extensions.at(i).uri == uri) {
            QModelIndex index = createIndex(int(i), 0);
            emit dataChanged(index, index);
            return;
        }
    }
}

int ExtensionsListModel::itemIndexByUri(const QString& uri_) const
{
    Uri uri(uri_.toStdString());
    for (size_t i = 0; i < m_extensions.size(); ++i) {
        if (m_extensions[i].uri == uri) {
            return static_cast<int>(i);
        }
    }

    return INVALID_INDEX;
}
