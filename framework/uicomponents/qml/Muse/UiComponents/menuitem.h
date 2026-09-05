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

#include <QObject>
#include <QString>

#include "actions/actiontypes.h"
#include "global/async/asyncable.h"

#include "ui/uiaction.h"
#include "rcommand/commandtypes.h"

namespace muse::uicomponents {
// This must be in sync with QAction::MenuRole
enum class MenuItemRole {
    NoRole = 0,
    TextHeuristicRole,
    ApplicationSpecificRole,
    AboutQtRole,
    AboutRole,
    PreferencesRole,
    QuitRole
};
class MenuItem;
using MenuItemList = QList<MenuItem*>;

class MenuItem : public QObject, public async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString code READ code_property NOTIFY itemChanged)

    Q_PROPERTY(QString title READ translatedTitle NOTIFY itemChanged)
    Q_PROPERTY(QString titleWithMnemonicUnderline READ titleWithMnemonicUnderline NOTIFY itemChanged)
    Q_PROPERTY(QString description READ translatedDescription NOTIFY itemChanged)
    Q_PROPERTY(QString shortcuts READ shortcutsTitle NOTIFY shortcutsChanged)
    Q_PROPERTY(QString portableShortcuts READ portableShortcuts NOTIFY shortcutsChanged)

    Q_PROPERTY(int icon READ icon_property NOTIFY itemChanged)
    Q_PROPERTY(QString iconColor READ iconColor_property NOTIFY itemChanged)

    Q_PROPERTY(bool enabled READ enabled NOTIFY stateChanged)
    Q_PROPERTY(bool checkable READ checkable NOTIFY itemChanged)
    Q_PROPERTY(bool checked READ checked NOTIFY stateChanged)
    Q_PROPERTY(bool selectable READ selectable NOTIFY selectableChanged)
    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged)

    Q_PROPERTY(QString section READ section NOTIFY sectionChanged)
    Q_PROPERTY(int role READ role_property NOTIFY roleChanged)
    Q_PROPERTY(muse::uicomponents::MenuItemList subitems READ subitems NOTIFY subitemsChanged)

public:
    MenuItem(QObject* parent = nullptr);

    void setId(const QString& id);
    QString id() const;
    std::string intent() const;
    QString code_property() const;

    void setTitle(const MnemonicString& title);
    QString translatedTitle() const;
    QString titleWithMnemonicUnderline() const;
    void setDescription(const TranslatableString& description);
    QString translatedDescription() const;
    void setShortcuts(const std::vector<std::string>& shortcuts);
    QString shortcutsTitle() const;
    QString portableShortcuts() const;

    void setIcon(ui::IconCode::Code icon);
    int icon_property() const;
    void setIconColor(const QString& iconColor);
    QString iconColor_property() const;

    void setEnabled(bool enabled);
    bool enabled() const;
    void setCheckable(bool checkable);
    bool checkable() const;
    void setChecked(bool checked);
    bool checked() const;
    void setSelectable(bool selectable);
    bool selectable() const;
    void setSelected(bool selected);
    bool selected() const;

    void setSection(const QString& section);
    QString section() const;
    void setRole(MenuItemRole role);
    MenuItemRole role() const;
    int role_property() const;
    void setSubitems(const MenuItemList& subitems);
    MenuItemList subitems() const;

    bool isValid() const;

    // command support
    MenuItem(const rcommand::CommandInfo& info, QObject* parent = nullptr);
    void setCommandInfo(const rcommand::CommandInfo& info);
    rcommand::CommandInfo commandInfo() const;
    void setCommand(const rcommand::Command& command);
    rcommand::Command command() const;
    void setCommandQuery(const rcommand::CommandQuery& query);
    rcommand::CommandQuery commandQuery() const;
    void setCommandState(const rcommand::CommandState& state);
    rcommand::CommandState commandState() const;

    // action support
    MenuItem(const ui::UiAction& action, QObject* parent = nullptr);
    void setActionCode(const actions::ActionCode& code);
    actions::ActionCode actionCode() const;
    void setAction(const muse::ui::UiAction& action);
    ui::UiAction action() const;
    void setState(const muse::ui::UiActionState& state);
    ui::UiActionState state() const;
    void setArgs(const muse::actions::ActionData& args);
    muse::actions::ActionData args() const;
    void setQuery(const muse::actions::ActionQuery& query);
    muse::actions::ActionQuery query() const;

signals:
    void idChanged(QString id);
    void titleChanged(QString title);
    void sectionChanged(QString section);
    void stateChanged();
    void selectableChanged(bool selectable);
    void selectedChanged(bool selected);
    void roleChanged(int role);
    void subitemsChanged(uicomponents::MenuItemList subitems, const QString& menuId);
    void itemChanged();
    void shortcutsChanged();

private:

    QString m_id;
    std::string m_intent;
    MnemonicString m_title;
    TranslatableString m_description;
    std::vector<std::string> m_shortcuts;

    ui::IconCode::Code m_icon = ui::IconCode::Code::NONE;
    QString m_iconColor;

    bool m_enabled = true;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_selectable = false;
    bool m_selected = false;

    QString m_section;
    MenuItemRole m_role = MenuItemRole::NoRole;
    MenuItemList m_subitems;

    muse::actions::ActionData m_args;
};

inline QVariantList menuItemListToVariantList(const uicomponents::MenuItemList& list)
{
    QVariantList result;
    for (MenuItem* item: list) {
        result << QVariant::fromValue(item);
    }

    return result;
}
}
