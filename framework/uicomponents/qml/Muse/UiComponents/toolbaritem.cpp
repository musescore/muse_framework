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
#include "toolbaritem.h"

#include <QVariantMap>

#include "global/stringutils.h"
#include "types/translatablestring.h"
#include "shortcuts/shortcutstypes.h"

using namespace muse::uicomponents;
using namespace muse::ui;

ToolBarItem::ToolBarItem(QObject* parent)
    : QObject(parent), Contextable(iocCtxForQmlObject(this))
{
}

ToolBarItem::ToolBarItem(ToolBarItemType::Type type, QObject* parent)
    : QObject(parent), Contextable(iocCtxForQmlObject(this))
{
    m_type = type;
}

QString ToolBarItem::id() const
{
    return m_id;
}

QString ToolBarItem::translatedTitle() const
{
    return m_title.qTranslatedWithoutMnemonic();
}

bool ToolBarItem::enabled() const
{
    return m_enabled;
}

void ToolBarItem::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit stateChanged();
}

void ToolBarItem::activate()
{
    if (strings::startsWith(m_intent, "command://")) {
        commandDispatcher()->dispatch(command(), m_params);
    } else {
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
        dispatcher()->dispatch(actionCode(), m_args);
#endif
    }
}

void ToolBarItem::handleMenuItem(const QString& menuId)
{
    for (const MenuItem* menuItem : m_menuItems) {
        if (menuItem->id() == menuId) {
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
            dispatcher()->dispatch(menuItem->actionCode(), menuItem->args());
#endif
            return;
        }
    }
}

bool ToolBarItem::selected() const
{
    return m_selected;
}

ToolBarItemType::Type ToolBarItem::type() const
{
    return m_type;
}

const QList<MenuItem*>& ToolBarItem::menuItems() const
{
    return m_menuItems;
}

bool ToolBarItem::isValid() const
{
    return !m_id.isEmpty();
}

QString ToolBarItem::shortcutsTitle() const
{
    return shortcuts::sequencesToNativeText(m_shortcuts);
}

void ToolBarItem::setId(const QString& id)
{
    if (m_id == id) {
        return;
    }

    m_id = id;
    emit idChanged(m_id);
}

void ToolBarItem::setTitle(const TranslatableString& title)
{
    if (m_title == title) {
        return;
    }

    m_title = title;
    emit itemChanged();
}

void ToolBarItem::setDescription(const TranslatableString& description)
{
    if (m_description == description) {
        return;
    }

    m_description = description;
    emit itemChanged();
}

void ToolBarItem::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }

    m_selected = selected;
    emit selectedChanged(m_selected);
}

void ToolBarItem::setType(ToolBarItemType::Type type)
{
    if (m_type == type) {
        return;
    }

    m_type = type;
    emit typeChanged(type_property());
}

void ToolBarItem::setMenuItems(const QList<MenuItem*>& menuItems)
{
    if (m_menuItems == menuItems) {
        return;
    }

    m_menuItems = menuItems;
    emit menuItemsChanged(m_menuItems, m_id);
}

void ToolBarItem::setShortcuts(const std::vector<std::string>& shortcuts)
{
    if (m_shortcuts == shortcuts) {
        return;
    }

    m_shortcuts = shortcuts;
    emit shortcutsChanged();
}

QString muse::uicomponents::ToolBarItem::code_property() const
{
    return QString::fromStdString(m_intent);
}

QString ToolBarItem::description_property() const
{
    return m_description.qTranslated();
}

int ToolBarItem::icon_property() const
{
    return static_cast<int>(m_icon);
}

bool ToolBarItem::checkable_property() const
{
    return m_checkable;
}

bool ToolBarItem::checked_property() const
{
    return m_checked;
}

bool ToolBarItem::selected_property() const
{
    return m_selected;
}

int ToolBarItem::type_property() const
{
    return static_cast<int>(m_type);
}

bool ToolBarItem::isMenuSecondary() const
{
    return m_isMenuSecondary;
}

void ToolBarItem::setIsMenuSecondary(bool secondary)
{
    if (m_isMenuSecondary == secondary) {
        return;
    }

    m_isMenuSecondary = secondary;
    emit isMenuSecondaryChanged(secondary);
}

bool ToolBarItem::showTitle() const
{
    return m_showTitle;
}

