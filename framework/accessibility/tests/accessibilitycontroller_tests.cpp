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

#include <memory>

#include <QKeyEvent>
#include <QRect>
#include <QString>
#include <QVariant>
#include <QWindow>

#include "async/channel.h"
#include "iapplication.h"

#include "accessibility/iaccessible.h"
#include "accessibility/internal/accessibilitycontroller.h"
#include "accessibility/internal/accessibleobject.h"
#include "global/tests/mocks/applicationmock.h"

#include "modularity/ioc.h"
#include "ui/tests/mocks/mainwindowmock.h"
#include "global/tests/mocks/applicationmock.h"
#include "mocks/accessibleapprootobjectmock.h"
#include "ui/tests/mocks/navigationmocks.h"

class QEvent;

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

using namespace muse;
using namespace muse::accessibility;

static const muse::modularity::ContextPtr utestCtx = std::make_shared<muse::modularity::Context>(1);

class Accessibility_ControllerTests : public ::testing::Test
{
public:

    void SetUp() override
    {
        m_controller = std::make_shared<AccessibilityController>(utestCtx);

        m_controller->setAccessibilityEnabled(true);

        m_mainWindow = std::make_shared<NiceMock<muse::ui::MainWindowMock> >();
        m_controller->mainWindow.set(m_mainWindow);

        m_application = std::make_shared<NiceMock<ApplicationMock> >();
        m_controller->application.set(m_application);

        m_appRootObject = std::make_shared<NiceMock<AccessibleAppRootObjectMock> >();
        m_controller->appRootObject.set(m_appRootObject);

        m_navigationController = std::make_shared<NiceMock<muse::ui::NavigationControllerMock> >();
        m_controller->navigationController.set(m_navigationController);
    }

    class AccessibleItem : public IAccessible
    {
    public:
        void setParent(IAccessible* parent) { m_parent = parent; }

        const IAccessible* accessibleParent() const override { return m_parent; }
        size_t accessibleChildCount() const override { return m_children.size(); }
        IAccessible* accessibleChild(size_t i) const override { return m_children.at(i); }
        QWindow* accessibleWindow() const override { return nullptr; }
        muse::modularity::ContextPtr iocContext() const override { return muse::modularity::globalCtx(); }
        IAccessible::Role accessibleRole() const override { return m_role; }
        QString accessibleName() const override { return QString(); }
        QString accessibleDescription() const override { return QString(); }
        bool accessibleState(State st) const override { return st == State::Enabled ? m_enabled : true; }
        QRect accessibleRect() const override { return m_rect; }
        bool accessibleIgnored() const override { return false; }

        QVariant accessibleValue() const override { return QString(); }
        QVariant accessibleMaximumValue() const override { return QString(); }
        QVariant accessibleMinimumValue() const override { return QString(); }
        QVariant accessibleValueStepSize() const override { return QString(); }

        void accessibleSelection(int, int*, int*) const override { }
        int accessibleSelectionCount() const override { return 0; }

        int accessibleCursorPosition() const override { return 0; }

        QString accessibleText(int, int) const override { return QString(); }

        QString accessibleTextBeforeOffset(int, TextBoundaryType, int*, int*) const override { return QString(); }
        QString accessibleTextAfterOffset(int, TextBoundaryType, int*, int*) const override { return QString(); }
        QString accessibleTextAtOffset(int, TextBoundaryType, int*, int*) const override { return QString(); }
        int accessibleCharacterCount() const override { return 0; }

        int accessibleRowIndex() const override { return 0; }

        async::Channel<IAccessible::Property, Val> accessiblePropertyChanged() const override
        {
            return m_propertyChanged;
        }

        async::Channel<IAccessible::State, bool> accessibleStateChanged() const override
        {
            return m_stateChanged;
        }

        void setState(State, bool) override {}

        async::Channel<IAccessible::Property, Val> m_propertyChanged;
        async::Channel<IAccessible::State, bool> m_stateChanged;

        IAccessible* m_parent = nullptr;
        std::vector<IAccessible*> m_children;
        IAccessible::Role m_role = IAccessible::NoRole;
        QRect m_rect;
        bool m_enabled = true;
    };

