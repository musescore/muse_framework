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
#include "abstractmenumodel.h"

#include "rcommand/commandtypes.h"
#include "thirdparty/kors_logger/src/log_base.h"
#include "types/translatablestring.h"

#include "log.h"

using namespace muse::uicomponents;
using namespace muse::ui;
using namespace muse::actions;

const int AbstractMenuModel::INVALID_ITEM_INDEX = -1;

AbstractMenuModel::AbstractMenuModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

QVariant AbstractMenuModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();

    if (!isIndexValid(row)) {
        return QVariant();
    }

    MenuItem* item = m_items.at(row);

    switch (role) {
    case ItemRole: return QVariant::fromValue(item);
    case UserRole: return QVariant();
    }

    return QVariant();
}

bool AbstractMenuModel::isIndexValid(int index) const
{
    return index >= 0 && index < m_items.size();
}

int AbstractMenuModel::rowCount(const QModelIndex&) const
{
    return m_items.count();
}

QHash<int, QByteArray> AbstractMenuModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { ItemRole, "item" }
    };

    return roles;
}

void AbstractMenuModel::handleMenuItem(const QString& itemId)
{
    MenuItem& menuItem = findItem(itemId);

    std::string intent = menuItem.intent();
    UriQuery query(intent);
    if (query.isValid()) {
        dispatch(query);
    } else {
        dispatch(intent, menuItem.args());
    }
}

void AbstractMenuModel::dispatch(const std::string& command, const ActionData& args)
{
    if (muse::strings::startsWith(command, "command://")) {
        DO_ASSERT(args.empty());
        commandDispatcher()->dispatch(rcommand::Command(command));
    } else {
        dispatcher()->dispatch(command, args);
    }
}

void AbstractMenuModel::dispatch(const UriQuery& query)
{
    if (query.uri().scheme() == "command") {
        commandDispatcher()->dispatch(query);
    } else {
        dispatcher()->dispatch(query);
    }
}

QVariantMap AbstractMenuModel::get(int index)
{
    QVariantMap result;

    QHash<int, QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> i(names);
    while (i.hasNext()) {
        i.next();
        QModelIndex idx = this->index(index, 0);
        QVariant data = idx.data(i.key());
        result[i.value()] = data;
    }

    return result;
}

void AbstractMenuModel::load()
{
    subscribeOnChanges();
}

QVariantList AbstractMenuModel::itemsProperty() const
{
    QVariantList items;

    for (MenuItem* item: m_items) {
        items << QVariant::fromValue(item);
    }

    return items;
}

const MenuItemList& AbstractMenuModel::items() const
{
    return m_items;
}

void AbstractMenuModel::setItems(const MenuItemList& items)
{
    TRACEFUNC;

    beginResetModel();
    m_items = items;
    updateShortcutsAll();
    endResetModel();

    emit itemsChanged();
}

void AbstractMenuModel::clear()
{
    setItems(MenuItemList());
}

int AbstractMenuModel::itemIndex(const QString& itemId) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->id() == itemId) {
            return i;
        }
    }

    return INVALID_ITEM_INDEX;
}

MenuItem& AbstractMenuModel::item(int index)
{
    MenuItem& item = *m_items[index];
    if (item.isValid()) {
        return item;
    }

    static MenuItem dummy;
    return dummy;
}

MenuItem& AbstractMenuModel::findItem(const QString& itemId)
{
    return item(m_items, itemId);
}

MenuItem& AbstractMenuModel::findItem(const ActionCode& actionCode)
{
    MenuItemList list = items(m_items, actionCode);
    if (list.empty()) {
        static MenuItem dummy;
        return dummy;
    }

    if (list.size() > 1) {
        LOGD() << "There is more than one item for " << actionCode << ", will return the first one found";
    }

    return *list.front();
}

MenuItemList AbstractMenuModel::findItems(const ActionCode& actionCode)
{
    return items(m_items, actionCode);
}

