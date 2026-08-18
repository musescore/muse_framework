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
#include "menuitem.h"

#include <QVariantMap>

#include "types/color.h"
#include "types/translatablestring.h"
#include "shortcuts/shortcutstypes.h"

using namespace muse::uicomponents;
using namespace muse::ui;

MenuItem::MenuItem(QObject* parent)
    : QObject(parent)
{
}

void MenuItem::setId(const QString& id)
{
    if (m_id == id) {
        return;
    }

    m_id = id;
    emit idChanged(m_id);
}

QString MenuItem::id() const
{
    return m_id;
}

std::string MenuItem::intent() const
{
    return m_intent;
}

QString MenuItem::code_property() const
{
    return QString::fromStdString(m_intent);
}

void MenuItem::setTitle(const MnemonicString& title)
{
    if (m_title == title) {
        return;
    }

    m_title = title;
    emit itemChanged();
}

QString MenuItem::translatedTitle() const
{
    return m_title.qTranslatedWithoutMnemonic();
}

QString MenuItem::titleWithMnemonicUnderline() const
{
    return m_title.qTranslatedWithMnemonicUnderline();
}

void MenuItem::setDescription(const TranslatableString& description)
{
    if (m_description == description) {
        return;
    }

    m_description = description;
    emit itemChanged();
}

QString MenuItem::translatedDescription() const
{
    return m_description.qTranslated();
}

void MenuItem::setShortcuts(const std::vector<std::string>& shortcuts)
{
    if (m_shortcuts == shortcuts) {
        return;
    }

    m_shortcuts = shortcuts;
    emit shortcutsChanged();
}

QString MenuItem::shortcutsTitle() const
{
    return shortcuts::sequencesToNativeText(m_shortcuts);
}

QString MenuItem::portableShortcuts() const
{
    return QString::fromStdString(shortcuts::Shortcut::sequencesToString(m_shortcuts));
}

void MenuItem::setIcon(ui::IconCode::Code icon)
{
    if (m_icon == icon) {
        return;
    }

    m_icon = icon;
    emit itemChanged();
}

int MenuItem::icon_property() const
{
    return static_cast<int>(m_icon);
}

void MenuItem::setIconColor(const QString& iconColor)
{
    if (m_iconColor == iconColor) {
        return;
    }

    m_iconColor = iconColor;
    emit itemChanged();
}

QString MenuItem::iconColor_property() const
{
    return m_iconColor;
}

void MenuItem::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit stateChanged();
}

bool MenuItem::enabled() const
{
    return m_enabled;
}

void MenuItem::setCheckable(bool checkable)
{
    if (m_checkable == checkable) {
        return;
    }

    m_checkable = checkable;
    emit itemChanged();
}

bool MenuItem::checkable() const
{
    return m_checkable;
}

void MenuItem::setChecked(bool checked)
{
    if (m_checked == checked) {
        return;
    }

    m_checked = checked;
    emit stateChanged();
}

bool MenuItem::checked() const
{
    return m_checked;
}

void MenuItem::setSelectable(bool selectable)
{
    if (m_selectable == selectable) {
        return;
    }

    m_selectable = selectable;
    emit selectableChanged(m_selectable);
}

bool MenuItem::selectable() const
{
    return m_selectable;
}

void MenuItem::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }

    m_selected = selected;
    emit selectedChanged(m_selected);
}

void MenuItem::setSection(const QString& section)
{
    if (m_section == section) {
        return;
    }

    m_section = section;
    emit sectionChanged(m_section);
}

bool MenuItem::selected() const
{
    return m_selected;
}

QString MenuItem::section() const
{
    return m_section;
}

void MenuItem::setRole(MenuItemRole role)
{
    if (m_role == role) {
        return;
    }

    m_role = role;
    emit roleChanged(role_property());
}

MenuItemRole MenuItem::role() const
{
    return m_role;
}

int MenuItem::role_property() const
{
    return static_cast<int>(m_role);
}

