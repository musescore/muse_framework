/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>

#include "kddockwidgets/src/Config.h"
#include "kddockwidgets/src/ContextData.h"
#include "kddockwidgets/src/qtquick/Platform.h"
#include "kddockwidgets/src/qtquick/QmlConfig.h"
#include "kddockwidgets/src/qtquick/ViewFactory.h"

#include "dockcentralview.h"
#include "dockpageview.h"
#include "dockpanelview.h"
#include "docktoolbarview.h"
#include "dockwindow.h"
#include "internal/dockcommandsregister.h"
#include "internal/dockcommandsstate.h"
#include "internal/dockwindowprovider.h"

#include "rcommand/internal/commandsregister.h"
#include "rcommand/internal/commandsstate.h"

#include "shortcuts_v2/tests/mocks/commandshortcutsregistermock.h"
#include "ui/tests/mocks/uistatemock.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using namespace muse;
using namespace muse::dock;

static const modularity::ContextPtr iocCtx = std::make_shared<modularity::Context>(1);
static const std::string MODULE_NAME = "utests";

static constexpr int WINDOW_HEIGHT = 400;
static constexpr int NARROW_WINDOW_WIDTH = 300;
static constexpr int WIDE_WINDOW_WIDTH = 600;

//! NOTE: The panel has a fixed width that depends on the window width
static constexpr int PANEL_SWITCH_WINDOW_WIDTH = 450;
static constexpr int NARROW_PANEL_WIDTH = 30;
static constexpr int WIDE_PANEL_WIDTH = 120;

static constexpr int TOOLBAR_WIDTH = 100;
static constexpr int TOOLBAR_HEIGHT = 20;

//! NOTE: See Group.qml of KDDockWidgets
static constexpr int GROUP_CONTENTS_MARGIN = 1;

