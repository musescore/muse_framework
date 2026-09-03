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
#include <gtest/gtest.h>

#include <vector>

#include <QWindow>
#include <QQuickWindow>

#include "ui/internal/navigationcontroller.h"
#include "rcommand/internal/commanddispatcher.h"
#include "actions/internal/actionsdispatcher.h"
#include "ui/navigationcommands.h"

#include "global/tests/mocks/applicationmock.h"
#include "ui/tests/mocks/mainwindowmock.h"

#include "ui/qml/Muse/Ui/navigationcontrol.h"
#include "ui/qml/Muse/Ui/navigationpanel.h"
#include "ui/qml/Muse/Ui/navigationsection.h"

#include "log.h"

using ::testing::Return;

using namespace muse;
using namespace muse::ui;

static const modularity::ContextPtr iocCtx = std::make_shared<modularity::Context>(1);

class Ui_NavigationControllerTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_controller = std::make_shared<NavigationController>(nullptr);

        m_dispatcher = std::make_shared<rcommand::CommandDispatcher>();
        m_actionsDispatcher = std::make_shared<actions::ActionsDispatcher>(iocCtx);
        m_controller->dispatcher.set(m_dispatcher);
        m_controller->actionsDispatcher.set(m_actionsDispatcher);

        m_mainWindow = std::make_shared<ui::MainWindowMock>();
        ON_CALL(*m_mainWindow, qWindow()).WillByDefault(Return(&m_window));
        m_controller->mainWindow.set(m_mainWindow);

        m_applicationMock = std::make_shared<muse::ApplicationMock>();
        ON_CALL(*m_applicationMock, focusWindow()).WillByDefault(Return(&m_window));

        m_controller->init();

        m_idxsRefs.reserve(10000);
    }

    INavigation::Index& make_idx()
    {
        m_idxsRefs.push_back(INavigation::Index());
        return m_idxsRefs.back();
    }

    struct Control {
        NavigationControl* control = nullptr;

        ~Control()
        {
            delete control;
        }
    };

    struct Panel {
        NavigationPanel* panel = nullptr;
        std::vector<Control*> controls;
        std::set<INavigationControl*> icontrols;

        ~Panel()
        {
            delete panel;
            for (Control* c : controls) {
                delete c;
            }
        }
    };

    struct Section {
        NavigationSection* section = nullptr;
        std::vector<Panel*> panels;
        std::set<INavigationPanel*> ipanels;
        OnActiveRequested activeCallback;

        ~Section()
        {
            delete section;
            for (Panel* p : panels) {
                delete p;
            }
        }
    };

    Control* make_control(INavigation::Index& idx)
    {
        Control* c = new Control();
        NavigationControl* navCtrl = new NavigationControl(iocCtx);
        navCtrl->setRow(idx.row);
        navCtrl->setColumn(idx.column);
        navCtrl->setOrder(idx.order());
        navCtrl->setEnabled(true);
        navCtrl->setActive(false);

        setComponentWindow(navCtrl, &m_window);

        navCtrl->navigationController.set(m_controller);

        c->control = navCtrl;

        return c;
    }

    Panel* make_panel(int panelOrder, size_t controlsCount)
    {
        Panel* p = new Panel();

        NavigationPanel* navPanel = new NavigationPanel(iocCtx);

        for (size_t ci = 0; ci < controlsCount; ++ci) {
            INavigation::Index& idx = make_idx();
            idx.column = static_cast<int>(ci);

            Control* c = make_control(idx);
            c->control->setPanel(navPanel);

            p->controls.push_back(c);
            p->icontrols.insert(c->control);
        }

        navPanel->setEnabled(true);
        navPanel->setActive(false);

        INavigation::Index& idx = make_idx();
        idx.setOrder(panelOrder);

        navPanel->setRow(idx.row);
        navPanel->setColumn(idx.column);
        navPanel->setOrder(idx.order());

        setComponentWindow(navPanel, &m_window);

        navPanel->navigationController.set(m_controller);

        p->panel = navPanel;

        return p;
    }

    Section* make_directed_section(int sectOrder, size_t controlsCount, NavigationPanel::QmlDirection dir)
    {
        Section* s = new Section();

        NavigationSection* navSection = new NavigationSection(iocCtx);

        Panel* p = new Panel();
        NavigationPanel* navPanel = new NavigationPanel(iocCtx);

        for (size_t ci = 0; ci < controlsCount; ++ci) {
            INavigation::Index& idx = make_idx();
            bool isLast = (ci == controlsCount - 1);
            if (dir == NavigationPanel::Vertical) {
                idx.row = static_cast<int>(ci);
                idx.column = isLast ? 1 : 0;
            } else {
                idx.row = isLast ? 1 : 0;
                idx.column = static_cast<int>(ci);
            }

            Control* c = make_control(idx);
            c->control->setPanel(navPanel);

            p->controls.push_back(c);
            p->icontrols.insert(c->control);
        }

        navPanel->setDirection(dir);
        navPanel->setEnabled(true);
        navPanel->setActive(false);

        INavigation::Index& panelIdx = make_idx();
        panelIdx.setOrder(0);

        navPanel->setRow(panelIdx.row);
        navPanel->setColumn(panelIdx.column);
        navPanel->setOrder(panelIdx.order());

        setComponentWindow(navPanel, &m_window);

        navPanel->navigationController.set(m_controller);

        navPanel->setSection(navSection);
        p->panel = navPanel;
        s->panels.push_back(p);
        s->ipanels.insert(p->panel);

        navSection->setType(NavigationSection::QmlType::Regular);
        navSection->setEnabled(true);
        navSection->setActive(false);

        INavigation::Index& sectIdx = make_idx();
        sectIdx.setOrder(sectOrder);

        navSection->setRow(sectIdx.row);
        navSection->setColumn(sectIdx.column);
        navSection->setOrder(sectIdx.order());

        setComponentWindow(navSection, &m_window);
        navSection->application.set(m_applicationMock);

        navSection->navigationController.set(m_controller);

        s->section = navSection;

        return s;
    }

    Section* make_section(int sectOrder, size_t panelsCount, size_t controlsCount)
    {
        Section* s = new Section();

        NavigationSection* navSection = new NavigationSection(iocCtx);

        for (size_t pi = 0; pi < panelsCount; ++pi) {
            Panel* p = make_panel(static_cast<int>(pi), controlsCount);
            p->panel->setSection(navSection);

            s->panels.push_back(p);
            s->ipanels.insert(p->panel);
        }

        navSection->setType(NavigationSection::QmlType::Regular);
        navSection->setEnabled(true);
        navSection->setActive(false);

        INavigation::Index& idx = make_idx();
        idx.setOrder(sectOrder);

        navSection->setRow(idx.row);
        navSection->setColumn(idx.column);
        navSection->setOrder(idx.order());

        setComponentWindow(navSection, &m_window);
        navSection->application.set(m_applicationMock);

        navSection->navigationController.set(m_controller);

        s->section = navSection;

        return s;
    }

    void setComponentWindow(AbstractNavigation* navigation, const QQuickWindow* window)
    {
        QQuickItem* parentItem = new QQuickItem(window->contentItem());
        navigation->setParent(parentItem);
    }

    void print(Section* s)
    {
        LOGI() << "section: " << s->section->name() << ", idx: " << s->section->index().to_string()
               << ", active: " << s->section->active() << ", enabled: " << s->section->enabled();

        for (const Panel* p : s->panels) {
            LOGI() << "panel: " << p->panel->name() << ", idx: " << p->panel->index().to_string()
                   << ", active: " << p->panel->active() << ", enabled: " << p->panel->enabled();

            for (const Control* c : p->controls) {
                LOGI() << "control: " << c->control->name() << ", idx: " << c->control->index().to_string()
                       << ", active: " << c->control->active() << ", enabled: " << c->control->enabled();
            }
        }
    }

    std::shared_ptr<NavigationController> m_controller;
    std::shared_ptr<rcommand::CommandDispatcher> m_dispatcher;
    std::shared_ptr<actions::ActionsDispatcher> m_actionsDispatcher;
    std::shared_ptr<MainWindowMock> m_mainWindow;
    std::shared_ptr<muse::ApplicationMock> m_applicationMock;

    QQuickWindow m_window;

    //! NOTE Garbage and references
    std::vector<INavigation::Index> m_idxsRefs;
};

