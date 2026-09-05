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

#include <QtQmlIntegration/qqmlintegration.h>

#include <QAbstractListModel>

#include "async/asyncable.h"
#include "menuitem.h"

#include "modularity/ioc.h"
#include "rcommand/commandtypes.h"
#include "types/uri.h"
#include "ui/iuiactionsregister.h"
#include "shortcuts/ishortcutsregister.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/icommanddispatcher.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"

#include "muse_framework_config.h"

namespace muse::uicomponents {
class AbstractMenuModel : public QAbstractListModel, public muse::Contextable, public async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(int length READ rowCount NOTIFY itemsChanged)
    Q_PROPERTY(QVariantList items READ itemsProperty NOTIFY itemsChanged)

    QML_ELEMENT

public:
    muse::GlobalInject<rcommand::ICommandsRegister> commandsRegister;
    muse::ContextInject<shortcuts::IShortcutsRegister> shortcutsRegister = { this };
    muse::ContextInject<rcommand::ICommandsState> commandsState = { this };
    muse::ContextInject<rcommand::ICommandDispatcher> commandDispatcher = { this };

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    muse::ContextInject<ui::IUiActionsRegister> uiActionsRegister = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
#endif

public:
    explicit AbstractMenuModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    virtual void load();

    QVariantList itemsProperty() const;
    const MenuItemList& items() const;

    Q_INVOKABLE virtual void handleMenuItem(const QString& itemId);
    Q_INVOKABLE QVariantMap get(int index);

signals:
    void itemsChanged();
    void itemChanged(muse::uicomponents::MenuItem* item);

protected:
    enum Roles {
        ItemRole,

        UserRole
    };

    virtual void subscribeOnChanges();
    virtual void onCommandStateChanged(const rcommand::Command& command, const rcommand::CommandState& state);

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    virtual void onActionsStateChanges(const muse::actions::ActionCodeList& codes);
#endif

    void setItem(int index, MenuItem* item);
    void setItems(const MenuItemList& items);
    void clear();

    static const int INVALID_ITEM_INDEX;
    int itemIndex(const QString& itemId) const;

    MenuItem& item(int index);

    MenuItem& findItem(const QString& itemId);
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    MenuItem& findItem(const muse::actions::ActionCode& actionCode);
    MenuItemList findItems(const muse::actions::ActionCode& actionCode);
#endif
    MenuItem& findItem(const muse::rcommand::Command& command);
    MenuItemList findItems(const muse::rcommand::Command& command);
    MenuItem& findMenu(const QString& menuId);

    MenuItem* makeMenu(const TranslatableString& title, const MenuItemList& items, const QString& menuId = "", bool enabled = true);

    MenuItem* makeMenuItem(const muse::rcommand::Command& command, const TranslatableString& title = {});
    MenuItem* makeMenuItem(const muse::rcommand::CommandQuery& query, const TranslatableString& title = {});
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    MenuItem* makeMenuItem(const muse::actions::ActionCode& actionCode, const TranslatableString& title = {});
#endif

    MenuItem* makeSeparator();

    bool isIndexValid(int index) const;
    void dispatch(const std::string& command, const muse::actions::ActionData& args = muse::actions::ActionData());
    void dispatch(const muse::UriQuery& query);

private:
    MenuItem& item(MenuItemList& items, const QString& itemId);
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    MenuItemList items(const MenuItemList& items, const muse::actions::ActionCode& actionCode) const;
#endif
    MenuItemList items(const MenuItemList& items, const muse::rcommand::Command& command) const;
    MenuItem& menu(MenuItemList& items, const QString& menuId);

    void updateState(MenuItemList& items, const rcommand::Command& command, const rcommand::CommandState& state);
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    void updateState(MenuItemList& items, const muse::actions::ActionCodeList& codes, std::map<muse::actions::ActionCode,
                                                                                               muse::ui::UiActionState>& states);
#endif

    void updateShortcutsAll();
    void updateShortcuts(MenuItem* item);

    bool m_subscribedOnChanges = false;
    MenuItemList m_items;
};
}