MenuItem& AbstractMenuModel::findItem(const muse::rcommand::Command& command)
{
    MenuItemList list = items(m_items, command);
    if (list.empty()) {
        static MenuItem dummy;
        return dummy;
    }

    if (list.size() > 1) {
        LOGD() << "There is more than one item for " << command << ", will return the first one found";
    }

    return *list.front();
}

MenuItemList AbstractMenuModel::findItems(const muse::rcommand::Command& command)
{
    return items(m_items, command);
}

MenuItem& AbstractMenuModel::findMenu(const QString& menuId)
{
    return menu(m_items, menuId);
}

MenuItem* AbstractMenuModel::makeMenu(const TranslatableString& title, const MenuItemList& items,
                                      const QString& menuId, bool enabled)
{
    MenuItem* item = new MenuItem(this);
    item->setId(menuId);
    item->setTitle(title);
    item->setSubitems(items);
    item->setEnabled(enabled);

    updateShortcuts(item);

    return item;
}

MenuItem* AbstractMenuModel::makeMenuItem(const rcommand::Command& command, const TranslatableString& title)
{
    return makeMenuItem(rcommand::CommandQuery(command), title);
}

MenuItem* AbstractMenuModel::makeMenuItem(const rcommand::CommandQuery& query, const TranslatableString& title)
{
    const rcommand::CommandInfo& info = commandsRegister()->commandInfo(query.uri());
    if (!info.isValid()) {
        LOGW() << "not found command: " << query.uri().toString();
        return nullptr;
    }

    MenuItem* item = new MenuItem(info, this);
    item->setCommandQuery(query);
    item->setCommandState(commandsState()->commandState(query.uri()));

    if (!title.isEmpty()) {
        item->setTitle(title);
    }

    return item;
}

MenuItem* AbstractMenuModel::makeMenuItem(const ActionCode& actionCode, const TranslatableString& title)
{
    if (muse::strings::startsWith(actionCode, "command://")) {
        return makeMenuItem(rcommand::CommandQuery(actionCode), title);
    }

    const UiAction& action = uiActionsRegister()->action(actionCode);
    if (!action.isValid()) {
        LOGW() << "not found action: " << actionCode;
        return nullptr;
    }

    MenuItem* item = new MenuItem(action, this);
    item->setState(uiActionsRegister()->actionState(actionCode));

    if (!title.isEmpty()) {
        item->setTitle(title);
    }

    ActionQuery q(actionCode);
    if (q.isValid()) {
        item->setQuery(q);
    }

    return item;
}

MenuItem* AbstractMenuModel::makeSeparator()
{
    MenuItem* item = new MenuItem(this);
    item->setTitle({});
    return item;
}

void AbstractMenuModel::subscribeOnChanges()
{
    if (m_subscribedOnChanges) {
        return;
    }

    commandsState()->commandStateChanged().onReceive(this, [this](const rcommand::Command& command, const rcommand::CommandState& state) {
        onCommandStateChanged(command, state);
    });

    uiActionsRegister()->actionStateChanged().onReceive(this, [this](const ActionCodeList& codes) {
        onActionsStateChanges(codes);
    });

    shortcutsRegister()->shortcutsChanged().onNotify(this, [this]() {
        updateShortcutsAll();
    });

    m_subscribedOnChanges = true;
}

void AbstractMenuModel::onCommandStateChanged(const rcommand::Command& command, const rcommand::CommandState& state)
{
    updateState(m_items, command, state);
}

void AbstractMenuModel::onActionsStateChanges(const muse::actions::ActionCodeList& codes)
{
    TRACEFUNC;

    if (codes.empty()) {
        return;
    }

    std::map<actions::ActionCode, ui::UiActionState> states;
    updateState(m_items, codes, states);
}