TEST_F(Ui_NavigationControllerTests, FirstActiveOnNextSection)
{
    //! CASE Nothing active, and we call next section (F6)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [WHEN] Send action `next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The first section, the first panel, the first control must be activated
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(sect1->panels[0]->panel->active());
    EXPECT_TRUE(sect1->panels[0]->controls[0]->control->active());

    //! [THEN] The second section must not be activated
    EXPECT_FALSE(sect2->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnNextPanelOnExclusiveSection)
{
    //! CASE Nothing active, and we call next panel (Tab)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    sect1->section->setName("sect1");
    Section* sect2 = make_section(2, 2, 3);
    sect2->section->setName("sect2");

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Section 2 is exclusive and active
    sect2->section->setType(NavigationSection::QmlType::Exclusive);
    sect2->section->requestActive();

    //! [GIEN] Last panel of section 2 is active
    sect2->panels.back()->panel->requestActive();

    //! [WHEN] Send action `nav-next-panel` (usually Tab)
    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);

    //! [THEN] The second section, the first panel, the first control must be activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    //! [THEN] The first section must not be activated
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnNextSectionOnExclusiveSection)
{
    //! CASE Nothing active, and we call next section (F6)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Section 2 is exclusive and active
    sect2->section->setType(NavigationSection::QmlType::Exclusive);
    sect2->section->requestActive();

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The second section, the first panel, the first control must be activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    //! [THEN] The first section must not be activated
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnNextSectionExclusive)
{
    //! CASE Nothing active, and we call next section (F6)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Section 2 is exclusive,
    //!         we have this behavior for menus and dropdowns
    sect2->section->setType(NavigationSection::Exclusive);
    sect2->section->requestActive();

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The second section, the first panel, the first control must be activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    //! [THEN] The first section must not be activated
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnPrevSectionExclusive)
{
    //! CASE Nothing active, and we call previous section (Shift+F6)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Section 2 is exclusive,
    //!         we have this behavior for menus and dropdowns
    sect2->section->setType(NavigationSection::Exclusive);
    sect2->section->requestActive();

    //! [WHEN] Send action `nav-prev-section` (usually Shift+F6)
    m_dispatcher->dispatch(PREV_SECTION_COMMAND);

    //! [THEN] The second section, the first panel, the first control must be activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    //! [THEN] The first section must not be activated
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnNextPanel)
{
    //! CASE Nothing active, and we call next panel (Tab)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! DO Send action `nav-next-panel` (usually Tab)
    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);

    //! [THEN] The first section, the first panel, the first control must be activated
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(sect1->panels[0]->panel->active());
    EXPECT_TRUE(sect1->panels[0]->controls[0]->control->active());

    //! [THEN] The second section must not be activated
    EXPECT_FALSE(sect2->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnPrevSection)
{
    //! CASE Nothing active, and we call prev section (Shift+F6)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [WHEN] Send action `nav-prev-section` (usually Shift+F6)
    m_dispatcher->dispatch(PREV_SECTION_COMMAND);

    //! [THEN] The last section, the first panel, the first control must be activated.
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    //! [THEN] The first section must not be activated
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, FirstActiveOnPrevPanel)
{
    //! CASE Nothing active, and we call prev panel (Shift+Tab)

    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [WHEN] Send action `nav-prev-panel` (usually Shift+Tab)
    m_dispatcher->dispatch(PREV_PANEL_COMMAND);

    //! [THEN] The second section, the first panel, the first control must be activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    //! [THEN] The first section must not be activated
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, NextPanelOnSectionWithoutPanels)
{
    //! CASE The panels of the active section are gone (for example, they have been destroyed
    //! along with the page they belong to), and we call next panel (Tab)

    //! [GIVEN] Two section, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);
    ASSERT_TRUE(sect1->section->active());

    //! [GIVEN] Its panels have left the section, the section itself is still active
    for (Panel* p : sect1->panels) {
        p->panel->setSection(nullptr);
    }

    ASSERT_TRUE(sect1->section->active());

    //! [WHEN] Send action `nav-next-panel` (usually Tab)
    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);

    //! [THEN] The navigation has left the section it has nothing to navigate in
    EXPECT_FALSE(sect1->section->active());

    //! [THEN] The second section, the first panel, the first control must be activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_TRUE(sect2->panels[0]->panel->active());
    EXPECT_TRUE(sect2->panels[0]->controls[0]->control->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, PrevPanelOnSectionWithoutPanels)
{
    //! CASE The same as above, but we call prev panel (Shift+Tab)

    //! [GIVEN] Two section, the second one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    m_dispatcher->dispatch(PREV_PANEL_COMMAND);
    ASSERT_TRUE(sect2->section->active());

    //! [GIVEN] Its panels have left the section, the section itself is still active
    for (Panel* p : sect2->panels) {
        p->panel->setSection(nullptr);
    }

    ASSERT_TRUE(sect2->section->active());

    //! [WHEN] Send action `nav-prev-panel` (usually Shift+Tab)
    m_dispatcher->dispatch(PREV_PANEL_COMMAND);

    //! [THEN] The navigation has left the section it has nothing to navigate in
    EXPECT_FALSE(sect2->section->active());

    //! [THEN] The first section, its last panel, the first control must be activated
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(sect1->panels[1]->panel->active());
    EXPECT_TRUE(sect1->panels[1]->controls[0]->control->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, UserPressedSomeKeyHasActiveKey)
{
    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Section 1, panel 1 and control 1 are active
    sect1->panels[1]->controls[1]->control->requestActive();

    //! [GIVEN] The navigation is not activated
    m_controller->setIsHighlight(false);

    //! [WHEN] The user has requested the activation of navigation on any key
    m_dispatcher->dispatch(RIGHT_COMMAND);

    //! [THEN] Next control and highlight must be activated
    EXPECT_EQ(m_controller->activeControl(), sect1->panels[1]->controls[2]->control);
    EXPECT_TRUE(m_controller->isHighlight());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, UserClickedOnControlOnMainWindow)
{
    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Control on main window
    setComponentWindow(sect1->panels[1]->controls[1]->control, &m_window);

    //! [WHEN] The user has clicked on control
    sect1->panels[1]->controls[1]->control->requestActiveByInteraction();

    //! [THEN] The control is not activated
    EXPECT_NE(m_controller->activeControl(), sect1->panels[1]->controls[1]->control);
    EXPECT_FALSE(m_controller->isHighlight());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, UserClickedOnControlOnNonMainWindow)
{
    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    //! [GIVEN] Second section on non main window
    sect2->section->setType(NavigationSection::QmlType::Exclusive);
    QQuickWindow* controlWindow = new QQuickWindow();
    setComponentWindow(sect2->panels[1]->controls[1]->control, controlWindow);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [WHEN] The user has clicked on control of second section
    sect2->panels[1]->controls[1]->control->requestActiveByInteraction();

    //! [THEN] The control is activated
    EXPECT_EQ(m_controller->activeControl(), sect2->panels[1]->controls[1]->control);
    EXPECT_FALSE(m_controller->isHighlight());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, UserClickedNotOnControl)
{
    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] First control is active
    sect1->panels[1]->controls[1]->control->requestActive();

    //! [WHEN] User clicked somewhere on application
    QEvent mouseEvent(QEvent::MouseButtonPress);
    qApp->sendEvent(qApp, &mouseEvent);

    //! [THEN] The control is not active
    EXPECT_FALSE(sect1->panels[1]->controls[1]->control->active());
    EXPECT_FALSE(m_controller->isHighlight());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, UserClickedNotOnControlHasDefaultControl)
{
    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Active control
    sect1->panels[1]->controls[1]->control->requestActive();

    //! [GIVEN] Default control
    m_controller->setDefaultNavigationControl(sect2->panels[0]->controls[1]->control);

    //! [WHEN] User clicked somewhere on application
    QEvent mouseEvent(QEvent::MouseButtonPress);
    qApp->sendEvent(qApp, &mouseEvent);

    //! [THEN] Active control was reseted to default control
    EXPECT_FALSE(sect1->panels[1]->controls[1]->control->active());
    EXPECT_TRUE(sect2->panels[0]->controls[1]->control->active());
    EXPECT_FALSE(m_controller->isHighlight());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, UserClickedNotOnControlHasDefaultControlWithNotEnabledSection)
{
    //! [GIVEN] Two section, not active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] Active control
    sect1->panels[1]->controls[1]->control->requestActive();

    //! [GIVEN] Default control in not enabled section
    m_controller->setDefaultNavigationControl(sect2->panels[0]->controls[1]->control);
    sect2->section->setEnabled(false);

    //! [WHEN] User clicked somewhere on application
    QEvent mouseEvent(QEvent::MouseButtonPress);
    qApp->sendEvent(qApp, &mouseEvent);

    //! [THEN] Active control was reseted
    EXPECT_FALSE(sect1->panels[1]->controls[1]->control->active());
    EXPECT_FALSE(sect2->panels[0]->controls[1]->control->active());
    EXPECT_FALSE(m_controller->isHighlight());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, RightWrapsToFirstOnHorizontalPanel)
{
    //! CASE On a horizontal panel, pressing Right at the last control wraps to the first

    //! [GIVEN] Section with a horizontal panel containing 3 controls
    Section* sect = make_directed_section(1, 3, NavigationPanel::Horizontal);
    m_controller->reg(sect->section);

    //! [GIVEN] Last control is active
    sect->panels[0]->controls[2]->control->requestActive();

    //! [WHEN] Navigate right
    m_dispatcher->dispatch(RIGHT_COMMAND);

    //! [THEN] First control becomes active (wraps around)
    EXPECT_EQ(m_controller->activeControl(), sect->panels[0]->controls[0]->control);

    delete sect;
}

