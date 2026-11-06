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

#include <memory>

#include "accessibility/iaccessibilitycontroller.h"
#include "modularity/ioc.h"
#include "ui/qml/Muse/Ui/qmlaccessible.h"

using namespace muse;

class Ui_QmlAccessibleTests : public ::testing::Test
{
public:

    //! NOTE AccessibleItem resolves the controller on construction and destruction,
    //! so it needs one to be registered in its ioc context.
    class ControllerStub : public accessibility::IAccessibilityController
    {
    public:
        void reg(accessibility::IAccessible*) override { }
        void unreg(accessibility::IAccessible*) override { }
        bool isReg(accessibility::IAccessible*) const override { return false; }

        void announce(const QString&) override { }
        QString announcement() const override { return QString(); }

        const accessibility::IAccessible* accessibleRoot() const override { return nullptr; }
        const accessibility::IAccessible* lastFocused() const override { return nullptr; }

        bool needToVoicePanelInfo() const override { return false; }
        QString currentPanelAccessibleName() const override { return QString(); }

        bool isEnabled() const override { return true; }

        void setIgnoreQtAccessibilityEvents(bool) override { }
    };

    void SetUp() override
    {
        m_iocCtx = std::make_shared<modularity::Context>(2);
        modularity::ioc(m_iocCtx)->registerExport<accessibility::IAccessibilityController>("utest", new ControllerStub());

        m_item = std::make_unique<muse::ui::AccessibleItem>(m_iocCtx);
    }

    void TearDown() override
    {
        m_item.reset();
        modularity::removeIoC(m_iocCtx);
    }

    modularity::ContextPtr m_iocCtx;
    std::unique_ptr<muse::ui::AccessibleItem> m_item;
};

TEST_F(Ui_QmlAccessibleTests, SelectionCountCountsRangesNotCharacters)
{
    //! [GIVEN] Some text with nothing selected
    m_item->setText("hello world");

    //! [THEN] There are no selected ranges
    EXPECT_EQ(m_item->accessibleSelectionCount(), 0);

    //! [WHEN] A range of several characters is selected
    m_item->setSelectionStart(0);
    m_item->setSelectionEnd(5);
    m_item->setSelectedText("hello");

    //! [THEN] That is one range, not one per selected character
    //! NOTE Qt's UIA bridge calls removeSelection() once per range before selecting a
    //! new one, so reporting a character count here made every cursor routing key
    //! press do redundant work
    EXPECT_EQ(m_item->accessibleSelectionCount(), 1);
}

TEST_F(Ui_QmlAccessibleTests, CaretWriteIsIgnoredWithoutATextItem)
{
    //! [GIVEN] No text item has been set (e.g. a non-editable control)
    ASSERT_FALSE(m_item->textItem());

    //! [WHEN] A screen reader tries to move the caret
    //! [THEN] Nothing happens, and in particular nothing crashes
    m_item->accessibleSetCursorPosition(3);
    m_item->accessibleSetSelection(0, 1, 4);
    m_item->accessibleRemoveSelection(0);
}