void ToolBarItem::setShowTitle(bool show)
{
    if (m_showTitle == show) {
        return;
    }

    m_showTitle = show;
    emit showTitleChanged();
}

bool ToolBarItem::isTitleBold() const
{
    return m_isTitleBold;
}

void ToolBarItem::setIsTitleBold(bool newIsTitleBold)
{
    if (m_isTitleBold == newIsTitleBold) {
        return;
    }

    m_isTitleBold = newIsTitleBold;
    emit isTitleBoldChanged();
}

bool ToolBarItem::isTransparent() const
{
    return m_isTransparent;
}

void ToolBarItem::setIsTransparent(bool isTransparent)
{
    if (m_isTransparent == isTransparent) {
        return;
    }

    m_isTransparent = isTransparent;
    emit isTransparentChanged();
}

// command support
ToolBarItem::ToolBarItem(const rcommand::CommandInfo& info, ToolBarItemType::Type type, QObject* parent)
    : QObject(parent), Contextable(iocCtxForQmlObject(this))
{
    m_type = type;
    setCommandInfo(info);
}

void ToolBarItem::setCommandInfo(const rcommand::CommandInfo& info)
{
    m_intent = info.command.toString();
    setId(QString::fromStdString(m_intent));

    m_title = info.title;
    m_description = info.description;
    m_icon = info.decoration.iconCode;
    m_checkable = info.decoration.checkable == rcommand::Checkable::Yes;

    emit itemChanged();
}

muse::rcommand::CommandInfo ToolBarItem::commandInfo() const
{
    rcommand::CommandInfo info;
    info.command = rcommand::Command(m_intent);
    info.title = m_title;
    info.description = m_description;
    info.decoration.iconCode = m_icon;
    info.decoration.checkable = m_checkable ? rcommand::Checkable::Yes : rcommand::Checkable::No;
    return info;
}

void ToolBarItem::setCommand(const rcommand::Command& command)
{
    m_intent = command.toString();
    setId(QString::fromStdString(m_intent));
}

muse::rcommand::Command ToolBarItem::command() const
{
    return rcommand::Command(m_intent);
}

void ToolBarItem::setParams(const rcommand::Params& params)
{
    m_params = params;
}

muse::rcommand::Params ToolBarItem::params() const
{
    return m_params;
}

void ToolBarItem::setCommandState(const rcommand::CommandState& state)
{
    if (m_enabled == state.enabled && m_checked == state.checked) {
        return;
    }
    m_enabled = state.enabled;
    m_checked = state.checked;
    emit stateChanged();
}

muse::rcommand::CommandState ToolBarItem::commandState() const
{
    return rcommand::CommandState(m_enabled, m_checked);
}

// action support

#ifdef MUSE_MODULE_ACTIONS_SUPPORT

ToolBarItem::ToolBarItem(const UiAction& action, ToolBarItemType::Type type, QObject* parent)
    : QObject(parent), Contextable(iocCtxForQmlObject(this))
{
    m_id = QString::fromStdString(action.code);
    m_type = type;

    setAction(action);
}

void ToolBarItem::setAction(const UiAction& action)
{
    m_intent = action.code;
    m_title = action.title;
    m_description = action.description;
    m_icon = action.iconCode;
    m_checkable = action.checkable == ui::Checkable::Yes;

    emit itemChanged();
}

UiAction ToolBarItem::action() const
{
    UiAction action;
    action.code = m_intent;
    action.title = m_title;
    action.description = m_description;
    action.iconCode = m_icon;
    action.checkable = m_checkable ? ui::Checkable::Yes : ui::Checkable::No;
    return action;
}

muse::actions::ActionCode ToolBarItem::actionCode() const
{
    return m_intent;
}

void ToolBarItem::setState(const UiActionState& state)
{
    if (m_enabled == state.enabled && m_checked == state.checked) {
        return;
    }

    m_enabled = state.enabled;
    m_checked = state.checked;
    emit stateChanged();
}

UiActionState ToolBarItem::state() const
{
    return { m_enabled, m_checked };
}

void ToolBarItem::setArgs(const muse::actions::ActionData& args)
{
    m_args = args;
}

muse::actions::ActionData ToolBarItem::args() const
{
    return m_args;
}

#endif
