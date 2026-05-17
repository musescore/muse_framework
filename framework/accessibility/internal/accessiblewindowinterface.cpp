/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "accessiblewindowinterface.h"

#include <QWindow>

#include "accessibilitycontroller.h"

#include "modularity/ioc.h"
#include "../iaccessibleapprootobject.h"

#include "translation.h"
#include "log.h"

//#define MUSE_ACCESSIBILITY_TRACE

#undef MYLOG
#ifdef MUSE_ACCESSIBILITY_TRACE
#define MYLOG() LOGI()
#else
#define MYLOG() LOGN()
#endif

using namespace muse::accessibility;

AccessibleWindowInterface::AccessibleWindowInterface(QObject* window)
{
    m_window = qobject_cast<QWindow*>(window);
}

AccessibleObject* AccessibleWindowInterface::resolveWindowRoot() const
{
    if (!m_window) {
        return nullptr;
    }

    auto appRoot = muse::modularity::globalIoc()->resolve<IAccessibleAppRootObject>("accessibility");
    if (!appRoot) {
        return nullptr;
    }

#ifdef Q_OS_LINUX
    // On Linux do not walk transientParent to prevent Orca frow re-interperting
    // the whole window tree, that results in VO delay.
    return appRoot->windowRoot(m_window);
#else
    QWindow* w = m_window;
    while (w) {
        if (AccessibleObject* root = appRoot->windowRoot(w)) {
            return root;
        }
        w = w->transientParent();
    }
    return nullptr;
#endif
}

bool AccessibleWindowInterface::isValid() const
{
    return m_window != nullptr;
}

QObject* AccessibleWindowInterface::object() const
{
    return m_window;
}

QWindow* AccessibleWindowInterface::window() const
{
    return m_window;
}

QRect AccessibleWindowInterface::rect() const
{
    if (!m_window) {
        return {};
    }
    return QRect(m_window->x(), m_window->y(), m_window->width(), m_window->height());
}

QAccessibleInterface* AccessibleWindowInterface::parent() const
{
    auto appRoot = muse::modularity::globalIoc()->resolve<IAccessibleAppRootObject>("accessibility");
    if (!appRoot) {
        return nullptr;
    }
    return QAccessible::queryAccessibleInterface(appRoot->asQObject());
}

int AccessibleWindowInterface::childCount() const
{
    AccessibleObject* root = resolveWindowRoot();
    if (!root) {
        return 0;
    }

    auto controller = root->controller().lock();
    if (!controller) {
        return 0;
    }

    int count = controller->childCount(root->item());
    MYLOG() << "item: " << root->item()->accessibleName() << ", childCount: " << count;
    return count;
}

QAccessibleInterface* AccessibleWindowInterface::child(int index) const
{
    AccessibleObject* root = resolveWindowRoot();
    if (!root) {
        return nullptr;
    }

    auto controller = root->controller().lock();
    if (!controller) {
        return nullptr;
    }

    QAccessibleInterface* iface = controller->child(root->item(), index);
    MYLOG() << "item: " << root->item()->accessibleName() << ", child: " << index << " " <<
        (iface ? iface->text(QAccessible::Name) : "null");
    return iface;
}

int AccessibleWindowInterface::indexOfChild(const QAccessibleInterface* iface) const
{
    AccessibleObject* root = resolveWindowRoot();
    if (!root) {
        return -1;
    }

    auto controller = root->controller().lock();
    if (!controller) {
        return -1;
    }

    int idx = controller->indexOfChild(root->item(), iface);
    MYLOG() << "item: " << root->item()->accessibleName() << ", indexOfChild: " <<
        (iface ? iface->text(QAccessible::Name) : "null") << " = " << idx;
    return idx;
}

QAccessibleInterface* AccessibleWindowInterface::childAt(int, int) const
{
    NOT_IMPLEMENTED;
    return nullptr;
}

QAccessibleInterface* AccessibleWindowInterface::focusChild() const
{
    AccessibleObject* root = resolveWindowRoot();
    if (!root) {
        return nullptr;
    }

    auto controller = root->controller().lock();
    if (!controller) {
        return nullptr;
    }

    QAccessibleInterface* child = controller->focusedChild(root->item());
    MYLOG() << "item: " << root->item()->accessibleName() << ", focused child: " << (child ? child->text(QAccessible::Name) : "null");
    return child;
}

QAccessible::State AccessibleWindowInterface::state() const
{
    QAccessible::State st;
    st.active = true;
    return st;
}

QAccessible::Role AccessibleWindowInterface::role() const
{
    return QAccessible::Window;
}

QString AccessibleWindowInterface::text(QAccessible::Text) const
{
    if (!m_window) {
        return {};
    }
    return m_window->title();
}

void AccessibleWindowInterface::setText(QAccessible::Text, const QString&)
{
    NOT_IMPLEMENTED;
}

void* AccessibleWindowInterface::interface_cast(QAccessible::InterfaceType)
{
    //! NOTE Not implemented
    return nullptr;
}