    void TearDown() override
    {
        for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
            m_controller->unreg(it->get());
        }
        m_items.clear();

        //! NOTE: need if tested class was created as a shared pointer
        testing::Mock::AllowLeak(m_mainWindow.get());
        testing::Mock::AllowLeak(m_application.get());
        testing::Mock::AllowLeak(m_appRootObject.get());
        testing::Mock::AllowLeak(m_navigationController.get());
    }

    //! Makes an item that is owned by the test; without a parent, it is a child of the root
    AccessibleItem* makeItem(IAccessible::Role role, const QRect& rect, AccessibleItem* parent = nullptr)
    {
        auto item = std::make_unique<AccessibleItem>();
        item->m_role = role;
        item->m_rect = rect;

        if (parent) {
            item->setParent(parent);
            parent->m_children.push_back(item.get());
        } else {
            item->setParent(m_controller.get());
        }

        m_items.push_back(std::move(item));
        return m_items.back().get();
    }

    static const IAccessible* itemOf(const QAccessibleInterface* iface)
    {
        return iface ? static_cast<AccessibleObject*>(iface->object())->item() : nullptr;
    }

    const IAccessible* root() const
    {
        return m_controller.get();
    }

    AccessibleItem* makeItemWithRegisteredParent()
    {
        AccessibleItem* parent = new AccessibleItem();
        parent->setParent(m_controller.get());
        m_controller->reg(parent);

        AccessibleItem* item = new AccessibleItem();
        item->setParent(parent);

        return item;
    }

#ifdef Q_OS_LINUX
    void expectDispatchEventOnFocus(QEvent** event)
    {
        //! For Linux it needs to send spontaneous event for canceling reading the name of previous control on accessibility
        QWindow* window = new QWindow();
        EXPECT_CALL(*m_mainWindow, qWindow()).WillOnce(Return(window));
        EXPECT_CALL(*m_application, notify(window, _)).WillOnce(DoAll(SaveArg<1>(event), Return(true)));
    }

    void notExpectDispatchEventOnFocus()
    {
        //! If accessibility is active then the accessibility system should handle sended event,
        //! Otherwise, the item that is currently on focus will process the signal
        //! So, shouldn't dispatch event if accessibility is not active
        QWindow* window = new QWindow();
        EXPECT_CALL(*m_mainWindow, qWindow()).Times(0);
        EXPECT_CALL(*m_application, notify(window, _)).Times(0);
    }

    void checkDispatchEventOnFocus(const QEvent* event)
    {
        EXPECT_TRUE(event);
        EXPECT_TRUE(event->spontaneous());
    }

#else
    void expectDispatchEventOnFocus(QEvent**) {}
    void notExpectDispatchEventOnFocus() {}
    void checkDispatchEventOnFocus(const QEvent*) {}
#endif

    std::shared_ptr<AccessibilityController> m_controller;
    std::shared_ptr<muse::ui::MainWindowMock> m_mainWindow;
    std::shared_ptr<AccessibleAppRootObjectMock> m_appRootObject;
    std::shared_ptr<ApplicationMock> m_application;
    std::shared_ptr<muse::ui::NavigationControllerMock> m_navigationController;

    std::vector<std::unique_ptr<AccessibleItem> > m_items;
};

TEST_F(Accessibility_ControllerTests, SendEventOnFocusChanged)
{
    //! [GIVEN] Accessibility is active
    ON_CALL(*m_appRootObject, isAccessibilityActive()).WillByDefault(Return(true));
    //! [GIVEN] Navigation has an active section (so isEnabled() returns true)
    ON_CALL(*m_navigationController, activeSection()).WillByDefault(Return(reinterpret_cast<muse::ui::INavigationSection*>(1)));

    //! [GIVEN] Two items
    AccessibleItem* item1 = makeItemWithRegisteredParent();
    AccessibleItem* item2 = makeItemWithRegisteredParent();

    //! [GIVEN] Register items
    m_controller->reg(item1);
    m_controller->reg(item2);

    //! [GIVEN] First is on focus
    item1->accessibleStateChanged().send(IAccessible::State::Focused, true);

    QEvent* event = nullptr;
    expectDispatchEventOnFocus(&event);

    //! [WHEN] Focus on second item
    item2->accessibleStateChanged().send(IAccessible::State::Focused, true);

    //! [THEN] It is expected that there will be an event dispatch when the focus is changed
    checkDispatchEventOnFocus(event);

    delete item1;
    delete item2;

    //! NOTE: need if tested class was created as a shared pointer
    testing::Mock::AllowLeak(m_mainWindow.get());
    testing::Mock::AllowLeak(m_application.get());
    testing::Mock::AllowLeak(m_appRootObject.get());
    testing::Mock::AllowLeak(m_navigationController.get());
}