void MenuItem::setSubitems(const QList<MenuItem*>& subitems)
{
    if (m_subitems == subitems) {
        return;
    }

    m_subitems = subitems;
    emit subitemsChanged(m_subitems, m_id);
}

MenuItemList MenuItem::subitems() const
{
    return m_subitems;
}

bool MenuItem::isValid() const
{
    return !m_id.isEmpty();
}

// command support
MenuItem::MenuItem(const rcommand::CommandInfo& info, QObject* parent)
    : QObject(parent)
{
    setCommandInfo(info);
}

void MenuItem::setCommandInfo(const rcommand::CommandInfo& info)
{
    m_intent = info.command.toString();
    setId(QString::fromStdString(m_intent));

    m_title = info.title;
    m_description = info.description;
    m_icon = info.decoration.iconCode;
    m_iconColor = QString::fromStdString(info.decoration.iconColor.toString());
    m_checkable = info.decoration.checkable == rcommand::Checkable::Yes;

    emit itemChanged();
}

muse::rcommand::CommandInfo MenuItem::commandInfo() const
{
    rcommand::CommandInfo info;
    info.command = rcommand::Command(m_intent);
    info.title = m_title;
    info.description = m_description;
    info.decoration.iconCode = m_icon;
    info.decoration.iconColor = Color::fromString(m_iconColor.toStdString());
    info.decoration.checkable = m_checkable ? rcommand::Checkable::Yes : rcommand::Checkable::No;
    return info;
}

void MenuItem::setCommand(const rcommand::Command& command)
{
    m_intent = command.toString();
    setId(QString::fromStdString(m_intent));
}

muse::rcommand::Command MenuItem::command() const
{
    return rcommand::Command(m_intent);
}

void MenuItem::setCommandQuery(const rcommand::CommandQuery& query)
{
    m_intent = query.toString();
    setId(QString::fromStdString(m_intent));
}

muse::rcommand::CommandQuery MenuItem::commandQuery() const
{
    return rcommand::CommandQuery(m_intent);
}

void MenuItem::setCommandState(const rcommand::CommandState& state)
{
    if (m_enabled == state.enabled && m_checked == state.checked) {
        return;
    }
    m_enabled = state.enabled;
    m_checked = state.checked;
    emit stateChanged();
}

muse::rcommand::CommandState MenuItem::commandState() const
{
    return rcommand::CommandState(m_enabled, m_checked);
}

// action support
MenuItem::MenuItem(const UiAction& action, QObject* parent)
    : QObject(parent)
{
    m_id = QString::fromStdString(action.code);
    setAction(action);
}

void MenuItem::setActionCode(const actions::ActionCode& code)
{
    m_intent = code;
}

muse::actions::ActionCode MenuItem::actionCode() const
{
    return m_intent;
}

void MenuItem::setAction(const UiAction& action)
{
    m_intent = action.code;
    m_title = action.title;
    m_description = action.description;
    m_icon = action.iconCode;
    m_iconColor = action.iconColor;
    m_checkable = action.checkable == ui::Checkable::Yes;

    emit itemChanged();
}

UiAction MenuItem::action() const
{
    UiAction action;
    action.code = m_intent;
    action.title = m_title;
    action.description = m_description;
    action.iconCode = m_icon;
    action.iconColor = m_iconColor;
    action.checkable = m_checkable ? ui::Checkable::Yes : ui::Checkable::No;
    return action;
}

void MenuItem::setState(const UiActionState& state)
{
    if (m_enabled == state.enabled && m_checked == state.checked) {
        return;
    }

    m_enabled = state.enabled;
    m_checked = state.checked;
    emit stateChanged();
}

UiActionState MenuItem::state() const
{
    return UiActionState { m_enabled, m_checked };
}

void MenuItem::setArgs(const muse::actions::ActionData& args)
{
    m_args = args;
}

muse::actions::ActionData MenuItem::args() const
{
    return m_args;
}

void MenuItem::setQuery(const muse::actions::ActionQuery& query)
{
    m_intent = query.toString();
}

muse::actions::ActionQuery MenuItem::query() const
{
    return muse::actions::ActionQuery(m_intent);
}
