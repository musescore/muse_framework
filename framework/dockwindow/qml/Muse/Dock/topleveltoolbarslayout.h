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

#include <QObject>

namespace KDDockWidgets::QtQuick {
class MainWindow;
}

namespace muse::dock {
class DockWindow;
class DockPageView;
class DockToolBarView;

class TopLevelToolBarsLayout : public QObject
{
    Q_OBJECT

public:
    TopLevelToolBarsLayout(KDDockWidgets::QtQuick::MainWindow* mainWindow, DockWindow* window);

    void setUpPageConnections(const DockPageView* page);

    void adjustContentForAvailableSpace(const DockPageView* page);
    void relayout();
    virtual void scheduleRelayout();

protected:
    //! NOTE: Virtual for tests
    virtual DockPageView* currentPage() const;
    virtual QList<DockToolBarView*> topLevelToolBars(const DockPageView* page) const;
    virtual int availableWidth() const;
    virtual int separatorThickness() const;
    virtual bool isToolBarFloating(const DockToolBarView* toolBar) const;
    virtual bool applyLayoutSizeToFitWindow(int targetWidth);

private:
    void alignToolBars(const DockPageView* page, int availableWidth);

    KDDockWidgets::QtQuick::MainWindow* m_mainWindow = nullptr;
    DockWindow* m_window = nullptr;
};
}
