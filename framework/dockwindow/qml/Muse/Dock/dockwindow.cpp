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

#include "dockwindow.h"

#include "kddockwidgets/src/LayoutSaver.h"
#include "kddockwidgets/src/core/DockRegistry.h"
#include "kddockwidgets/src/core/Layout.h"
#include "kddockwidgets/src/core/MainWindow.h"
#include "kddockwidgets/src/qtquick/views/MainWindow.h"
#include "kddockwidgets/src/qtquick/views/DockWidget.h"
#include "kddockwidgets/src/qtquick/views/View.h"
#include "kddockwidgets/src/core/Group.h"
#include "kddockwidgets/src/core/Window_p.h"

#include "global/async/async.h"

#include "dockcentralview.h"
#include "dockpageview.h"
#include "dockpanelview.h"
#include "dockstatusbar.h"
#include "docktoolbarview.h"
#include "dockingholderview.h"
#include "dockwindow.h"
#include "topleveltoolbarslayout.h"

#include "muse_framework_config.h"

#include "log.h"

using namespace muse::dock;
using namespace muse::async;

namespace muse::dock {
static const QList<Location> POSSIBLE_LOCATIONS {
    Location::Left,
    Location::Right,
    Location::Top,
    Location::Bottom
};

static KDDockWidgets::Location locationToKLocation(Location location)
{
    switch (location) {
    case Location::Left: return KDDockWidgets::Location_OnLeft;
    case Location::Right: return KDDockWidgets::Location_OnRight;
    case Location::Top: return KDDockWidgets::Location_OnTop;
    case Location::Bottom: return KDDockWidgets::Location_OnBottom;
    case Location::Center: break;
    case Location::Undefined: break;
    }

    return KDDockWidgets::Location_None;
}

static void clearRegistry(int ctx)
{
    TRACEFUNC;

    auto* registry = KDDockWidgets::DockRegistry::self(ctx);

    for (KDDockWidgets::Core::MainWindow* mw : registry->mainwindows()) {
        mw->layout()->clearLayout();
    }

    registry->clear();

    for (KDDockWidgets::Core::DockWidget* dock : registry->dockwidgets()) {
        registry->unregisterDockWidget(dock);
    }

    for (KDDockWidgets::Core::Group* group : registry->groups()) {
        const auto dockWidgets = group->dockWidgets();
        for (KDDockWidgets::Core::DockWidget* dock : dockWidgets) {
            group->removeWidget(dock);
        }
        registry->unregisterGroup(group);
    }
}
}

DockWindow::DockWindow(QQuickItem* parent)
    : QQuickItem(parent), muse::Contextable(muse::iocCtxForQmlObject(this)),
    m_toolBars(this),
    m_pages(this)
{
}

DockWindow::~DockWindow()
{
    dockWindowProvider()->deinit();
}

void DockWindow::componentComplete()
{
    TRACEFUNC;

    QQuickItem::componentComplete();

    static const QString name = "mainWindow";

    m_mainWindow = new KDDockWidgets::QtQuick::MainWindow(iocContext()->id, name,
                                                          KDDockWidgets::MainWindowOption_None,
                                                          this);

    m_topLevelToolBarsLayout = new TopLevelToolBarsLayout(m_mainWindow, this);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &DockWindow::onQuit);
    connect(this, &QQuickItem::windowChanged, this, &DockWindow::windowPropertyChanged);
}

void DockWindow::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (m_currentPage && !qFuzzyCompare(newGeometry.width(), oldGeometry.width())) {
        polish();
    }
}

void DockWindow::updatePolish()
{
    QQuickItem::updatePolish();

    if (m_topLevelToolBarsLayout) {
        m_topLevelToolBarsLayout->relayout();
    }
}

void DockWindow::onQuit()
{
    TRACEFUNC;

    IF_ASSERT_FAILED(m_currentPage) {
        return;
    }

    savePageState(m_currentPage->objectName());
    m_reloadCurrentPageAllowed = false;

    clearRegistry(iocContext()->id);

    saveWindowGeometry();
}

