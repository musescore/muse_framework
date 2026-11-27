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

#include "dialogview.h"

#include <functional>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QPointer>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <QTimer>

#ifdef Q_OS_MAC
#include "internal/platform/macos/macoswindowlevelcontroller.h"
#endif

#include "ui/inavigation.h"

#include "log.h"

using namespace Qt::Literals::StringLiterals;

using namespace muse::uicomponents;

DialogView::DialogView(QQuickItem* parent)
    : WindowView(parent)
{
    setObjectName("DialogView");
    setRetCode(Ret::Code::Ok);
}

//! NOTE A separate application-wide filter is used (rather than DialogView::eventFilter,
//! which WindowView already installs on the view) so that WindowView's own handling
//! of the view's events is not invoked twice.
class DialogView::EnterKeyFilter : public QObject
{
public:
    explicit EnterKeyFilter(DialogView* dialog)
        : QObject(dialog), m_dialog(dialog) {}

    bool eventFilter(QObject*, QEvent* event) override
    {
        return m_dialog->handleShortcutOverride(event);
    }

private:
    DialogView* m_dialog = nullptr;
};

void DialogView::initView()
{
    QQuickWindow::setDefaultAlphaBuffer(m_frameless);

    WindowView::initView();

    m_view->setFlags(Qt::Dialog);
    m_view->setIcon(QIcon(uiConfiguration()->appIconPath().toString()));

    if (m_frameless) {
        m_view->setColor(QColor(Qt::transparent));
    } else {
        auto updateBackgroundColor = [this]() {
            if (!m_view) {
                return;
            }

            QString bgColorStr = uiConfiguration()->currentTheme().values.value(ui::BACKGROUND_PRIMARY_COLOR).toString();
            m_view->setColor(QColor(bgColorStr));
        };

        uiConfiguration()->currentThemeChanged().onNotify(this, updateBackgroundColor);

        updateBackgroundColor();
    }
}

void DialogView::beforeOpen()
{
    IF_ASSERT_FAILED(m_view) {
        return;
    }

    //! NOTE Set default title
    if (m_title.isEmpty()) {
        setTitle(application()->title());
    }

    m_view->setTitle(m_title);
    m_view->setFlag(Qt::FramelessWindowHint, m_frameless);

#ifdef MUSE_MODULE_UI_DISABLE_MODALITY
    m_view->setModality(Qt::NonModal);
#else
    m_view->setModality(m_modal ? Qt::ApplicationModal : Qt::NonModal);
#endif

    //! NOTE ok will be if they call accept
    setRetCode(Ret::Code::Cancel);

    windowsController()->regWindow(m_view->winId());

    if (!m_enterKeyFilter) {
        m_enterKeyFilter = new EnterKeyFilter(this);
    }
    qApp->installEventFilter(m_enterKeyFilter);
}

void DialogView::onHidden()
{
    WindowView::onHidden();

    if (m_enterKeyFilter) {
        qApp->removeEventFilter(m_enterKeyFilter);
    }

    windowsController()->unregWindow(m_view->winId());
}

bool DialogView::handleShortcutOverride(QEvent* event)
{
    if (event->type() != QEvent::ShortcutOverride) {
        return false;
    }

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->key() != Qt::Key_Return && keyEvent->key() != Qt::Key_Enter) {
        return false;
    }

    if (!shortcutOverrideIsAllowed()) {
        return false;
    }

    //! NOTE Rule: Enter/Return triggers the dialog's accent button, unless the currently
    //! highlighted control is itself a button, in which case that button is triggered.
    QObject* target = findEnterTarget();
    if (!target) {
        return false;
    }

    //! NOTE Both spontaneous and synthetic events are received; trigger only once
    if (event->spontaneous()) {
        QPointer<QObject> guard(target);
        QTimer::singleShot(0, target, [guard]() {
            if (guard) {
                QMetaObject::invokeMethod(guard, "triggered");
            }
        });
    }

    event->accept();
    return true;
}

bool DialogView::shortcutOverrideIsAllowed() const
{
    //! NOTE Only the dialog that owns the active navigation section may react
    //! (some windows, e.g. dropdowns, don't take window focus).
    if (!m_view) {
        return false;
    }

    const ui::INavigationSection* section = navigationController()->activeSection();
    return section && section->window() == m_view;
}

QObject* DialogView::findEnterTarget() const
{
    //! NOTE The dialog content lives in its own item tree (not among QObject children of this view),
    //! so walk the visual hierarchy, which also covers Loader/Repeater-created items.
    QQuickItem* root = contentItem();
    if (!root) {
        return nullptr;
    }

    QObject* accentNavigation = nullptr;
    QObject* highlightedNavigation = nullptr;

    std::function<void(QQuickItem*)> visit = [&](QQuickItem* item) {
        if (highlightedNavigation || !item->isVisible()) {
            return;
        }

        const QString className = QString::fromLatin1(item->metaObject()->className());

        if (className.contains(u"FlatButton"_s)) {
            QObject* navigation = item->property("navigation").value<QObject*>();
            if (navigation) {
                if (navigation->property("highlight").toBool()) {
                    highlightedNavigation = navigation;
                    return;
                }

                if (!accentNavigation && item->property("accentButton").toBool() && item->isEnabled()) {
                    accentNavigation = navigation;
                }
            }
        }

        //! NOTE Embedded windows (WindowContainer) keep their content in a separate window
        if (className.contains(u"WindowContainer"_s)) {
            QQuickWindow* embedded = item->property("window").value<QQuickWindow*>();
            if (embedded && embedded->contentItem()) {
                visit(embedded->contentItem());
            }
        }

        for (QQuickItem* child : item->childItems()) {
            visit(child);
        }
    };

    visit(root);

    return highlightedNavigation ? highlightedNavigation : accentNavigation;
}