class Dock_DockWindowTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_uiState = std::make_shared<NiceMock<ui::UiStateMock> >();
        ON_CALL(*m_uiState, pageState(_)).WillByDefault(Return(ValNt<QByteArray>()));

        modularity::ioc(iocCtx)->registerExport<ui::IUiState>(MODULE_NAME, m_uiState);
        modularity::ioc(iocCtx)->registerExport<IDockWindowProvider>(MODULE_NAME, std::make_shared<DockWindowProvider>());

        //! NOTE: The panel context menu needs the dock commands
        auto commandsRegister = std::make_shared<rcommand::CommandsRegister>();
        commandsRegister->reg(std::make_shared<DockCommandsRegister>());
        modularity::globalIoc()->registerExport<rcommand::ICommandsRegister>(MODULE_NAME, commandsRegister);

        auto commandsState = std::make_shared<rcommand::CommandsState>();
        commandsState->reg(std::make_shared<DockCommandsState>(iocCtx));
        modularity::ioc(iocCtx)->registerExport<rcommand::ICommandsState>(MODULE_NAME, commandsState);

        m_shortcutsRegister = std::make_shared<NiceMock<shortcuts::CommandShortcutsRegisterMock> >();
        ON_CALL(*m_shortcutsRegister, shortcut(_)).WillByDefault(ReturnRef(m_shortcut));
        modularity::globalIoc()->registerExport<shortcuts::ICommandShortcutsRegister>(MODULE_NAME, m_shortcutsRegister);

        m_engine = new QQmlEngine();

        auto qmlIoc = new QmlIoCContext(m_engine);
        qmlIoc->ctx = iocCtx;
        m_engine->rootContext()->setContextProperty("ioc_context", QVariant::fromValue(qmlIoc));

        setUpKDDockWidgets();

        m_window = new QQuickWindow();
    }

    void TearDown() override
    {
        m_dockWindow->setParentItem(nullptr);
        delete m_dockWindow;
        delete m_window;

        KDDockWidgets::ContextData::destroyContext(iocCtx->id);

        delete m_engine;

        modularity::removeIoC(iocCtx);
        modularity::globalIoc()->unregister<rcommand::ICommandsRegister>(MODULE_NAME);
        modularity::globalIoc()->unregister<shortcuts::ICommandShortcutsRegister>(MODULE_NAME);
    }

    void setUpKDDockWidgets()
    {
        const int ctx = iocCtx->id;

        KDDockWidgets::ContextData::context(ctx);

        KDDockWidgets::Config& config = KDDockWidgets::Config::self(ctx);
        config.setViewFactory(new KDDockWidgets::QtQuick::ViewFactory(ctx));

        KDDockWidgets::setCtxForEngine(m_engine, ctx);
        KDDockWidgets::QtQuick::Platform::instance()->setQmlEngine(m_engine);

        config.setAbsoluteWidgetMinSize(QSize(10, 10));
        config.setSeparatorThickness(1);
    }

    template<typename T>
    T* beginItem(QObject* owner)
    {
        T* item = new T();
        item->setParent(owner);

        QQmlEngine::setContextForObject(item, m_engine->rootContext());
        static_cast<QQmlParserStatus*>(item)->classBegin();

        return item;
    }

    void completeItem(QQuickItem* item)
    {
        static_cast<QQmlParserStatus*>(item)->componentComplete();
    }

    template<typename T>
    T* createDock(const QString& name, QObject* owner, QQuickItem** content = nullptr)
    {
        T* dock = beginItem<T>(owner);
        dock->setObjectName(name);

        QQuickItem* dockContent = new QQuickItem(dock);
        if (content) {
            *content = dockContent;
        }

        completeItem(dock);

        return dock;
    }

    DockToolBarView* createToolBar(const QString& name, DockToolBarAlignment::Type alignment, QObject* owner,
                                   QQuickItem** content)
    {
        DockToolBarView* toolBar = beginItem<DockToolBarView>(owner);
        toolBar->setObjectName(name);
        toolBar->setAlignment(static_cast<int>(alignment));
        toolBar->setContentWidth(TOOLBAR_WIDTH);
        toolBar->setContentHeight(TOOLBAR_HEIGHT);
        toolBar->setMinimumHeight(TOOLBAR_HEIGHT);
        toolBar->setMaximumHeight(TOOLBAR_HEIGHT);

        *content = new QQuickItem(toolBar);

        completeItem(toolBar);

        return toolBar;
    }

    //! NOTE: Emulates the QML bindings of the panel width to the window width
    void updatePanelWidth()
    {
        const int width = m_dockWindow->width() < PANEL_SWITCH_WINDOW_WIDTH ? NARROW_PANEL_WIDTH : WIDE_PANEL_WIDTH;

        m_panel->setWidth(width);
        m_panel->setMinimumWidth(width);
        m_panel->setMaximumWidth(width);
    }

    void openWindow(int width)
    {
        m_window->resize(width, WINDOW_HEIGHT);

        m_dockWindow = beginItem<DockWindow>(nullptr);
        m_dockWindow->setParentItem(m_window->contentItem());
        m_dockWindow->setSize(QSizeF(width, WINDOW_HEIGHT));

        DockPageView* page = beginItem<DockPageView>(m_dockWindow);
        page->setObjectName("a");
        page->setUri("test://a");
        page->setCentralDock(createDock<DockCentralView>("a_central", page));

        m_panel = createDock<DockPanelView>("a_panel", page, &m_panelContent);
        updatePanelWidth();

        QQmlListProperty<DockPanelView> panels = page->panelsProperty();
        panels.append(&panels, m_panel);

        //! NOTE: The left toolbar absorbs the free space of the toolbars row
        DockToolBarView* leftToolBar = createToolBar("a_left", DockToolBarAlignment::Left, m_dockWindow, &m_leftToolBarContent);
        QQmlListProperty<DockToolBarView> windowToolBars = m_dockWindow->toolBarsProperty();
        windowToolBars.append(&windowToolBars, leftToolBar);

        m_rightToolBar = createToolBar("a_right", DockToolBarAlignment::Right, page, &m_rightToolBarContent);
        QQmlListProperty<DockToolBarView> pageToolBars = page->mainToolBarsProperty();
        pageToolBars.append(&pageToolBars, m_rightToolBar);

        completeItem(page);

        QQmlListProperty<DockPageView> pages = m_dockWindow->pagesProperty();
        pages.append(&pages, page);

        completeItem(m_dockWindow);

        QObject::connect(m_dockWindow, &QQuickItem::widthChanged, m_dockWindow, [this]() {
            updatePanelWidth();
        });

        m_dockWindow->loadPage("test://a", {});

        processLayout();
    }

    //! NOTE: Resizes the window in a single step, like maximizing does.
    //! The panel constraints may change after the window relayout has already been done,
    //! so the layout must follow them without waiting for the next relayout
    void resizeWindow(int width)
    {
        m_window->resize(width, WINDOW_HEIGHT);
        m_dockWindow->setWidth(width);

        QCoreApplication::processEvents();
    }

    void processLayout()
    {
        QCoreApplication::processEvents();
        m_dockWindow->ensurePolished();
        QCoreApplication::processEvents();
    }

    static int widthInLayout(const QQuickItem* content)
    {
        //! NOTE: The content fills the dock widget inside the group
        return content->width() + 2 * GROUP_CONTENTS_MARGIN;
    }

    int panelWidthInLayout() const
    {
        return widthInLayout(m_panelContent);
    }

    std::shared_ptr<NiceMock<ui::UiStateMock> > m_uiState;
    std::shared_ptr<NiceMock<shortcuts::CommandShortcutsRegisterMock> > m_shortcutsRegister;
    shortcuts::Shortcut m_shortcut;
    QQmlEngine* m_engine = nullptr;
    QQuickWindow* m_window = nullptr;
    DockWindow* m_dockWindow = nullptr;
    DockPanelView* m_panel = nullptr;
    QQuickItem* m_panelContent = nullptr;
    DockToolBarView* m_rightToolBar = nullptr;
    QQuickItem* m_leftToolBarContent = nullptr;
    QQuickItem* m_rightToolBarContent = nullptr;
};

