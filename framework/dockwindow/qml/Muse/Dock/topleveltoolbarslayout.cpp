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

#include "topleveltoolbarslayout.h"

#include "kddockwidgets/src/Config.h"
#include "kddockwidgets/src/core/DockWidget.h"
#include "kddockwidgets/src/core/Layout.h"
#include "kddockwidgets/src/core/MainWindow.h"
#include "kddockwidgets/src/qtquick/views/MainWindow.h"

#include "dockpageview.h"
#include "docktoolbarview.h"
#include "dockwindow.h"

using namespace muse::dock;

namespace muse::dock {
//! NOTE: Toolbar's maximum lenght, see DockToolBar.qml
static constexpr int UNCONSTRAINED_TOOLBAR_WIDTH = 16777215;
}

TopLevelToolBarsLayout::TopLevelToolBarsLayout(KDDockWidgets::QtQuick::MainWindow* mainWindow, DockWindow* window)
    : QObject(window), m_mainWindow(mainWindow), m_window(window)
{
}

void TopLevelToolBarsLayout::setUpPageConnections(const DockPageView* page)
{
    for (DockToolBarView* toolBar : topLevelToolBars(page)) {
        connect(toolBar, &DockToolBarView::floatingChanged,
                this, &TopLevelToolBarsLayout::scheduleRelayout, Qt::UniqueConnection);

        connect(toolBar, &DockToolBarView::contentSizeChanged,
                this, &TopLevelToolBarsLayout::scheduleRelayout, Qt::UniqueConnection);

        connect(toolBar, &DockToolBarView::visibleChanged,
                this, &TopLevelToolBarsLayout::scheduleRelayout, Qt::UniqueConnection);
    }
}

void TopLevelToolBarsLayout::scheduleRelayout()
{
    m_window->polish();
}

void TopLevelToolBarsLayout::relayout()
{
    DockPageView* page = currentPage();
    if (!page) {
        return;
    }

    const int width = availableWidth();

    adjustContentForAvailableSpace(page);
    alignToolBars(page, width);

    //! NOTE: Force the layouting items to pick up the current dock minimums
    for (DockBase* dock : page->allDocks()) {
        if (dock) {
            dock->syncLayoutItemMinSize();
        }
    }

    if (!applyLayoutSizeToFitWindow(width)) {
        scheduleRelayout();
    }
}

void TopLevelToolBarsLayout::adjustContentForAvailableSpace(const DockPageView* page)
{
    if (!page) {
        return;
    }

    const int spaceWidth = availableWidth();
    const int separator = separatorThickness();

    auto adjustDocks = [spaceWidth, separator](QList<DockBase*> docks) {
        int width = separator * std::max(0, int(docks.size()) - 1);
        for (DockBase* dock : docks) {
            width += dock->contentWidth();
        }

        docks.erase(std::remove_if(docks.begin(), docks.end(), [](const DockBase* dock){
            return dock->compactPriorityOrder() == -1;
        }), docks.end());

        if (docks.empty()) {
            return;
        }

        std::sort(docks.begin(), docks.end(), [](const DockBase* dock1, DockBase* dock2) {
            return dock1->compactPriorityOrder() < dock2->compactPriorityOrder();
        });

        if (width > spaceWidth) {
            for (DockBase* dock : docks) {
                if (width <= spaceWidth) {
                    break;
                }

                if (!dock->isCompact()) {
                    dock->setIsCompact(true);

                    width -= dock->nonCompactWidth();
                    width += dock->width();
                }
            }
        } else {
            for (int i = docks.size() - 1; i >= 0; i--) {
                DockBase* dock = docks.at(i);
                if (!dock->isCompact()) {
                    continue;
                }

                int actualWidth = dock->contentWidth();
                int nonCompactWidth = dock->nonCompactWidth();
                if (width - actualWidth + nonCompactWidth <= spaceWidth) {
                    dock->setIsCompact(false);
                }

                break;
            }
        }
    };

    QList<DockBase*> topLevelToolBarsDocks;

    for (DockToolBarView* toolBar : topLevelToolBars(page)) {
        if (!isToolBarFloating(toolBar) && toolBar->isVisible()) {
            topLevelToolBarsDocks << toolBar;
        }
    }

    adjustDocks(topLevelToolBarsDocks);
}