TEST_F(Ui_NavigationControllerTests, LeftWrapsToLastOnHorizontalPanel)
{
    //! CASE On a horizontal panel, pressing Left at the first control wraps to the last

    //! [GIVEN] Section with a horizontal panel containing 3 controls
    Section* sect = make_directed_section(1, 3, NavigationPanel::Horizontal);
    m_controller->reg(sect->section);

    //! [GIVEN] First control is active
    sect->panels[0]->controls[0]->control->requestActive();

    //! [WHEN] Navigate left
    m_dispatcher->dispatch(LEFT_COMMAND);

    //! [THEN] Last control becomes active (wraps around)
    EXPECT_EQ(m_controller->activeControl(), sect->panels[0]->controls[2]->control);

    delete sect;
}

TEST_F(Ui_NavigationControllerTests, DownWrapsToFirstOnVerticalPanel)
{
    //! CASE On a vertical panel, pressing Down at the last control wraps to the first

    //! [GIVEN] Section with a vertical panel containing 3 controls
    Section* sect = make_directed_section(1, 3, NavigationPanel::Vertical);
    m_controller->reg(sect->section);

    //! [GIVEN] Last control is active
    sect->panels[0]->controls[2]->control->requestActive();

    //! [WHEN] Navigate down
    m_dispatcher->dispatch(DOWN_COMMAND);

    //! [THEN] First control becomes active (wraps around)
    EXPECT_EQ(m_controller->activeControl(), sect->panels[0]->controls[0]->control);

    delete sect;
}

