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

#include "dockpanelview.h"

#include "kddockwidgets/src/core/DockWidget.h"
#include "kddockwidgets/src/core/Group.h"
#include "kddockwidgets/src/qtquick/views/DockWidget.h"
#include "kddockwidgets/src/qtquick/views/View.h"

#include "rcommand/commandtypes.h"
#include "types/translatablestring.h"

#include "uicomponents/qml/Muse/UiComponents/abstractmenumodel.h"

#include "dockcommands.h"

#include "log.h"

using namespace muse;
using namespace muse::dock;
using namespace muse::ui;
using namespace muse::uicomponents;
using namespace muse::actions;

class DockPanelView::DockPanelMenuModel : public muse::uicomponents::AbstractMenuModel
{
public:
    DockPanelMenuModel(DockPanelView* panel)
        : AbstractMenuModel(panel), m_panel(panel)
    {
        listenFloatingChanged();
    }

    void load() override
    {
        TRACEFUNC;

        MenuItemList items;

        if (m_customMenuModel && m_customMenuModel->rowCount() > 0) {
            items << m_customMenuModel->items();
            items << makeSeparator();
        }

        MenuItem* closeDockItem = makeMenuItem(DOCK_SET_OPEN_COMMAND, TranslatableString("appshell/dock", "Close"));
        closeDockItem->setParams({ { "dock_name", Val(m_panel->objectName().toStdString()) }, { "open", Val(false) } });
        items << closeDockItem;

        MenuItem* toggleFloatingItem = makeMenuItem(DOCK_TOGGLE_FLOATING_COMMAND, toggleFloatingActionTitle());
        toggleFloatingItem->setParams({ { "dock_name", Val(m_panel->objectName().toStdString()) } });
        items << toggleFloatingItem;

        setItems(items);
    }

    void handleMenuItem(const QString& itemId) override
    {
        // my items
        rcommand::Command cmd(itemId.toStdString());
        if (cmd == DOCK_SET_OPEN_COMMAND || cmd == DOCK_TOGGLE_FLOATING_COMMAND) {
            AbstractMenuModel::handleMenuItem(itemId);
            return;
        }

        // forward to custom model
        if (m_customMenuModel) {
            m_customMenuModel->handleMenuItem(itemId);
        }
    }

    AbstractMenuModel* customMenuModel() const
    {
        return m_customMenuModel;
    }

    void setCustomMenuModel(AbstractMenuModel* model)
    {
        m_customMenuModel = model;

        if (!model) {
            return;
        }

        connect(model, &AbstractMenuModel::itemsChanged, this, [this]() {
            load();
        });

        connect(model, &AbstractMenuModel::itemChanged, this, [this](MenuItem* item) {
            updateItem(item);
        });

        connect(model, &AbstractMenuModel::destroyed, this, [this]() {
            m_customMenuModel = nullptr;
        });
    }

private:

    TranslatableString toggleFloatingActionTitle() const
    {
        return m_panel->floating() ? TranslatableString("appshell/dock", "Dock") : TranslatableString("appshell/dock", "Undock");
    }

    void listenFloatingChanged()
    {
        connect(m_panel, &DockPanelView::floatingChanged, this, [this]() {
            MenuItem& item = findItem(DOCK_TOGGLE_FLOATING_COMMAND);
            item.setTitle(toggleFloatingActionTitle());
        });
    }

    void updateItem(MenuItem* newItem)
    {
        int index = itemIndex(newItem->id());

        if (index == INVALID_ITEM_INDEX) {
            return;
        }

        setItem(index, newItem);
    }

    AbstractMenuModel* m_customMenuModel = nullptr;
    DockPanelView* m_panel = nullptr;
};

DockPanelView::DockPanelView(QQuickItem* parent)
    : DockBase(DockType::Panel, parent), m_menuModel(new DockPanelMenuModel(this))
{
    setLocation(Location::Left);
}

DockPanelView::~DockPanelView()
{
    auto* dockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
        KDDockWidgets::QtQuick::asQQuickItem(dockWidget()));
    if (!dockWidgetView) {
        return;
    }

    dockWidgetView->setProperty(DOCK_PANEL_PROPERTY, QVariant::fromValue(nullptr));
    dockWidgetView->setProperty(CONTEXT_MENU_MODEL_PROPERTY, QVariant::fromValue(nullptr));
    dockWidgetView->setProperty(TITLEBAR_PROPERTY, QVariant::fromValue(nullptr));
    dockWidgetView->setProperty(TOOLBAR_COMPONENT_PROPERTY, QVariant::fromValue(nullptr));
}

