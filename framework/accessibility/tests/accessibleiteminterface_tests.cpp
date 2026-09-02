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

#include "accessibility/iaccessible.h"
#include "accessibility/internal/accessibleiteminterface.h"
#include "accessibility/internal/accessibleobject.h"

using namespace muse;
using namespace muse::accessibility;

class Accessibility_ItemInterfaceTests : public ::testing::Test
{
public:

    //! NOTE Records the write side of the text interface, so we can check that
    //! AccessibleItemInterface forwards it instead of dropping it (as it used to,
    //! which broke braille display cursor routing keys).
    class RecordingItem : public IAccessible
    {
    public:
        const IAccessible* accessibleParent() const override { return nullptr; }
        size_t accessibleChildCount() const override { return 0; }
        IAccessible* accessibleChild(size_t) const override { return nullptr; }
        QWindow* accessibleWindow() const override { return nullptr; }
        muse::modularity::ContextPtr iocContext() const override { return muse::modularity::globalCtx(); }
        IAccessible::Role accessibleRole() const override { return IAccessible::EditableText; }
        QString accessibleName() const override { return QString(); }
        QString accessibleDescription() const override { return QString(); }
        bool accessibleState(State) const override { return true; }
        QRect accessibleRect() const override { return QRect(); }
        bool accessibleIgnored() const override { return false; }

        QVariant accessibleValue() const override { return QVariant(); }
        QVariant accessibleMaximumValue() const override { return QVariant(); }
        QVariant accessibleMinimumValue() const override { return QVariant(); }
        QVariant accessibleValueStepSize() const override { return QVariant(); }

        void accessibleSelection(int, int*, int*) const override { }
        int accessibleSelectionCount() const override { return m_selectionCount; }

        int accessibleCursorPosition() const override { return 0; }

        QString accessibleText(int, int) const override { return QString(); }
        QString accessibleTextBeforeOffset(int, TextBoundaryType, int*, int*) const override { return QString(); }
        QString accessibleTextAfterOffset(int, TextBoundaryType, int*, int*) const override { return QString(); }
        QString accessibleTextAtOffset(int, TextBoundaryType, int*, int*) const override { return QString(); }
        int accessibleCharacterCount() const override { return 0; }

        void accessibleSetSelection(int selectionIndex, int startOffset, int endOffset) override
        {
            setSelectionCalls.push_back({ selectionIndex, startOffset, endOffset });
        }

        void accessibleRemoveSelection(int selectionIndex) override
        {
            removeSelectionCalls.push_back(selectionIndex);
        }

        void accessibleSetCursorPosition(int position) override
        {
            setCursorPositionCalls.push_back(position);
        }

        int accessibleRowIndex() const override { return 0; }

        async::Channel<IAccessible::Property, Val> accessiblePropertyChanged() const override { return m_propertyChanged; }
        async::Channel<IAccessible::State, bool> accessibleStateChanged() const override { return m_stateChanged; }
        void setState(State, bool) override { }

        struct SetSelectionCall {
            int selectionIndex = 0;
            int startOffset = 0;
            int endOffset = 0;
        };

        std::vector<SetSelectionCall> setSelectionCalls;
        std::vector<int> removeSelectionCalls;
        std::vector<int> setCursorPositionCalls;

        int m_selectionCount = 0;

        async::Channel<IAccessible::Property, Val> m_propertyChanged;
        async::Channel<IAccessible::State, bool> m_stateChanged;
    };

    void SetUp() override
    {
        m_item = std::make_unique<RecordingItem>();
        m_object = std::make_unique<AccessibleObject>(m_item.get());
        m_interface = std::make_unique<AccessibleItemInterface>(m_object.get());
    }

    std::unique_ptr<RecordingItem> m_item;
    std::unique_ptr<AccessibleObject> m_object;
    std::unique_ptr<AccessibleItemInterface> m_interface;
};

TEST_F(Accessibility_ItemInterfaceTests, AddSelectionIsForwardedAsSetSelection)
{
    //! [GIVEN] Nothing has been written yet
    EXPECT_TRUE(m_item->setSelectionCalls.empty());

    //! [WHEN] A screen reader selects a range
    //! NOTE This is the path a cursor routing key takes on Windows: NVDA selects a range
    //! and Qt's UIA bridge turns ITextRangeProvider::Select() into addSelection()
    m_interface->addSelection(3, 7);

    //! [THEN] It reached the item as selection 0
    ASSERT_EQ(m_item->setSelectionCalls.size(), size_t(1));
    EXPECT_EQ(m_item->setSelectionCalls[0].selectionIndex, 0);
    EXPECT_EQ(m_item->setSelectionCalls[0].startOffset, 3);
    EXPECT_EQ(m_item->setSelectionCalls[0].endOffset, 7);
}

TEST_F(Accessibility_ItemInterfaceTests, DegenerateAddSelectionIsForwarded)
{
    //! [WHEN] A cursor routing key is pressed, i.e. an empty range is selected
    m_interface->addSelection(5, 5);

    //! [THEN] It reached the item, rather than being dropped
    ASSERT_EQ(m_item->setSelectionCalls.size(), size_t(1));
    EXPECT_EQ(m_item->setSelectionCalls[0].startOffset, 5);
    EXPECT_EQ(m_item->setSelectionCalls[0].endOffset, 5);
}

TEST_F(Accessibility_ItemInterfaceTests, SetSelectionIsForwarded)
{
    //! [WHEN] A selection is set explicitly
    m_interface->setSelection(0, 1, 4);

    //! [THEN] It reached the item unchanged
    ASSERT_EQ(m_item->setSelectionCalls.size(), size_t(1));
    EXPECT_EQ(m_item->setSelectionCalls[0].selectionIndex, 0);
    EXPECT_EQ(m_item->setSelectionCalls[0].startOffset, 1);
    EXPECT_EQ(m_item->setSelectionCalls[0].endOffset, 4);
}

TEST_F(Accessibility_ItemInterfaceTests, RemoveSelectionIsForwarded)
{
    //! [WHEN] A selection is removed
    m_interface->removeSelection(0);

    //! [THEN] It reached the item
    ASSERT_EQ(m_item->removeSelectionCalls.size(), size_t(1));
    EXPECT_EQ(m_item->removeSelectionCalls[0], 0);
}

TEST_F(Accessibility_ItemInterfaceTests, SetCursorPositionIsForwarded)
{
    //! [WHEN] The caret is moved (the path used on Linux/AT-SPI)
    m_interface->setCursorPosition(9);

    //! [THEN] It reached the item
    ASSERT_EQ(m_item->setCursorPositionCalls.size(), size_t(1));
    EXPECT_EQ(m_item->setCursorPositionCalls[0], 9);
}

TEST_F(Accessibility_ItemInterfaceTests, SelectionCountIsForwarded)
{
    //! [GIVEN] The item reports one selected range
    m_item->m_selectionCount = 1;

    //! [THEN] So does the interface
    EXPECT_EQ(m_interface->selectionCount(), 1);
}