TEST_F(Ui_NavigationControllerTests, UpWrapsToLastOnVerticalPanel)
{
    //! CASE On a vertical panel, pressing Up at the first control wraps to the last

    //! [GIVEN] Section with a vertical panel containing 3 controls
    Section* sect = make_directed_section(1, 3, NavigationPanel::Vertical);
    m_controller->reg(sect->section);

    //! [GIVEN] First control is active
    sect->panels[0]->controls[0]->control->requestActive();

    //! [WHEN] Navigate up
    m_dispatcher->dispatch(UP_COMMAND);

    //! [THEN] Last control becomes active (wraps around)
    EXPECT_EQ(m_controller->activeControl(), sect->panels[0]->controls[2]->control);

    delete sect;
}

TEST_F(Ui_NavigationControllerTests, NextSectionGoesToPrioritySection)
{
    //! CASE A priority section is activated by the next F6, wherever the navigation is

    //! [GIVEN] Three sections, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);
    Section* sect3 = make_section(3, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);
    m_controller->reg(sect3->section);

    sect1->section->requestActive();

    //! [GIVEN] The third section is set as the priority one
    m_controller->setPrioritySection(sect3->section);

    //! [WHEN] Send action `nav-next-section`
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The priority section is activated instead of the second one
    EXPECT_TRUE(sect3->section->active());
    EXPECT_TRUE(sect3->panels[0]->panel->active());
    EXPECT_TRUE(sect3->panels[0]->controls[0]->control->active());
    EXPECT_FALSE(sect1->section->active());
    EXPECT_FALSE(sect2->section->active());

    //! [WHEN] Send the action again
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The request was one-shot: the cycle continues from the priority section
    EXPECT_TRUE(sect1->section->active());
    EXPECT_FALSE(sect3->section->active());

    delete sect1;
    delete sect2;
    delete sect3;
}

