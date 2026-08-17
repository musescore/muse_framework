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

#include <memory>

#include <gtest/gtest.h>

#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>

#include "../menuview.h"

using namespace muse::uicomponents;

namespace muse::uicomponents {
class MenuViewTestAccess
{
public:
    static void updateGeometry(MenuView& view)
    {
        view.updateGeometry();
    }
};
}

namespace {
class MainWindowStub final : public muse::ui::IMainWindow
{
public:
    void init(muse::ui::MainWindowBridge*) override {}
    void deinit() override {}

    QWindow* qWindow() const override { return nullptr; }

    void requestShowOnBack() override {}
    void requestShowOnFront() override {}

    bool isFullScreen() const override { return false; }
    muse::async::Notification isFullScreenChanged() const override { return {}; }
    void toggleFullScreen() override {}

    QScreen* screen() const override { return QGuiApplication::primaryScreen(); }
};

class TestMenuView final : public MenuView
{
public:
    using MenuView::MenuView;

    QPointF globalPosition() const { return m_globalPos; }

    void setMainWindowForTest(const std::shared_ptr<muse::ui::IMainWindow>& window)
    {
        mainWindow.set(window);
    }
};
}

TEST(MenuViewTests, OverflowingTopLevelMenuOpensLeftOfCursor)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    ASSERT_NE(screen, nullptr);

    const QRect availableGeometry = screen->availableGeometry();
    ASSERT_GT(availableGeometry.width(), 400);

    QQuickWindow parentWindow;
    parentWindow.setGeometry(availableGeometry);

    QQuickItem anchor(parentWindow.contentItem());
    anchor.setX(availableGeometry.width() - 100);
    anchor.setY(100);
    anchor.setSize(QSizeF(1, 1));

    QQuickItem content;
    TestMenuView menu(&anchor);
    menu.setMainWindowForTest(std::make_shared<MainWindowStub>());
    menu.setContentItem(&content);
    menu.setDesiredWidth(200);
    menu.setDesiredHeight(100);

    MenuViewTestAccess::updateGeometry(menu);

    const qreal cursorX = anchor.mapToGlobal(QPointF(0, 0)).x();
    const qreal contentRight = menu.globalPosition().x() + menu.padding() + menu.desiredWidth();

    EXPECT_LT(menu.globalPosition().x(), cursorX);
    EXPECT_DOUBLE_EQ(contentRight, cursorX);
    EXPECT_NE(contentRight, availableGeometry.right());
}