QString DockWindow::currentPageUri() const
{
    return m_currentPage ? m_currentPage->uri() : QString();
}

QQmlListProperty<muse::dock::DockToolBarView> DockWindow::toolBarsProperty()
{
    return m_toolBars.property();
}

QQmlListProperty<muse::dock::DockPageView> DockWindow::pagesProperty()
{
    return m_pages.property();
}

QQuickWindow* DockWindow::windowProperty() const
{
    return window();
}

void DockWindow::init()
{
    clearRegistry(iocContext()->id);
    restoreGeometry();

    dockWindowProvider()->init(this);

    uiState()->windowGeometryChanged().onNotify(this, [this]() {
        reloadCurrentPage();
    });

    workspaceManager()->currentWorkspaceAboutToBeChanged().onNotify(this, [this]() {
        if (const DockPageView* page = currentPage()) {
            savePageState(page->objectName());
        }
    });
}

void DockWindow::loadPage(const QString& uri, const QVariantMap& params)
{
    TRACEFUNC;

    if (currentPageUri() == uri) {
        if (m_currentPage) {
            m_currentPage->setParams(params);
        }
        return;
    }

    const bool isFirstOpening = (m_currentPage == nullptr);

    if (!isFirstOpening) {
        const QString pageName = m_currentPage->objectName();
        uiState()->pageState(pageName).notification.disconnect(this);
        savePageState(pageName);
        clearRegistry(iocContext()->id);
        m_currentPage->setVisible(false);
        m_currentPage->deinit();
    }

    bool ok = doLoadPage(uri, params);
    if (!ok) {
        return;
    }

    if (checkLayoutIsCorrupted()) {
        LOGE() << "Layout is corrupted, restoring default";
        restoreDefaultLayout();
    }

    auto notifyAboutPageLoaded = [this, &uri]() {
        emit currentPageUriChanged(uri);
        emit pageLoaded();
        notifyAboutDocksOpenStatus();
    };

    if (isFirstOpening) {
        async::Async::call(this, [this, notifyAboutPageLoaded]() {
            if (!m_hasGeometryBeenRestored
                || (m_mainWindow->window()->isFullScreen())) {
                //! NOTE: show window as maximized if no geometry has been restored
                //! or if the user had closed app in FullScreen mode
                // m_mainWindow->window()->->showMaximized(); // todo kddock
            }

            notifyAboutPageLoaded();
        });
    } else {
        notifyAboutPageLoaded();
    }
}

void DockWindow::openPage(const QString& uri)
{
    interactive()->open(uri.toStdString());
}

bool DockWindow::isDockOpen(const QString& dockName) const
{
    return m_currentPage && m_currentPage->isDockOpen(dockName);
}

void DockWindow::toggleDock(const QString& dockName)
{
    if (m_currentPage) {
        m_currentPage->toggleDock(dockName);
        m_docksOpenStatusChanged.send({ dockName });
    }
}

void DockWindow::setDockOpen(const QString& dockName, bool open)
{
    if (m_currentPage) {
        m_currentPage->setDockOpen(dockName, open);
        m_docksOpenStatusChanged.send({ dockName });
    }
}

Channel<QStringList> DockWindow::docksOpenStatusChanged() const
{
    return m_docksOpenStatusChanged;
}

bool DockWindow::isDockFloating(const QString& dockName) const
{
    return m_currentPage && m_currentPage->isDockFloating(dockName);
}

void DockWindow::toggleDockFloating(const QString& dockName)
{
    if (m_currentPage) {
        m_currentPage->toggleDockFloating(dockName);
    }
}

DockPageView* DockWindow::currentPage() const
{
    return m_currentPage;
}

QQuickItem& DockWindow::asItem() const
{
    return *m_mainWindow;
}

