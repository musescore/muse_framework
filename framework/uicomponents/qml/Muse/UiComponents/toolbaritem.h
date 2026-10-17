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

#include <QObject>
#include <QString>

#include "global/async/asyncable.h"

#include "uicomponents/qml/Muse/UiComponents/menuitem.h"

#include "abstracttoolbarmodel.h"

#include "modularity/ioc.h"

#include "rcommand/commandtypes.h"
#include "rcommand/icommanddispatcher.h"

#include "muse_framework_config.h"

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
#include "ui/uiaction.h"
#include "actions/iactionsdispatcher.h"
#endif

namespace muse::uicomponents {
class ToolBarItem : public QObject, public Contextable, public async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT;

    Q_PROPERTY(QString id READ id NOTIFY idChanged)

    Q_PROPERTY(QString code READ code_property NOTIFY itemChanged)
    Q_PROPERTY(QString shortcuts READ shortcutsTitle NOTIFY shortcutsChanged)

    Q_PROPERTY(QString title READ translatedTitle NOTIFY itemChanged)
    Q_PROPERTY(bool showTitle READ showTitle WRITE setShowTitle NOTIFY showTitleChanged)
    Q_PROPERTY(bool isTitleBold READ isTitleBold WRITE setIsTitleBold NOTIFY isTitleBoldChanged)

    Q_PROPERTY(QString description READ description_property NOTIFY itemChanged)

    Q_PROPERTY(int icon READ icon_property NOTIFY itemChanged)

    Q_PROPERTY(bool enabled READ enabled NOTIFY stateChanged)

    Q_PROPERTY(bool checkable READ checkable_property NOTIFY itemChanged)
    Q_PROPERTY(bool checked READ checked_property NOTIFY stateChanged)

    Q_PROPERTY(bool selected READ selected_property NOTIFY selectedChanged)

    Q_PROPERTY(bool isTransparent READ isTransparent WRITE setIsTransparent NOTIFY isTransparentChanged)

    Q_PROPERTY(int type READ type_property NOTIFY typeChanged)

    Q_PROPERTY(QList<MenuItem*> menuItems READ menuItems NOTIFY menuItemsChanged)
    Q_PROPERTY(bool isMenuSecondary READ isMenuSecondary NOTIFY isMenuSecondaryChanged)

    ContextInject<muse::rcommand::ICommandDispatcher> commandDispatcher = { this };
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
#endif

public:
    ToolBarItem(QObject* parent = nullptr);
    ToolBarItem(ToolBarItemType::Type type, QObject* parent = nullptr);

    ToolBarItemType::Type type() const;
    bool isValid() const;

    QString id() const;
    QString translatedTitle() const;
    bool showTitle() const;
    void setShowTitle(bool show);
    bool isTitleBold() const;
    void setIsTitleBold(bool newIsTitleBold);

    void setEnabled(bool enabled);
    bool enabled() const;

    bool selected() const;

    const QList<MenuItem*>& menuItems() const;

    bool isMenuSecondary() const;
    void setIsMenuSecondary(bool secondary);

    QString shortcutsTitle() const;

    bool isTransparent() const;
    void setIsTransparent(bool isTransparent);

    Q_INVOKABLE void activate();
    Q_INVOKABLE void handleMenuItem(const QString& menuId);

    // command support
    ToolBarItem(const rcommand::CommandInfo& info, ToolBarItemType::Type type, QObject* parent = nullptr);
    void setCommandInfo(const rcommand::CommandInfo& info);
    rcommand::CommandInfo commandInfo() const;
    void setCommand(const rcommand::Command& command);
    rcommand::Command command() const;
    void setParams(const rcommand::Params& params);
    rcommand::Params params() const;
    void setCommandState(const rcommand::CommandState& state);
    rcommand::CommandState commandState() const;

    // action support
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    ToolBarItem(const ui::UiAction& action, ToolBarItemType::Type type, QObject* parent = nullptr);
    void setAction(const ui::UiAction& action);
    ui::UiAction action() const;
    actions::ActionCode actionCode() const;
    void setState(const ui::UiActionState& state);
    ui::UiActionState state() const;
    void setArgs(const muse::actions::ActionData& args);
    muse::actions::ActionData args() const;
#endif

public slots:
    void setId(const QString& id);
    void setTitle(const TranslatableString& title);
    void setDescription(const TranslatableString& description);
    void setSelected(bool selected);
    void setType(muse::uicomponents::ToolBarItemType::Type type);
    void setMenuItems(const QList<uicomponents::MenuItem*>& menuItems);
    void setShortcuts(const std::vector<std::string>& shortcuts);

signals:
    void idChanged(QString id);
    void titleChanged(QString title);
    void stateChanged();
    void selectedChanged(bool selected);
    void typeChanged(int type);
    void menuItemsChanged(QList<uicomponents::MenuItem*> menuItems, const QString& menuId);
    void isMenuSecondaryChanged(bool secondary);
    void itemChanged();
    void shortcutsChanged();

    void showTitleChanged();
    void isTitleBoldChanged();
    void isTransparentChanged();

private:
    QString code_property() const;
    QString description_property() const;
    int icon_property() const;
    bool checkable_property() const;
    bool checked_property() const;
    bool selected_property() const;
    int type_property() const;

    ToolBarItemType::Type m_type = ToolBarItemType::ACTION;

    QString m_id;
    std::string m_intent;
    MnemonicString m_title;
    TranslatableString m_description;
    bool m_showTitle = false;
    bool m_isTitleBold = false;

    ui::IconCode::Code m_icon = ui::IconCode::Code::NONE;

    bool m_enabled = true;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_selected = false;

    QList<MenuItem*> m_menuItems;
    bool m_isMenuSecondary = false;

    std::vector<std::string> m_shortcuts;
    bool m_isTransparent = true;

    rcommand::Params m_params;
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    muse::actions::ActionData m_args;
#endif
};
using ToolBarItemList = QList<ToolBarItem*>;
}