TEST_F(Ui_NavigationControllerTests, PrevSectionGoesToPrioritySection)
{
    //! CASE A priority section is also activated by the prev section navigation (Shift+F6)

    //! [GIVEN] Three sections, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);
    Section* sect3 = make_section(3, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);
    m_controller->reg(sect3->section);

    sect2->section->requestActive();

    //! [GIVEN] The third section is set as the priority one
    m_controller->setPrioritySection(sect3->section);

    //! [WHEN] Send action `nav-prev-section` (usually Shift+F6)
    m_dispatcher->dispatch(PREV_SECTION_COMMAND);

    //! [THEN] The priority section is activated on its first panel (a jump, not a cycle step)
    EXPECT_TRUE(sect3->section->active());
    EXPECT_TRUE(sect3->panels[0]->panel->active());
    EXPECT_FALSE(sect1->section->active());
    EXPECT_FALSE(sect2->section->active());

    delete sect1;
    delete sect2;
    delete sect3;
}

TEST_F(Ui_NavigationControllerTests, NextSectionGoesToPrioritySectionWithNoActiveSection)
{
    //! CASE Nothing is active and a priority section is set

    //! [GIVEN] Two sections, nothing active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    //! [GIVEN] The second section is set as the priority one
    m_controller->setPrioritySection(sect2->section);

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The priority section is activated instead of the first one
    EXPECT_TRUE(sect2->section->active());
    EXPECT_FALSE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, NextSectionSkipsDisabledPrioritySection)
{
    //! CASE The priority section was disabled before the next section navigation

    //! [GIVEN] Three sections, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);
    Section* sect3 = make_section(3, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);
    m_controller->reg(sect3->section);

    sect1->section->requestActive();

    //! [GIVEN] The third section is set as the priority one, then disabled
    m_controller->setPrioritySection(sect3->section);
    sect3->section->setEnabled(false);

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The usual next section is activated
    EXPECT_TRUE(sect2->section->active());
    EXPECT_FALSE(sect3->section->active());

    //! [WHEN] The priority section is enabled again and the action is sent again
    sect3->section->setEnabled(true);
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The request was cleared: the cycle continues as usual
    EXPECT_TRUE(sect3->section->active());

    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);
    EXPECT_TRUE(sect1->section->active());

    delete sect1;
    delete sect2;
    delete sect3;
}