TEST_F(Dock_DockWindowTests, PanelGrowsWhenWindowBecomesWide)
{
    //! [GIVEN] A narrow window with the narrow panel
    openWindow(NARROW_WINDOW_WIDTH);
    ASSERT_EQ(panelWidthInLayout(), NARROW_PANEL_WIDTH);

    //! [WHEN] The window becomes wide in a single step
    resizeWindow(WIDE_WINDOW_WIDTH);

    //! [THEN] The panel takes the wide width in the layout
    EXPECT_EQ(panelWidthInLayout(), WIDE_PANEL_WIDTH);
}

TEST_F(Dock_DockWindowTests, PanelShrinksWhenWindowBecomesNarrow)
{
    //! [GIVEN] A wide window with the wide panel
    openWindow(WIDE_WINDOW_WIDTH);
    ASSERT_EQ(panelWidthInLayout(), WIDE_PANEL_WIDTH);

    //! [WHEN] The window becomes narrow in a single step
    resizeWindow(NARROW_WINDOW_WIDTH);

    //! [THEN] The panel takes the narrow width in the layout
    EXPECT_EQ(panelWidthInLayout(), NARROW_PANEL_WIDTH);
}

TEST_F(Dock_DockWindowTests, TopToolBarShrinksWhenItsMaximumWidthDecreases)
{
    //! [GIVEN] A window with the left and right toolbars on top
    openWindow(WIDE_WINDOW_WIDTH);
    ASSERT_EQ(widthInLayout(m_rightToolBarContent), TOOLBAR_WIDTH);

    //! [WHEN] The maximum width of the right toolbar decreases
    const int newWidth = TOOLBAR_WIDTH / 2;
    const int leftToolBarWidth = widthInLayout(m_leftToolBarContent);

    m_rightToolBar->setMinimumWidth(newWidth);
    m_rightToolBar->setMaximumWidth(newWidth);

    QCoreApplication::processEvents();

    //! [THEN] The right toolbar takes the new width, and the left one takes the free space
    EXPECT_EQ(widthInLayout(m_rightToolBarContent), newWidth);
    EXPECT_EQ(widthInLayout(m_leftToolBarContent), leftToolBarWidth + TOOLBAR_WIDTH - newWidth);
}
