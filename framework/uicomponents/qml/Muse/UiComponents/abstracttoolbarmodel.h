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

#include <qqmlintegration.h>

#include <QAbstractListModel>
#include <QObject>

#include "async/asyncable.h"

#include "modularity/ioc.h"
#include "ui/iuiactionsregister.h"
#include "shortcuts/ishortcutsregister.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/icommandsstate.h"
#include "rcommand/icommandsregister.h"
#include "muse_framework_config.h"

Q_MOC_INCLUDE("uicomponents/qml/Muse/UiComponents/toolbaritem.h")

namespace muse::uicomponents {
class ToolBarItem;
class MenuItem;
using ToolBarItemList = QList<ToolBarItem*>;
namespace ToolBarItemType {
Q_NAMESPACE;
QML_ELEMENT;

enum Type
{
    ACTION,
    SEPARATOR,
    USER_TYPE
};
Q_ENUM_NS(Type)
}

class AbstractToolBarModel : public QAbstractListModel, public Contextable, public async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT;

    Q_PROPERTY(int length READ rowCount NOTIFY itemsChanged)
    Q_PROPERTY(QVariantList items READ itemsProperty NOTIFY itemsChanged)

    Q_PROPERTY(bool isCompactMode READ isCompactMode WRITE setIsCompactMode NOTIFY isCompactModeChanged)

public:
    GlobalInject<rcommand::ICommandsRegister> commandsRegister;
    ContextInject<shortcuts::IShortcutsRegister> shortcutsRegister = { this };
    ContextInject<rcommand::ICommandsState> commandsState = { this };

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    ContextInject<ui::IUiActionsRegister> uiActionsRegister = { this };
    ContextInject<actions::IActionsDispatcher> dispatcher = { this };
#endif

public:
    explicit AbstractToolBarModel(QObject* parent = nullptr);

    const actions::ActionCode SEPARATOR_ID = "";

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE virtual void load();

    QVariantList itemsProperty() const;
    const ToolBarItemList& items() const;

    Q_INVOKABLE QVariantMap get(int index);

    bool isCompactMode() const;
    void setIsCompactMode(bool isCompactMode);

signals:
    void itemsChanged();
    void itemChanged(muse::uicomponents::ToolBarItem* item);

    void isCompactModeChanged();

protected:
    enum Roles {
        ItemRole,

        UserRole
    };

    void setItem(int index, ToolBarItem* item);
    void setItems(const ToolBarItemList& items);
    void clear();

    static const int INVALID_ITEM_INDEX;
    int itemIndex(const QString& itemId) const;

    ToolBarItem& item(int index);

    ToolBarItem& findItem(const QString& itemId);
    ToolBarItem* findItemPtr(const QString& itemId);

    ToolBarItem* makeSeparator();

    bool isIndexValid(int index) const;

    // command support
    virtual void onCommandStateChanged(const rcommand::Command& command, const rcommand::CommandState& state);
    void updateState(ToolBarItemList& items, const rcommand::Command& command, const rcommand::CommandState& state);
    void updateState(QList<MenuItem*>& items, const rcommand::Command& command, const rcommand::CommandState& state);

    ToolBarItem* makeItem(const rcommand::Command& command, const TranslatableString& title = {});
    ToolBarItem& findItem(const rcommand::Command& command) const;
    ToolBarItem* findItemPtr(const rcommand::Command& command) const;

    // actions support
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    ToolBarItem* makeItem(const actions::ActionCode& actionCode, const TranslatableString& title = {});
    ToolBarItem* makeMenuItem(const TranslatableString& title, const actions::ActionCodeList& subitemsActionCodesLists,
                              const QString& menuId = "", bool enabled = true);

    ToolBarItem& findItem(const actions::ActionCode& actionCode);
    ToolBarItem* findItemPtr(const actions::ActionCode& actionCode);

    virtual void onActionsStateChanges(const actions::ActionCodeList& codes);

#endif

private:
    ToolBarItem& item(const ToolBarItemList& items, const QString& itemId) const;
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    ToolBarItem& item(const ToolBarItemList& items, const actions::ActionCode& actionCode) const;
#endif

    void updateShortcutsAll();
    void updateShortcuts(MenuItem* menuItem);

    ToolBarItemList m_items;

    bool m_isCompactMode = false;
};
}