void DockWindow::restoreDefaultLayout()
{
    TRACEFUNC;

    //! HACK: notify about upcoming change of current URI
    //! so that all subscribers of this channel finish their work.
    //! For example, our popups and tooltips will close.
    interactive()->currentUriAboutToBeChanged().notify();

    if (m_currentPage) {
        for (DockBase* dock : m_currentPage->allDocks()) {
            dock->resetToDefault();
        }
    }

    m_reloadCurrentPageAllowed = false;
    for (const DockPageView* page : m_pages.list()) {
        uiState()->setPageState(page->objectName(), QByteArray());
    }

    uiState()->setWindowGeometry(QByteArray());
    m_reloadCurrentPageAllowed = true;

    reloadCurrentPage();
}

void DockWindow::loadPageContent(const DockPageView* page)
{
    TRACEFUNC;

    addDock(page->centralDock());

    loadPanels(page);
    loadToolBars(page);

    if (page->statusBar()) {
        addDock(page->statusBar(), Location::Bottom);
    }

    loadTopLevelToolBars(page);
}

void DockWindow::loadTopLevelToolBars(const DockPageView* page)
{
    TRACEFUNC;

    QList<DockToolBarView*> allToolBars = m_toolBars.list();
    allToolBars << page->mainToolBars();

    DockToolBarView* prevToolBar = nullptr;

    for (DockToolBarView* toolBar : allToolBars) {
        auto location = prevToolBar ? Location::Right : Location::Top;
        addDock(toolBar, location, prevToolBar);
        prevToolBar = toolBar;
    }
}

void DockWindow::loadToolBars(const DockPageView* page)
{
    TRACEFUNC;

    for (DockToolBarView* toolBar : page->toolBars()) {
        addDock(toolBar, toolBar->location());
    }

    for (Location location : POSSIBLE_LOCATIONS) {
        if (auto holder = page->holder(DockType::ToolBar, location)) {
            addDock(holder, location);
        }
    }
}

void DockWindow::loadPanels(const DockPageView* page)
{
    TRACEFUNC;

    for (DockPanelView* panel : page->panels()) {
        if (DockPanelView* destinationPanel = page->findPanelForTab(panel)) {
            addPanelAsTab(panel, destinationPanel);
            continue;
        }

        const Location location = panel->location();
        const bool isSideLocation = location == Location::Left || location == Location::Right;
        addDock(panel, location, isSideLocation ? page->centralDock() : nullptr);
    }

    for (Location location : POSSIBLE_LOCATIONS) {
        if (auto holder = page->holder(DockType::Panel, location)) {
            addDock(holder, location);
        }
    }
}

void DockWindow::addDock(DockBase* dock, Location location, const DockBase* relativeTo)
{
    TRACEFUNC;

    registerDock(dock);

    auto* dockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
        KDDockWidgets::QtQuick::asQQuickItem(dock->dockWidget()));

    KDDockWidgets::QtQuick::DockWidget* relativeDockWidgetView = nullptr;
    if (relativeTo) {
        relativeDockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
            KDDockWidgets::QtQuick::asQQuickItem(relativeTo->dockWidget()));
    }

    auto visibilityOption = dock->defaultVisibility() ? KDDockWidgets::InitialVisibilityOption::StartVisible
                            : KDDockWidgets::InitialVisibilityOption::StartHidden;

    KDDockWidgets::InitialOption options(visibilityOption, dock->preferredSize());

    m_mainWindow->addDockWidget(dockWidgetView, locationToKLocation(location), relativeDockWidgetView, options);
}

void DockWindow::addPanelAsTab(DockPanelView* panel, DockPanelView* destinationPanel)
{
    registerDock(panel);

    if (panel->defaultVisibility()) {
        destinationPanel->addPanelAsTab(panel);
        destinationPanel->setCurrentTabIndex(0);
    }
}