void AbstractMenuModel::setItem(int index, MenuItem* item)
{
    if (!isIndexValid(index)) {
        return;
    }

    m_items[index] = item;

    QModelIndex modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex);
}

MenuItem& AbstractMenuModel::item(MenuItemList& items, const QString& itemId)
{
    for (MenuItem* menuItem : items) {
        if (!menuItem) {
            continue;
        }
        if (menuItem->id() == itemId) {
            return *menuItem;
        }

        auto subitems = menuItem->subitems();
        if (!subitems.empty()) {
            MenuItem& subitem = item(subitems, itemId);
            if (subitem.id() == itemId) {
                return subitem;
            }
        }
    }

    static MenuItem dummy;
    return dummy;
}

MenuItemList AbstractMenuModel::items(const MenuItemList& items, const ActionCode& actionCode) const
{
    MenuItemList result;

    for (MenuItem* menuItem : items) {
        if (!menuItem) {
            continue;
        }

        if (menuItem->actionCode() == actionCode) {
            result.append(menuItem);
        }

        auto subitems = menuItem->subitems();
        if (!subitems.empty()) {
            MenuItemList list = this->items(subitems, actionCode);
            if (!list.empty()) {
                result.append(list);
            }
        }
    }

    return result;
}

MenuItemList AbstractMenuModel::items(const MenuItemList& items, const muse::rcommand::Command& command) const
{
    MenuItemList result;

    for (MenuItem* menuItem : items) {
        if (!menuItem) {
            continue;
        }

        if (menuItem->command() == command) {
            result.append(menuItem);
        }

        auto subitems = menuItem->subitems();
        if (!subitems.empty()) {
            MenuItemList list = this->items(subitems, command);
            if (!list.empty()) {
                result.append(list);
            }
        }
    }

    return result;
}

MenuItem& AbstractMenuModel::menu(MenuItemList& items, const QString& menuId)
{
    for (MenuItem* item : items) {
        if (!item) {
            continue;
        }

        if (item->id() == menuId) {
            return *item;
        }

        auto subitems = item->subitems();
        MenuItem& menuItem = menu(subitems, menuId);
        if (menuItem.isValid()) {
            return menuItem;
        }
    }

    static MenuItem dummy;
    return dummy;
}

void AbstractMenuModel::updateState(MenuItemList& items, const rcommand::Command& command, const rcommand::CommandState& state)
{
    for (MenuItem* menuItem : items) {
        if (!menuItem) {
            continue;
        }

        if (command == menuItem->command()) {
            menuItem->setCommandState(state);
        }

        MenuItemList subitems = menuItem->subitems();
        if (!subitems.empty()) {
            updateState(subitems, command, state);
        }
    }
}

void AbstractMenuModel::updateState(MenuItemList& items, const actions::ActionCodeList& codes,
                                    std::map<actions::ActionCode, ui::UiActionState>& states)
{
    for (MenuItem* menuItem : items) {
        if (!menuItem) {
            continue;
        }

        ActionCode code = menuItem->actionCode();
        if (muse::contains(codes, code)) {
            if (!muse::contains(states, code)) {
                states.insert({ code, uiActionsRegister()->actionState(code) });
            }
            menuItem->setState(states.at(code));
        }

        MenuItemList subitems = menuItem->subitems();
        if (!subitems.empty()) {
            updateState(subitems, codes, states);
        }
    }
}

void AbstractMenuModel::updateShortcutsAll()
{
    for (MenuItem* menuItem : m_items) {
        if (!menuItem) {
            continue;
        }

        updateShortcuts(menuItem);
    }
}

void AbstractMenuModel::updateShortcuts(MenuItem* item)
{
    std::vector<std::string> shortcuts = shortcutsRegister()->shortcut(item->actionCode()).sequences;
    item->setShortcuts(shortcuts);

    for (MenuItem* subItem : item->subitems()) {
        if (!subItem) {
            continue;
        }

        updateShortcuts(subItem);
    }
}
