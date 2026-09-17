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
#include "abstracttoolbarmodel.h"

#include "types/translatablestring.h"

#include "toolbaritem.h"
#include "menuitem.h"

#include "log.h"

using namespace muse::uicomponents;
using namespace muse::ui;
using namespace muse::actions;

const int AbstractToolBarModel::INVALID_ITEM_INDEX = -1;

AbstractToolBarModel::AbstractToolBarModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

QVariant AbstractToolBarModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();

    if (!isIndexValid(row)) {
        return QVariant();
    }

    ToolBarItem* item = m_items.at(row);

    switch (role) {
    case ItemRole: return QVariant::fromValue(item);
    case UserRole: return QVariant();
    }

    return QVariant();
}

bool AbstractToolBarModel::isIndexValid(int index) const
{
    return index >= 0 && index < m_items.size();
}

int AbstractToolBarModel::rowCount(const QModelIndex&) const
{
    return m_items.count();
}

QHash<int, QByteArray> AbstractToolBarModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { ItemRole, "item" }
    };

    return roles;
}

QVariantMap AbstractToolBarModel::get(int index)
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

void AbstractToolBarModel::load()
{
    commandsState()->commandStateChanged().onReceive(this, [this](const rcommand::Command& command, const rcommand::CommandState& state) {
        onCommandStateChanged(command, state);
    });

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    uiActionsRegister()->actionStateChanged().onReceive(this, [this](const ActionCodeList& codes) {
        onActionsStateChanges(codes);
    }, async::Asyncable::Mode::SetReplace);
#endif

    shortcutsRegister()->shortcutsChanged().onNotify(this, [this]() {
        updateShortcutsAll();
    }, async::Asyncable::Mode::SetReplace);
}

void AbstractToolBarModel::onCommandStateChanged(const rcommand::Command& command, const rcommand::CommandState& state)
{
    updateState(m_items, command, state);
}

void AbstractToolBarModel::updateState(ToolBarItemList& items, const rcommand::Command& command, const rcommand::CommandState& state)
{
    for (ToolBarItem* toolBarItem : items) {
        if (!toolBarItem) {
            continue;
        }

        if (command == toolBarItem->command()) {
            toolBarItem->setCommandState(state);
        }

        QList<MenuItem*> subitems = toolBarItem->menuItems();
        if (!subitems.empty()) {
            updateState(subitems, command, state);
        }
    }
}

void AbstractToolBarModel::updateState(QList<MenuItem*>& items, const rcommand::Command& command, const rcommand::CommandState& state)
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

ToolBarItem* AbstractToolBarModel::makeItem(const rcommand::Command& command, const TranslatableString& title)
{
    const rcommand::CommandInfo& info = commandsRegister()->commandInfo(command);
    if (!info.isValid()) {
        LOGW() << "not found command: " << command;
        return nullptr;
    }

    ToolBarItem* item = new ToolBarItem(info, ToolBarItemType::ACTION, this);
    item->setCommandState(commandsState()->commandState(command));

    if (!title.isEmpty()) {
        item->setTitle(title);
    }

    return item;
}

ToolBarItem& AbstractToolBarModel::findItem(const rcommand::Command& command) const
{
    if (ToolBarItem* toolBarItem = findItemPtr(command)) {
        return *toolBarItem;
    }

    static ToolBarItem dummy;
    return dummy;
}

ToolBarItem* AbstractToolBarModel::findItemPtr(const rcommand::Command& command) const
{
    for (ToolBarItem* toolBarItem : std::as_const(m_items)) {
        if (toolBarItem->command() == command) {
            return toolBarItem;
        }
    }

    return nullptr;
}

QVariantList AbstractToolBarModel::itemsProperty() const
{
    QVariantList items;

    for (ToolBarItem* item: m_items) {
        items << QVariant::fromValue(item);
    }

    return items;
}

const ToolBarItemList& AbstractToolBarModel::items() const
{
    return m_items;
}

void AbstractToolBarModel::setItems(const ToolBarItemList& items)
{
    TRACEFUNC;

    beginResetModel();

    qDeleteAll(m_items);
    m_items.clear();

    //! NOTE: make sure that we don't have two separators sequentially
    bool isPreviousSeparator = false;

    for (ToolBarItem* item : items) {
        if (item->type() == ToolBarItemType::SEPARATOR) {
            if (isPreviousSeparator) {
                delete item;
                continue;
            }

            isPreviousSeparator = true;
        } else {
            isPreviousSeparator = false;
        }

        m_items << item;
    }

    updateShortcutsAll();

    endResetModel();

    emit itemsChanged();
}

void AbstractToolBarModel::clear()
{
    setItems(ToolBarItemList());
}

int AbstractToolBarModel::itemIndex(const QString& itemId) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->id() == itemId) {
            return i;
        }
    }

    return INVALID_ITEM_INDEX;
}

ToolBarItem& AbstractToolBarModel::item(int index)
{
    ToolBarItem& item = *m_items[index];
    if (item.isValid()) {
        return item;
    }

    static ToolBarItem dummy;
    return dummy;
}

ToolBarItem& AbstractToolBarModel::findItem(const QString& itemId)
{
    return item(m_items, itemId);
}