TEST_F(Accessibility_ControllerTests, NotSendEventOnFocusChangedIfAccessibilityIsNotActive)
{
    //! [GIVEN] Accessibility is not active
    ON_CALL(*m_appRootObject, isAccessibilityActive()).WillByDefault(Return(false));

    //! [GIVEN] Two items
    AccessibleItem* item1 = makeItemWithRegisteredParent();
    AccessibleItem* item2 = makeItemWithRegisteredParent();

    //! [GIVEN] Register items
    m_controller->reg(item1);
    m_controller->reg(item2);

    notExpectDispatchEventOnFocus();

    //! [GIVEN] First is on focus
    item1->accessibleStateChanged().send(IAccessible::State::Focused, true);

    //! [WHEN] Focus on second item
    item2->accessibleStateChanged().send(IAccessible::State::Focused, true);

    delete item1;
    delete item2;

    //! NOTE: need if tested class was created as a shared pointer
    testing::Mock::AllowLeak(m_mainWindow.get());
    testing::Mock::AllowLeak(m_application.get());
    testing::Mock::AllowLeak(m_appRootObject.get());
    testing::Mock::AllowLeak(m_navigationController.get());
}

TEST_F(Accessibility_ControllerTests, ChildAtFindsTheMostSpecificItem)
{
    //! [GIVEN] A panel with a list item and a smaller button that overlap without being nested
    AccessibleItem* panel = makeItem(IAccessible::Panel, QRect(0, 0, 200, 200));
    AccessibleItem* listItem = makeItem(IAccessible::ListItem, QRect(0, 0, 100, 50), panel);
    AccessibleItem* button = makeItem(IAccessible::Button, QRect(10, 10, 20, 20), panel);
    m_controller->reg(panel);

    //! [THEN] The button is found where it covers the list item, and the list item elsewhere
    EXPECT_EQ(itemOf(m_controller->childAt(root(), 15, 15)), button);
    EXPECT_EQ(itemOf(m_controller->childAt(root(), 80, 40)), listItem);

    //! [THEN] Nothing is found outside every item
    EXPECT_EQ(m_controller->childAt(root(), 500, 500), nullptr);
}

TEST_F(Accessibility_ControllerTests, ChildAtLooksIntoItemsWithoutGeometry)
{
    //! [GIVEN] A button inside a container that has no geometry of its own
    AccessibleItem* container = makeItem(IAccessible::Panel, QRect());
    AccessibleItem* button = makeItem(IAccessible::Button, QRect(10, 10, 20, 20), container);
    m_controller->reg(container);

    //! [THEN] The button is found
    EXPECT_EQ(itemOf(m_controller->childAt(root(), 15, 15)), button);
}

TEST_F(Accessibility_ControllerTests, ChildAtSkipsWhatThePlatformIgnores)
{
    //! [GIVEN] A button, covered by a smaller item without a role and by a smaller disabled button
    AccessibleItem* panel = makeItem(IAccessible::Panel, QRect(0, 0, 200, 200));
    AccessibleItem* button = makeItem(IAccessible::Button, QRect(0, 0, 50, 50), panel);
    makeItem(IAccessible::NoRole, QRect(0, 0, 10, 10), panel);
    AccessibleItem* disabled = makeItem(IAccessible::Button, QRect(20, 20, 10, 10), panel);
    disabled->m_enabled = false;
    m_controller->reg(panel);

    //! [THEN] The button is found under both
    EXPECT_EQ(itemOf(m_controller->childAt(root(), 5, 5)), button);
    EXPECT_EQ(itemOf(m_controller->childAt(root(), 25, 25)), button);
}