void DockWindow::registerDock(DockBase* dock)
{
    TRACEFUNC;

    IF_ASSERT_FAILED(dock) {
        return;
    }

    auto* registry = KDDockWidgets::DockRegistry::self(iocContext()->id);
    auto* dockWidget = dock->dockWidget();

    if (!registry->containsDockWidget(dockWidget->uniqueName())) {
        registry->registerDockWidget(dockWidget);
    }
}

void DockWindow::handleUnknownDock(const DockPageView* page, DockBase* unknownDock)
{
    DockPanelView* unknownPanel = dynamic_cast<DockPanelView*>(unknownDock);
    if (!unknownPanel) {
        addDock(unknownDock, unknownDock->location(), page->centralDock());
        return;
    }

    if (DockPanelView* destinationPanel = page->findPanelForTab(unknownPanel)) {
        addPanelAsTab(unknownPanel, destinationPanel);
        return;
    }

    DockingHolderView* holder = page->holder(DockType::Panel, unknownPanel->location());
    IF_ASSERT_FAILED(holder) {
        addDock(unknownDock, unknownDock->location(), page->centralDock());
        return;
    }

    registerDock(unknownPanel);

    holder->open(); // init the group...

    auto* holderDockWidgetView = qobject_cast<KDDockWidgets::QtQuick::DockWidget*>(
        KDDockWidgets::QtQuick::asQQuickItem(holder->dockWidget()));
    if (holderDockWidgetView) {
        KDDockWidgets::Core::Group* group = holderDockWidgetView->group();
        if (group) {
            group->addTab(unknownPanel->dockWidget());
        }
    }

    holder->close();

    if (!unknownPanel->isVisible()) {
        unknownPanel->close();
    }
}

DockPageView* DockWindow::pageByUri(const QString& uri) const
{
    for (DockPageView* page : m_pages.list()) {
        if (page->uri() == uri) {
            return page;
        }
    }

    return nullptr;
}

bool DockWindow::doLoadPage(const QString& uri, const QVariantMap& params)
{
    DockPageView* newPage = pageByUri(uri);
    IF_ASSERT_FAILED(newPage) {
        return false;
    }

    newPage->setVisible(true);

    loadPageContent(newPage);
    restorePageState(newPage);
    initDocks(newPage);

    newPage->setParams(params);

    m_currentPage = newPage;

    connect(m_currentPage, &DockPageView::layoutRequested,
            this, &DockWindow::forceLayout, Qt::UniqueConnection);

    return true;
}

void DockWindow::saveWindowGeometry()
{
    /// NOTE: The state of all dock widgets is also saved here,
    /// since the library does not provide the ability to save
    /// and restore only the application geometry.
    uiState()->setWindowGeometry(windowState());
}

void DockWindow::restoreGeometry()
{
    TRACEFUNC;

    if (uiState()->windowGeometry().isEmpty()) {
        return;
    }

    if (restoreLayout(uiState()->windowGeometry())) {
        m_hasGeometryBeenRestored = true;
    } else {
        LOGE() << "Could not restore the window geometry!";
    }
}

void DockWindow::savePageState(const QString& pageName)
{
    TRACEFUNC;

    m_reloadCurrentPageAllowed = false;
    uiState()->setPageState(pageName, windowState());
    m_reloadCurrentPageAllowed = true;
}