ToolBarItem* AbstractToolBarModel::findItemPtr(const QString& itemId)
{
    for (ToolBarItem* toolBarItem : std::as_const(m_items)) {
        if (toolBarItem->id() == itemId) {
            return toolBarItem;
        }
    }

    return nullptr;
}

ToolBarItem* AbstractToolBarModel::makeSeparator()
{
    return new ToolBarItem(ToolBarItemType::SEPARATOR, this);
}

void AbstractToolBarModel::setItem(int index, ToolBarItem* item)
{
    if (!isIndexValid(index)) {
        return;
    }

    m_items[index] = item;

    QModelIndex modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex);
}

ToolBarItem& AbstractToolBarModel::item(const ToolBarItemList& items, const QString& itemId) const
{
    for (ToolBarItem* toolBarItem : items) {
        if (toolBarItem->id() == itemId) {
            return *toolBarItem;
        }
    }

    static ToolBarItem dummy;
    return dummy;
}

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
ToolBarItem* AbstractToolBarModel::makeItem(const ActionCode& actionCode, const TranslatableString& title)
{
    const UiAction& action = uiActionsRegister()->action(actionCode);
    if (!action.isValid()) {
        LOGW() << "not found action: " << actionCode;
        return nullptr;
    }

    ToolBarItem* item = new ToolBarItem(action, ToolBarItemType::ACTION, this);
    item->setState(uiActionsRegister()->actionState(actionCode));

    if (!title.isEmpty()) {
        item->setTitle(title);
    }

    return item;
}

ToolBarItem* AbstractToolBarModel::makeMenuItem(const TranslatableString& title, const ActionCodeList& subitemsActionCodesList,
                                                const QString& menuId, bool enabled)
{
    ToolBarItem* item = new ToolBarItem(this);
    item->setId(menuId);
    item->setTitle(title);
    item->setEnabled(enabled);

    MenuItemList subitems;
    for (const ActionCode& subitemActionCode: subitemsActionCodesList) {
        const UiAction& action = uiActionsRegister()->action(subitemActionCode);
        if (!action.isValid()) {
            LOGW() << "not found action: " << subitemActionCode;
            continue;
        }

        MenuItem* subitem = new MenuItem(action, this);
        subitem->setState(uiActionsRegister()->actionState(subitemActionCode));

        updateShortcuts(subitem);

        subitems << subitem;
    }
    item->setMenuItems(subitems);

    return item;
}

ToolBarItem& AbstractToolBarModel::item(const ToolBarItemList& items, const ActionCode& actionCode) const
{
    for (ToolBarItem* toolBarItem : items) {
        if (!toolBarItem) {
            continue;
        }

        if (toolBarItem->actionCode() == actionCode) {
            return *toolBarItem;
        }
    }

    static ToolBarItem dummy;
    return dummy;
}

ToolBarItem& AbstractToolBarModel::findItem(const ActionCode& actionCode)
{
    return item(m_items, actionCode);
}

ToolBarItem* AbstractToolBarModel::findItemPtr(const actions::ActionCode& actionCode)
{
    for (ToolBarItem* toolBarItem : std::as_const(m_items)) {
        if (toolBarItem->actionCode() == actionCode) {
            return toolBarItem;
        }
    }

    return nullptr;
}

void AbstractToolBarModel::onActionsStateChanges(const muse::actions::ActionCodeList& codes)
{
    if (codes.empty()) {
        return;
    }

    for (const ActionCode& code : codes) {
        ToolBarItem& actionItem = findItem(code);
        if (actionItem.isValid()) {
            actionItem.setState(uiActionsRegister()->actionState(code));
        }
    }
}

#endif // MUSE_MODULE_ACTIONS_SUPPORT

bool AbstractToolBarModel::isCompactMode() const
{
    return m_isCompactMode;
}

void AbstractToolBarModel::setIsCompactMode(bool isCompactMode)
{
    if (m_isCompactMode == isCompactMode) {
        return;
    }

    for (ToolBarItem* item : std::as_const(m_items)) {
        if (item) {
            item->setShowTitle(!isCompactMode);
        }
    }

    m_isCompactMode = isCompactMode;
    emit isCompactModeChanged();
}

void AbstractToolBarModel::updateShortcutsAll()
{
    for (ToolBarItem* toolBarItem : std::as_const(m_items)) {
        if (!toolBarItem) {
            continue;
        }

        std::vector<std::string> shortcuts;
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
        shortcuts = shortcutsRegister()->shortcut(toolBarItem->actionCode()).sequences;
#endif
        toolBarItem->setShortcuts(shortcuts);

        for (MenuItem* menuItem : std::as_const(toolBarItem->menuItems())) {
            if (!menuItem) {
                continue;
            }

            updateShortcuts(menuItem);
        }
    }
}

void AbstractToolBarModel::updateShortcuts(MenuItem* menuItem)
{
    std::vector<std::string> shortcuts;
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    shortcuts = shortcutsRegister()->shortcut(menuItem->actionCode()).sequences;
#endif
    menuItem->setShortcuts(shortcuts);

    for (MenuItem* subItem : menuItem->subitems()) {
        if (!subItem) {
            continue;
        }

        updateShortcuts(subItem);
    }
}