void TopLevelToolBarsLayout::alignToolBars(const DockPageView* page, int availableWidth)
{
    const QList<DockToolBarView*> topToolBars = topLevelToolBars(page);

    //! NOTE: Reset paddings/pins first, so that widths from a previous
    //! configuration do not accumulate
    for (DockToolBarView* toolBar : topToolBars) {
        toolBar->setMaximumWidth(UNCONSTRAINED_TOOLBAR_WIDTH);
        toolBar->setMinimumWidth(toolBar->contentWidth());
    }

    DockToolBarView* lastLeftToolBar = nullptr;
    DockToolBarView* lastCentralToolBar = nullptr;

    int leftBlockWidth = 0;
    int centralBlockWidth = 0;
    int rightBlockWidth = 0;

    int leftCount = 0;
    int centralCount = 0;
    int rightCount = 0;

    const int separator = separatorThickness();

    for (DockToolBarView* toolBar : topToolBars) {
        if (toolBar->floating() || !toolBar->isVisible()) {
            continue;
        }

        switch (static_cast<DockToolBarAlignment::Type>(toolBar->alignment())) {
        case DockToolBarAlignment::Left:
            lastLeftToolBar = toolBar;
            leftBlockWidth += toolBar->contentWidth();
            ++leftCount;
            break;
        case DockToolBarAlignment::Center:
            lastCentralToolBar = toolBar;
            centralBlockWidth += toolBar->contentWidth();
            ++centralCount;
            break;
        case DockToolBarAlignment::Right:
            rightBlockWidth += toolBar->contentWidth();
            ++rightCount;
            break;
        }
    }

    leftBlockWidth += separator * std::max(0, leftCount - 1);
    centralBlockWidth += separator * std::max(0, centralCount - 1);
    rightBlockWidth += separator * std::max(0, rightCount - 1);

    const int separatorLeftCentral = (leftCount > 0 && centralCount > 0) ? separator : 0;
    const int separatorCentralRight = (centralCount > 0 && rightCount > 0) ? separator : 0;
    const int separatorLeftRight = (leftCount > 0 && centralCount == 0 && rightCount > 0) ? separator : 0;

    const int occupied = leftBlockWidth + centralBlockWidth + rightBlockWidth
                         + separatorLeftCentral + separatorCentralRight + separatorLeftRight;

    const int freeSpace = availableWidth - occupied;
    if (freeSpace <= 0) {
        return;
    }

    DockToolBarView* grower = lastLeftToolBar ? lastLeftToolBar : lastCentralToolBar;
    if (!grower) {
        //! NOTE: No toolbar to absorb the free space; leave all the toolbars
        //! unpinned so the layout can distribute it
        return;
    }

    int paddingForLastCentral = 0;
    if (lastCentralToolBar && lastCentralToolBar != grower) {
        paddingForLastCentral = (availableWidth - centralBlockWidth) / 2 - rightBlockWidth - separatorCentralRight;
        paddingForLastCentral = std::min(std::max(paddingForLastCentral, 0), freeSpace);
    }

    for (DockToolBarView* toolBar : topToolBars) {
        if (toolBar->floating() || !toolBar->isVisible() || toolBar == grower) {
            continue;
        }

        const int pinnedWidth = (toolBar == lastCentralToolBar)
                                ? toolBar->contentWidth() + paddingForLastCentral
                                : toolBar->contentWidth();

        //! NOTE: max before min
        toolBar->setMaximumWidth(pinnedWidth);
        toolBar->setMinimumWidth(pinnedWidth);
    }
}

bool TopLevelToolBarsLayout::applyLayoutSizeToFitWindow(int targetWidth)
{
    KDDockWidgets::Core::MainWindow* mw = m_mainWindow ? m_mainWindow->mainWindow() : nullptr;
    KDDockWidgets::Core::Layout* layout = mw ? mw->layout() : nullptr;
    if (!layout) {
        return true;
    }

    const int applyWidth = std::max(targetWidth, layout->layoutMinimumSize().width());

    if (applyWidth != layout->layoutSize().width()) {
        layout->setLayoutSize(QSize(applyWidth, layout->layoutSize().height()));
    }

    return layout->layoutSize().width() == applyWidth;
}

DockPageView* TopLevelToolBarsLayout::currentPage() const
{
    return m_window->currentPage();
}

QList<DockToolBarView*> TopLevelToolBarsLayout::topLevelToolBars(const DockPageView* page) const
{
    return m_window->topLevelToolBars(page);
}

int TopLevelToolBarsLayout::availableWidth() const
{
    return int(m_window->width());
}

int TopLevelToolBarsLayout::separatorThickness() const
{
    return KDDockWidgets::Config::self(m_window->iocContext()->id).separatorThickness();
}

bool TopLevelToolBarsLayout::isToolBarFloating(const DockToolBarView* toolBar) const
{
    return toolBar->dockWidget()->isFloating();
}