void DialogView::afterShow()
{
#ifdef Q_OS_MAC
    if (m_alwaysAboveApp) {
        MacOSWindowLevelController::setAlwaysAboveApp(m_view);
    }
#endif
}

void DialogView::updateGeometry()
{
    QRect anchorRect = currentScreenGeometry();

    const QWindow* qMainWindow = mainWindow()->qWindow();
    bool mainWindowVisible = qMainWindow->isVisible();
    QRect referenceRect = qMainWindow->geometry();

    if (referenceRect.isEmpty() || !mainWindowVisible) {
        referenceRect = anchorRect;
    }

    QMargins frameMargins = m_view->frameMargins(); // actual window frame margins (title bar height etc.)
    QRect dlgRect = viewGeometry(); // content geometry
    dlgRect = dlgRect.marginsAdded(frameMargins);

    // position the dialog in the center of the main window
    dlgRect.moveLeft(referenceRect.x() + (referenceRect.width() - dlgRect.width()) / 2);
    dlgRect.moveTop(referenceRect.y() + (referenceRect.height() - dlgRect.height()) / 2);

    // try to move the dialog if it doesn't fit on the screen

    if (dlgRect.left() <= anchorRect.left()) {
        dlgRect.moveLeft(anchorRect.left());
    }

    if (dlgRect.top() <= anchorRect.top()) {
        dlgRect.moveTop(anchorRect.top());
    }

    if (dlgRect.right() >= anchorRect.right()) {
        dlgRect.moveRight(anchorRect.right());
    }

    if (dlgRect.bottom() >= anchorRect.bottom()) {
        dlgRect.moveBottom(anchorRect.bottom());
    }

    // if after moving the dialog does not fit on the screen, then adjust the size of the dialog
    if (!anchorRect.contains(dlgRect)) {
        dlgRect = anchorRect.intersected(dlgRect);
    }

    dlgRect = dlgRect.marginsRemoved(frameMargins);

    m_globalPos = dlgRect.topLeft();

    setContentWidth(dlgRect.width());
    setContentHeight(dlgRect.height());
}

QRect DialogView::viewGeometry() const
{
    return QRect(m_globalPos.toPoint(), QSize(contentWidth(), contentHeight()));
}

QString DialogView::title() const
{
    return m_title;
}

void DialogView::setTitle(QString title)
{
    if (m_title == title) {
        return;
    }

    m_title = title;
    emit titleChanged(m_title);

    if (isOpened()) {
        m_view->setTitle(title);
    }
}

QString DialogView::objectId() const
{
    return m_objectId;
}

void DialogView::setObjectId(QString objectId)
{
    if (m_objectId == objectId) {
        return;
    }

    m_objectId = objectId;
    emit objectIdChanged(m_objectId);
}

bool DialogView::modal() const
{
    return m_modal;
}

void DialogView::setModal(bool modal)
{
    if (m_modal == modal) {
        return;
    }

    m_modal = modal;
    emit modalChanged(m_modal);
}

bool DialogView::frameless() const
{
    return m_frameless;
}

void DialogView::setFrameless(bool frameless)
{
    if (m_frameless == frameless) {
        return;
    }

    m_frameless = frameless;
    emit framelessChanged(m_frameless);
}

bool DialogView::resizable() const
{
    return m_resizable;
}

void DialogView::setResizable(bool resizable)
{
    if (m_resizable == resizable) {
        return;
    }

    m_resizable = resizable;
    emit resizableChanged(m_resizable);

    if (isOpened()) {
        updateSize(m_view->size());
    }
}

bool DialogView::alwaysAboveApp() const
{
    return m_alwaysAboveApp;
}

void DialogView::setAlwaysAboveApp(bool alwaysAbove)
{
    if (m_alwaysAboveApp == alwaysAbove) {
        return;
    }

    m_alwaysAboveApp = alwaysAbove;
    emit alwaysAboveAppChanged();
}

QVariantMap DialogView::ret() const
{
    return m_ret;
}

void DialogView::setRet(QVariantMap ret)
{
    if (m_ret == ret) {
        return;
    }

    m_ret = ret;
    emit retChanged(m_ret);
}

void DialogView::setRetCode(Ret::Code code)
{
    QVariantMap ret;
    ret[u"errcode"_s] = static_cast<int>(code);
    setRet(ret);
}

void DialogView::show()
{
    open();
}

void DialogView::hide()
{
    close();
}

void DialogView::raise()
{
    if (isOpened()) {
        m_view->raise();
    }
}

void DialogView::accept()
{
    setRetCode(Ret::Code::Ok);
    close();
}

void DialogView::reject(int code)
{
    if (code > 0) {
        setRetCode(static_cast<Ret::Code>(code));
    } else {
        setRetCode(Ret::Code::Cancel);
    }

    close();
}