QString DockPanelView::groupName() const
{
    return m_groupName;
}

void DockPanelView::setGroupName(const QString& name)
{
    if (m_groupName == name) {
        return;
    }

    m_groupName = name;
    emit groupNameChanged();
}

void DockPanelView::componentComplete()
{
    DockBase::componentComplete();

    auto* dockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
        KDDockWidgets::QtQuick::asQQuickItem(dockWidget()));
    IF_ASSERT_FAILED(dockWidgetView) {
        return;
    }

    m_menuModel->load();

    dockWidgetView->setProperty(DOCK_PANEL_PROPERTY, QVariant::fromValue(this));
    dockWidgetView->setProperty(CONTEXT_MENU_MODEL_PROPERTY, QVariant::fromValue(m_menuModel));
    dockWidgetView->setProperty(TITLEBAR_PROPERTY, QVariant::fromValue(m_titleBar));
    dockWidgetView->setProperty(TOOLBAR_COMPONENT_PROPERTY, QVariant::fromValue(m_toolbarComponent));

    connect(m_menuModel, &AbstractMenuModel::itemsChanged, [dockWidgetView, this]() {
        if (dockWidgetView) {
            dockWidgetView->setProperty(CONTEXT_MENU_MODEL_PROPERTY, QVariant::fromValue(m_menuModel));
        }
    });

    connect(this, &DockPanelView::toolbarComponentChanged, this, [this, dockWidgetView]() {
        if (dockWidgetView) {
            dockWidgetView->setProperty(TOOLBAR_COMPONENT_PROPERTY, QVariant::fromValue(m_toolbarComponent));
        }
    });

    connect(this, &DockBase::frameCurrentWidgetChanged, this, [this]() {
        auto* ctrl = dockWidget();
        if (!ctrl) {
            return;
        }
        auto* dockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
            KDDockWidgets::QtQuick::asQQuickItem(ctrl));
        auto* group = dockWidgetView ? dockWidgetView->group() : nullptr;
        if (group && group->currentDockWidget() == ctrl) {
            emit panelShown();
        }
    });
}

AbstractMenuModel* DockPanelView::contextMenuModel() const
{
    return m_menuModel->customMenuModel();
}

QQmlComponent* DockPanelView::titleBar() const
{
    return m_titleBar;
}

QQmlComponent* DockPanelView::toolbarComponent() const
{
    return m_toolbarComponent;
}

void DockPanelView::setContextMenuModel(AbstractMenuModel* model)
{
    if (m_menuModel->customMenuModel() == model) {
        return;
    }

    m_menuModel->setCustomMenuModel(model);
    m_menuModel->load();
    emit contextMenuModelChanged();
}

void DockPanelView::setTitleBar(QQmlComponent* titleBar)
{
    if (m_titleBar == titleBar) {
        return;
    }

    m_titleBar = titleBar;
    emit titleBarChanged();
}

void DockPanelView::setToolbarComponent(QQmlComponent* component)
{
    if (m_toolbarComponent == component) {
        return;
    }

    m_toolbarComponent = component;
    emit toolbarComponentChanged();
}

bool DockPanelView::isTabAllowed(const DockPanelView* tab) const
{
    IF_ASSERT_FAILED(tab) {
        return false;
    }

    if (tab == this) {
        return false;
    }

    if (!isOpen()) {
        return false;
    }

    if (floating()) {
        return false;
    }

    if (m_groupName.isEmpty() || tab->m_groupName.isEmpty()) {
        return false;
    }

    return m_groupName == tab->m_groupName;
}

void DockPanelView::addPanelAsTab(DockPanelView* tab)
{
    IF_ASSERT_FAILED(tab && dockWidget()) {
        return;
    }

    if (!isTabAllowed(tab)) {
        return;
    }

    dockWidget()->addDockWidgetAsTab(tab->dockWidget());
    tab->setVisible(true);
}

void DockPanelView::setCurrentTabIndex(int index)
{
    IF_ASSERT_FAILED(dockWidget()) {
        return;
    }

    auto* dockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
        KDDockWidgets::QtQuick::asQQuickItem(dockWidget()));
    auto* group = dockWidgetView ? dockWidgetView->group() : nullptr;
    if (group) {
        group->setCurrentTabIndex(index);
    }
}