void DockWindow::restorePageState(const DockPageView* page)
{
    TRACEFUNC;

    const QString& pageName = page->objectName();

    ValNt<QByteArray> pageStateValNt = uiState()->pageState(pageName);
    const bool layoutIsEmpty = pageStateValNt.val.isEmpty();

    QSet<DockBase*> unknownDocks;
    if (!layoutIsEmpty) {
        for (DockBase* dock : page->allDocks()) {
            const KDDockWidgets::Core::DockWidget* dockWidget = dock->dockWidget();
            if (!pageStateValNt.val.contains(dockWidget->uniqueName().toLocal8Bit())) {
                unknownDocks.insert(dock);
            }
        }
    }

    /// NOTE: Do not restore geometry
    bool ok = restoreLayout(pageStateValNt.val, true /*restoreRelativeToMainWindow*/);
    if (!ok) {
        LOGE() << "Could not restore the state of " << pageName << "!";
    }

    if (!layoutIsEmpty) {
        for (DockBase* dock : unknownDocks) {
            handleUnknownDock(page, dock);
        }
    }

    if (!pageStateValNt.notification.isConnected()) {
        pageStateValNt.notification.onNotify(this, [this, pageName]() {
            bool isCurrentPage = m_currentPage && (m_currentPage->objectName() == pageName);
            if (isCurrentPage) {
                reloadCurrentPage();
            }
        });
    }
}

bool DockWindow::restoreLayout(const QByteArray& layout, bool restoreRelativeToMainWindow)
{
    if (layout.isEmpty()) {
        return true;
    }

    TRACEFUNC;

    auto option = restoreRelativeToMainWindow ? KDDockWidgets::RestoreOption_RelativeToMainWindow
                  : KDDockWidgets::RestoreOption_None;

    KDDockWidgets::LayoutSaver layoutSaver(iocContext()->id, option);
    return layoutSaver.restoreLayout(layout);
}

bool DockWindow::checkLayoutIsCorrupted() const
{
    TRACEFUNC;

    for (const DockBase* dock : m_currentPage->allDocks()) {
        if (!dock) {
            continue;
        }

        if (!dock->floatable() && dock->floating()) {
            return true;
        }
    }

    return false;
}

void DockWindow::forceLayout()
{
    m_mainWindow->layoutEqually();
}

QByteArray DockWindow::windowState() const
{
    TRACEFUNC;

    KDDockWidgets::LayoutSaver layoutSaver(iocContext()->id, KDDockWidgets::RestoreOption_None);
    return layoutSaver.serializeLayout();
}

void DockWindow::reloadCurrentPage()
{
    if (!m_reloadCurrentPageAllowed) {
        return;
    }

    TRACEFUNC;

    clearRegistry(iocContext()->id);

    for (DockBase* dock : m_currentPage->allDocks()) {
        dock->deinit();
    }

    QString currentPageUriBackup = currentPageUri();

    /// NOTE: for reset geometry
    m_currentPage = nullptr;

    if (doLoadPage(currentPageUriBackup)) {
        notifyAboutDocksOpenStatus();
    }
}

void DockWindow::initDocks(DockPageView* page)
{
    TRACEFUNC;

    //! before init we should correct toolbars sizes
    m_topLevelToolBarsLayout->adjustContentForAvailableSpace(page);

    for (DockToolBarView* toolbar : m_toolBars.list()) {
        toolbar->setParentItem(this);
        toolbar->init();
    }

    if (page) {
        page->setParentItem(this);
        page->init();
    }

    m_topLevelToolBarsLayout->setUpPageConnections(page);
    m_topLevelToolBarsLayout->scheduleRelayout();
}

void DockWindow::notifyAboutDocksOpenStatus()
{
    const DockPageView* page = currentPage();

    IF_ASSERT_FAILED(page) {
        return;
    }

    QStringList dockNames;

    for (DockToolBarView* toolBar : page->mainToolBars()) {
        dockNames << toolBar->objectName();
    }

    for (DockToolBarView* toolBar : page->toolBars()) {
        dockNames << toolBar->objectName();
    }

    for (DockPanelView* panel : page->panels()) {
        dockNames << panel->objectName();
    }

    if (page->statusBar()) {
        dockNames << page->statusBar()->objectName();
    }

    m_docksOpenStatusChanged.send(dockNames);
}

QList<DockToolBarView*> DockWindow::topLevelToolBars(const DockPageView* page) const
{
    QList<DockToolBarView*> toolBars = m_toolBars.list();

    if (page) {
        toolBars << page->mainToolBars();
    }

    return toolBars;
}