TEST_F(Ui_NavigationControllerTests, NextSectionContinuesFromActivePrioritySection)
{
    //! CASE The priority section is already the active one

    //! [GIVEN] Two sections, the second one is active and set as the priority one
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    sect2->section->requestActive();
    m_controller->setPrioritySection(sect2->section);

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The cycle continues as usual, the navigation does not get stuck
    EXPECT_TRUE(sect1->section->active());
    EXPECT_FALSE(sect2->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, ActivationByOtherMeansClearsPrioritySection)
{
    //! CASE The priority section was visited before the next section navigation

    //! [GIVEN] Three sections, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);
    Section* sect3 = make_section(3, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);
    m_controller->reg(sect3->section);

    sect1->section->requestActive();

    //! [GIVEN] The third section is set as the priority one, then activated directly
    m_controller->setPrioritySection(sect3->section);
    sect3->section->requestActive();

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The request was cleared by the visit: the cycle continues as usual
    EXPECT_TRUE(sect1->section->active());
    EXPECT_FALSE(sect3->section->active());

    delete sect1;
    delete sect2;
    delete sect3;
}

TEST_F(Ui_NavigationControllerTests, ExclusiveSectionWinsOverPrioritySection)
{
    //! CASE An exclusive section is active while a priority section is set

    //! [GIVEN] A regular and an exclusive section, the exclusive one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* exclusive = make_section(2, 2, 3);
    exclusive->section->setType(NavigationSection::QmlType::Exclusive);

    m_controller->reg(sect1->section);
    m_controller->reg(exclusive->section);

    exclusive->section->requestActive();

    //! [GIVEN] The regular section is set as the priority one
    m_controller->setPrioritySection(sect1->section);

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The navigation stays trapped in the exclusive section
    EXPECT_TRUE(exclusive->section->active());
    EXPECT_FALSE(sect1->section->active());

    //! [WHEN] The exclusive section goes away and the action is sent again
    m_controller->unreg(exclusive->section);
    exclusive->section->setActive(false);
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The request stayed armed: the priority section is activated
    EXPECT_TRUE(sect1->section->active());

    delete sect1;
    delete exclusive;
}

TEST_F(Ui_NavigationControllerTests, UnregClearsPrioritySection)
{
    //! CASE The priority section was unregistered before the next section navigation

    //! [GIVEN] Two sections, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    sect1->section->requestActive();

    //! [GIVEN] The second section is set as the priority one, then unregistered
    m_controller->setPrioritySection(sect2->section);
    m_controller->unreg(sect2->section);

    //! [WHEN] Send action `nav-next-section` (usually F6)
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    //! [THEN] The cycle wraps to the first section, no dangling access
    EXPECT_TRUE(sect1->section->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, DisablingActiveSectionRestoresLastActiveControl)
{
    //! CASE The priority-activated section is disabled while active (e.g. the last toast is dismissed)

    //! [GIVEN] Two sections, the first one is active on its second panel
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    sect1->section->requestActive();
    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);

    NavigationControl* prevControl = sect1->panels[1]->controls[0]->control;
    EXPECT_TRUE(prevControl->active());

    //! [GIVEN] The second section is priority-activated (usually F6)
    m_controller->setPrioritySection(sect2->section);
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);
    EXPECT_TRUE(sect2->section->active());

    //! [WHEN] The active section is disabled
    sect2->section->setEnabled(false);

    //! [THEN] The navigation goes back to the control that was active before
    EXPECT_FALSE(sect2->section->active());
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(prevControl->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, DisablingSectionEnteredByPanelNavigationRestoresLastActiveControl)
{
    //! CASE The section is entered by panel navigation (usually Tab) after its priority request was consumed

    //! [GIVEN] Two sections, the first one is active, the second one once requested priority
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    sect1->section->requestActive();
    m_controller->setPrioritySection(sect2->section);

    //! [GIVEN] The priority jump was consumed and the navigation is back on the first section
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);
    EXPECT_TRUE(sect1->section->active());

    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);

    NavigationControl* prevControl = sect1->panels[1]->controls[0]->control;
    EXPECT_TRUE(prevControl->active());

    //! [GIVEN] Tab past the last panel enters the second section
    m_dispatcher->dispatch(NEXT_PANEL_COMMAND);
    EXPECT_TRUE(sect2->section->active());

    //! [WHEN] The active section is disabled
    sect2->section->setEnabled(false);

    //! [THEN] The navigation goes back to the control that was active before
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(prevControl->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, RestoreFallsBackToDefaultControlWhenLastControlIsGone)
{
    //! CASE The control that was active before was destroyed in the meantime

    //! [GIVEN] Two sections, the first one is active, and a default control
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    sect1->section->requestActive();

    NavigationControl* defaultControl = sect1->panels[1]->controls[1]->control;
    m_controller->setDefaultNavigationControl(defaultControl);

    //! [GIVEN] The second section is priority-activated, then the remembered control is destroyed
    m_controller->setPrioritySection(sect2->section);
    m_dispatcher->dispatch(NEXT_SECTION_COMMAND);

    delete sect1->panels[0]->controls[0]->control;
    sect1->panels[0]->controls[0]->control = nullptr;

    //! [WHEN] The active section is disabled
    sect2->section->setEnabled(false);

    //! [THEN] The navigation falls back to the default control
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(defaultControl->active());

    delete sect1;
    delete sect2;
}

TEST_F(Ui_NavigationControllerTests, DisablingInactiveSectionDoesNotMoveNavigation)
{
    //! CASE A section that requested priority is disabled while another one holds the navigation

    //! [GIVEN] Two sections, the first one is active
    Section* sect1 = make_section(1, 2, 3);
    Section* sect2 = make_section(2, 2, 3);

    m_controller->reg(sect1->section);
    m_controller->reg(sect2->section);

    sect1->section->requestActive();
    m_controller->setPrioritySection(sect2->section);

    //! [WHEN] The inactive section is disabled
    sect2->section->setEnabled(false);

    //! [THEN] The navigation stays where it is
    EXPECT_TRUE(sect1->section->active());
    EXPECT_TRUE(sect1->panels[0]->controls[0]->control->active());

    delete sect1;
    delete sect2;
}
