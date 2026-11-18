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
#include <gmock/gmock.h>

#include <QList>

#include "dockpageview.h"
#include "docktoolbarview.h"
#include "topleveltoolbarslayout.h"

using namespace muse;
using namespace muse::dock;

//! NOTE: Toolbar's maximum lenght, see DockToolBar.qml
static constexpr int UNCONSTRAINED_TOOLBAR_WIDTH = 16777215;

class Dock_TopLevelToolBarsLayoutTests : public ::testing::Test
{
public:

    class ToolBarView : public DockToolBarView
    {
    public:
        void setFloatingState(bool floating) { doSetFloating(floating); }
    };

    class TestTopLevelToolBarsLayout : public TopLevelToolBarsLayout
    {
    public:
        TestTopLevelToolBarsLayout()
            : TopLevelToolBarsLayout(nullptr, nullptr) {}

        void scheduleRelayout() override
        {
            ++scheduledRelayoutCount;
        }

        DockPageView* currentPage() const override { return page; }
        QList<DockToolBarView*> topLevelToolBars(const DockPageView*) const override { return toolBars; }
        int availableWidth() const override { return windowWidth; }
        int separatorThickness() const override { return separator; }
        bool isToolBarFloating(const DockToolBarView* toolBar) const override { return toolBar->floating(); }

        bool applyLayoutSizeToFitWindow(int targetWidth) override
        {
            appliedLayoutWidth = targetWidth;
            return layoutSizeFitsWindow;
        }

        QList<DockToolBarView*> toolBars;
        DockPageView* page = nullptr;
        int windowWidth = 0;
        int separator = 0;
        bool layoutSizeFitsWindow = true;

        int scheduledRelayoutCount = 0;
        int appliedLayoutWidth = -1;
    };

    void SetUp() override
    {
        m_layout = std::make_shared<TestTopLevelToolBarsLayout>();

        m_page = new DockPageView();
        m_layout->page = m_page;
    }

    void TearDown() override
    {
        qDeleteAll(m_layout->toolBars);
        delete m_page;
    }

    ToolBarView* addToolBar(int contentWidth, DockToolBarAlignment::Type alignment = DockToolBarAlignment::Left,
                            int compactPriorityOrder = -1)
    {
        ToolBarView* toolBar = new ToolBarView();
        toolBar->setContentWidth(contentWidth);
        toolBar->setWidth(contentWidth);
        toolBar->setAlignment(alignment);
        toolBar->setCompactPriorityOrder(compactPriorityOrder);

        m_layout->toolBars << toolBar;

        return toolBar;
    }

    void simulateCompactResize(ToolBarView* toolBar, int compactWidth)
    {
        QObject::connect(toolBar, &DockBase::isCompactChanged, toolBar, [toolBar, compactWidth]() {
            if (toolBar->isCompact()) {
                toolBar->setWidth(compactWidth);
            }
        });
    }

    void expectPinned(const DockToolBarView* toolBar, int width)
    {
        EXPECT_EQ(toolBar->minimumWidth(), width);
        EXPECT_EQ(toolBar->maximumWidth(), width);
    }

    void expectUnpinned(const DockToolBarView* toolBar)
    {
        EXPECT_EQ(toolBar->minimumWidth(), toolBar->contentWidth());
        EXPECT_EQ(toolBar->maximumWidth(), UNCONSTRAINED_TOOLBAR_WIDTH);
    }

    std::shared_ptr<TestTopLevelToolBarsLayout> m_layout;
    DockPageView* m_page = nullptr;
};

TEST_F(Dock_TopLevelToolBarsLayoutTests, ScheduleRelayoutOnToolBarChanged)
{
    //! [GIVEN] A page with a toolbar
    ToolBarView* toolBar = addToolBar(100);

    //! [WHEN] Connections are set up
    m_layout->setUpPageConnections(m_page);

    //! [THEN] No relayout has been scheduled yet
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 0);

    //! [WHEN] The toolbar's content size changes
    toolBar->setContentWidth(200);

    //! [THEN] A relayout is scheduled
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 1);

    //! [WHEN] The toolbar's floating state changes
    toolBar->setFloatingState(true);

    //! [THEN] A relayout is scheduled
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 2);

    //! [WHEN] The toolbar's visibility changes
    toolBar->setVisible(false);

    //! [THEN] A relayout is scheduled
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 3);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, PageConnectionsAreUnique)
{
    //! [GIVEN] A page with a toolbar
    ToolBarView* toolBar = addToolBar(100);

    //! [WHEN] Connections are set up twice
    m_layout->setUpPageConnections(m_page);
    m_layout->setUpPageConnections(m_page);

    //! [WHEN] The toolbar's content size changes
    toolBar->setContentWidth(200);

    //! [THEN] Only one relayout is scheduled
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 1);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, RelayoutDoesNothingWithoutPage)
{
    //! [GIVEN] No current page
    m_layout->page = nullptr;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] The layout size has not been touched
    EXPECT_EQ(m_layout->appliedLayoutWidth, -1);
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 0);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, RelayoutAppliesLayoutSizeToFitWindow)
{
    //! [GIVEN] A window
    m_layout->windowWidth = 500;

    //! [WHEN] Relayout is requested and the layout can be resized to the window
    m_layout->relayout();

    //! [THEN] The layout size has been applied, no extra relayout is scheduled
    EXPECT_EQ(m_layout->appliedLayoutWidth, 500);
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 0);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, RelayoutIsRescheduledIfLayoutSizeDoesNotFitWindow)
{
    //! [GIVEN] A window and a layout that cannot be resized to the window yet
    m_layout->windowWidth = 500;
    m_layout->layoutSizeFitsWindow = false;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] Another relayout is scheduled
    EXPECT_EQ(m_layout->appliedLayoutWidth, 500);
    EXPECT_EQ(m_layout->scheduledRelayoutCount, 1);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, CompactToolBarsThatDoNotFit)
{
    //! [GIVEN] Toolbars that do not fit into the window
    ToolBarView* first = addToolBar(300, DockToolBarAlignment::Left, 0);
    ToolBarView* second = addToolBar(300, DockToolBarAlignment::Left, 1);
    ToolBarView* nonCompactable = addToolBar(300, DockToolBarAlignment::Left);

    //! [GIVEN] The toolbars shrink to 50 px when they become compact
    simulateCompactResize(first, 50);
    simulateCompactResize(second, 50);

    m_layout->windowWidth = 700;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] Only the toolbar with the highest compact priority became compact,
    //! since that freed enough space: 900 - 300 + 50 = 650 <= 700
    EXPECT_TRUE(first->isCompact());
    EXPECT_FALSE(second->isCompact());
    EXPECT_FALSE(nonCompactable->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, NotCompactToolBarsThatFit)
{
    //! [GIVEN] Toolbars that fit into the window
    ToolBarView* first = addToolBar(300, DockToolBarAlignment::Left, 0);
    ToolBarView* second = addToolBar(300, DockToolBarAlignment::Left, 1);

    m_layout->windowWidth = 700;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] No toolbar became compact
    EXPECT_FALSE(first->isCompact());
    EXPECT_FALSE(second->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, NotCompactToolBarsThatFitExactly)
{
    //! [GIVEN] Toolbars that exactly fit into the window
    ToolBarView* first = addToolBar(300, DockToolBarAlignment::Left, 0);
    ToolBarView* second = addToolBar(300, DockToolBarAlignment::Left, 1);

    m_layout->windowWidth = 600;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] No toolbar became compact
    EXPECT_FALSE(first->isCompact());
    EXPECT_FALSE(second->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, SeparatorsAreCountedWhenCompacting)
{
    //! [GIVEN] Two toolbars whose contents fit into the window,
    //! but not together with the separator between them
    ToolBarView* first = addToolBar(300, DockToolBarAlignment::Left, 0);
    ToolBarView* second = addToolBar(300, DockToolBarAlignment::Left, 1);

    m_layout->separator = 10;
    m_layout->windowWidth = 605;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] The toolbars became compact
    EXPECT_TRUE(first->isCompact());
    EXPECT_TRUE(second->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, FloatingAndHiddenToolBarsAreIgnoredWhenCompacting)
{
    //! [GIVEN] A floating and a hidden toolbar that would not fit into the window
    ToolBarView* floating = addToolBar(500, DockToolBarAlignment::Left, 0);
    floating->setFloatingState(true);

    ToolBarView* hidden = addToolBar(500, DockToolBarAlignment::Left, 1);
    hidden->setVisible(false);

    //! [GIVEN] A docked toolbar that fits
    ToolBarView* docked = addToolBar(300, DockToolBarAlignment::Left, 2);

    m_layout->windowWidth = 400;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] Only the docked toolbar is taken into account, nothing became compact
    EXPECT_FALSE(floating->isCompact());
    EXPECT_FALSE(hidden->isCompact());
    EXPECT_FALSE(docked->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, ExpandCompactToolBarWhenItFitsAgain)
{
    //! [GIVEN] A compact toolbar with a non-compact width of 300
    ToolBarView* toolBar = addToolBar(300, DockToolBarAlignment::Left, 0);
    toolBar->setIsCompact(true);
    toolBar->setContentWidth(50);

    //! [GIVEN] The window has enough space for the non-compact width
    m_layout->windowWidth = 700;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] The toolbar is no longer compact
    EXPECT_FALSE(toolBar->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, ExpandCompactToolBarWhenItFitsExactly)
{
    //! [GIVEN] A compact toolbar whose non-compact width of 300
    //! exactly matches the window width
    ToolBarView* toolBar = addToolBar(300, DockToolBarAlignment::Left, 0);
    toolBar->setIsCompact(true);
    toolBar->setContentWidth(50);

    m_layout->windowWidth = 300;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] The toolbar is no longer compact
    EXPECT_FALSE(toolBar->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, KeepToolBarCompactWhenExpansionWouldNotFit)
{
    //! [GIVEN] A compact toolbar with a non-compact width of 300
    ToolBarView* toolBar = addToolBar(300, DockToolBarAlignment::Left, 0);
    toolBar->setIsCompact(true);
    toolBar->setContentWidth(50);

    //! [GIVEN] Another toolbar that occupies most of the window
    addToolBar(300, DockToolBarAlignment::Left);

    //! [GIVEN] The window has no space for the non-compact width
    m_layout->windowWidth = 500;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] The toolbar remains compact
    EXPECT_TRUE(toolBar->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, ExpandOnlyLastCompactedToolBarPerPass)
{
    //! [GIVEN] Two compact toolbars
    ToolBarView* first = addToolBar(300, DockToolBarAlignment::Left, 0);
    first->setIsCompact(true);
    first->setContentWidth(50);

    ToolBarView* second = addToolBar(300, DockToolBarAlignment::Left, 1);
    second->setIsCompact(true);
    second->setContentWidth(50);

    //! [GIVEN] The window has enough space for both non-compact widths
    m_layout->windowWidth = 1000;

    //! [WHEN] The content is adjusted for the available space
    m_layout->adjustContentForAvailableSpace(m_page);

    //! [THEN] Only the toolbar with the highest compact priority order is expanded;
    //! the other one will be expanded on a next pass
    EXPECT_TRUE(first->isCompact());
    EXPECT_FALSE(second->isCompact());
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, LastLeftToolBarAbsorbsFreeSpace)
{
    //! [GIVEN] Two left toolbars and a right toolbar in a window with free space
    ToolBarView* left1 = addToolBar(100, DockToolBarAlignment::Left);
    ToolBarView* left2 = addToolBar(100, DockToolBarAlignment::Left);
    ToolBarView* right = addToolBar(100, DockToolBarAlignment::Right);

    m_layout->windowWidth = 1000;
    m_layout->separator = 4;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] All the toolbars except the last left one are pinned to their content width,
    //! the last left toolbar absorbs the free space
    expectPinned(left1, 100);
    expectPinned(right, 100);
    expectUnpinned(left2);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, CentralToolBarIsPaddedToStayCentered)
{
    //! [GIVEN] Left, central and right toolbars in a window with free space
    ToolBarView* left = addToolBar(100, DockToolBarAlignment::Left);
    ToolBarView* central = addToolBar(200, DockToolBarAlignment::Center);
    ToolBarView* right = addToolBar(100, DockToolBarAlignment::Right);

    m_layout->windowWidth = 1000;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] The left toolbar absorbs the free space
    expectUnpinned(left);

    //! [THEN] The right toolbar is pinned to its content width
    expectPinned(right, 100);

    //! [THEN] The central toolbar is padded so that its content stays centered:
    //! (windowWidth - centralWidth) / 2 - rightWidth = (1000 - 200) / 2 - 100 = 300
    expectPinned(central, 200 + 300);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, LastCentralToolBarAbsorbsFreeSpaceWhenNoLeftToolBars)
{
    //! [GIVEN] Two central toolbars and a right toolbar in a window with free space
    ToolBarView* central1 = addToolBar(100, DockToolBarAlignment::Center);
    ToolBarView* central2 = addToolBar(100, DockToolBarAlignment::Center);
    ToolBarView* right = addToolBar(100, DockToolBarAlignment::Right);

    m_layout->windowWidth = 1000;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] The last central toolbar absorbs the free space, the others are pinned
    expectPinned(central1, 100);
    expectPinned(right, 100);
    expectUnpinned(central2);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, AllToolBarsAreUnpinnedWhenThereIsNoGrower)
{
    //! [GIVEN] Only right toolbars
    ToolBarView* right1 = addToolBar(100, DockToolBarAlignment::Right);
    ToolBarView* right2 = addToolBar(150, DockToolBarAlignment::Right);

    m_layout->windowWidth = 1000;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] No toolbar is pinned, the layout distributes the free space
    expectUnpinned(right1);
    expectUnpinned(right2);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, AllToolBarsAreUnpinnedWhenThereIsNoFreeSpace)
{
    //! [GIVEN] Toolbars that occupy more than the window width
    ToolBarView* left = addToolBar(600, DockToolBarAlignment::Left);
    ToolBarView* right = addToolBar(500, DockToolBarAlignment::Right);

    m_layout->windowWidth = 1000;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] No toolbar is pinned
    expectUnpinned(left);
    expectUnpinned(right);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, FloatingAndHiddenToolBarsAreNotPinned)
{
    //! [GIVEN] A floating and a hidden left toolbar
    ToolBarView* floating = addToolBar(100, DockToolBarAlignment::Left);
    floating->setFloatingState(true);

    ToolBarView* hidden = addToolBar(100, DockToolBarAlignment::Left);
    hidden->setVisible(false);

    //! [GIVEN] Docked left and right toolbars
    ToolBarView* left = addToolBar(100, DockToolBarAlignment::Left);
    ToolBarView* right = addToolBar(100, DockToolBarAlignment::Right);

    m_layout->windowWidth = 1000;

    //! [WHEN] Relayout is requested
    m_layout->relayout();

    //! [THEN] The floating and hidden toolbars are not pinned
    expectUnpinned(floating);
    expectUnpinned(hidden);

    //! [THEN] The docked toolbars are laid out as usual
    expectUnpinned(left);
    expectPinned(right, 100);
}

TEST_F(Dock_TopLevelToolBarsLayoutTests, RepeatedRelayoutDoesNotAccumulatePinnedWidths)
{
    //! [GIVEN] Left, central and right toolbars
    ToolBarView* central = addToolBar(200, DockToolBarAlignment::Center);
    addToolBar(100, DockToolBarAlignment::Left);
    addToolBar(100, DockToolBarAlignment::Right);

    //! [GIVEN] The layout has already been done once
    m_layout->windowWidth = 1000;
    m_layout->relayout();
    expectPinned(central, 200 + 300);

    //! [WHEN] The window shrinks and relayout is requested again
    m_layout->windowWidth = 800;
    m_layout->relayout();

    //! [THEN] The central toolbar is pinned according to the new window width:
    //! (800 - 200) / 2 - 100 = 200
    expectPinned(central, 200 + 200);
}
