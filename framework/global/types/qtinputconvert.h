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
#pragma once

#ifndef NO_QT_SUPPORT

#include <cstdlib>
#include <string>
#include <vector>

#include <QDropEvent>
#include <QHoverEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QWheelEvent>

#include "types/dragevent.h"
#include "types/inputevents.h"
#include "types/qtmimedata.h"

//! NOTE Conversion of the Qt input types and events to the toolkit-independent ones and back
namespace muse {
inline KeyboardModifiers fromQtModifiers(Qt::KeyboardModifiers modifiers)
{
    return keyboardModifiersFromInt(int(modifiers));
}

inline Qt::KeyboardModifiers toQtModifiers(KeyboardModifiers modifiers)
{
    return Qt::KeyboardModifiers::fromInt(int(modifiers));
}

inline MouseButtons fromQtMouseButtons(Qt::MouseButtons buttons)
{
    return mouseButtonsFromInt(int(buttons));
}

inline Qt::MouseButtons toQtMouseButtons(MouseButtons buttons)
{
    return Qt::MouseButtons::fromInt(int(buttons));
}

inline KeyEvent fromQtKeyEvent(const QKeyEvent* event)
{
    KeyEvent result;
    result.type = event->type() == QEvent::KeyRelease ? KeyEvent::Type::Release : KeyEvent::Type::Press;
    result.key = event->key();
    result.modifiers = fromQtModifiers(event->modifiers());
    result.text = String::fromQString(event->text());
    result.isAutoRepeat = event->isAutoRepeat();
    result.setAccepted(event->isAccepted());
    return result;
}

inline TextInputEvent fromQtInputMethodEvent(const QInputMethodEvent* event)
{
    TextInputEvent result;
    result.commitString = String::fromQString(event->commitString());
    result.preeditString = String::fromQString(event->preeditString());
    result.replacementStart = event->replacementStart();
    result.replacementLength = event->replacementLength();
    result.setAccepted(event->isAccepted());
    return result;
}

//! NOTE The position is rounded to whole pixels, like QMouseEvent::pos()
inline PointerEvent fromQtMouseEvent(const QMouseEvent* event)
{
    PointerEvent result;
    switch (event->type()) {
    case QEvent::MouseButtonPress: result.type = PointerEvent::Type::Press;
        break;
    case QEvent::MouseButtonRelease: result.type = PointerEvent::Type::Release;
        break;
    case QEvent::MouseButtonDblClick: result.type = PointerEvent::Type::DoubleClick;
        break;
    default: result.type = PointerEvent::Type::Move;
        break;
    }
    result.position = PointF::fromQPointF(QPointF(event->position().toPoint()));
    result.button = MouseButton(int(event->button()));
    result.buttons = fromQtMouseButtons(event->buttons());
    result.modifiers = fromQtModifiers(event->modifiers());
    result.setAccepted(event->isAccepted());
    return result;
}

inline HoverEvent fromQtHoverEvent(const QHoverEvent* event)
{
    HoverEvent result;
    result.position = PointF::fromQPointF(event->position());
    result.oldPosition = PointF::fromQPointF(event->oldPosF());
    result.modifiers = fromQtModifiers(event->modifiers());
    result.setAccepted(event->isAccepted());
    return result;
}

inline WheelEvent fromQtWheelEvent(const QWheelEvent* event)
{
    WheelEvent result;
    result.position = PointF::fromQPointF(event->position());
    result.pixelDelta = Point(event->pixelDelta().x(), event->pixelDelta().y());
    result.angleDelta = Point(event->angleDelta().x(), event->angleDelta().y());
    result.modifiers = fromQtModifiers(event->modifiers());
#ifdef Q_OS_LINUX
    // pixelDelta is unreliable on X11, so ignore it unless Wayland is used
    if (std::getenv("WAYLAND_DISPLAY") == nullptr) {
        result.pixelDelta = Point();
    }
#endif
    result.setAccepted(event->isAccepted());
    return result;
}

//! NOTE Only the given mime formats are copied from the mime data
inline DragEvent fromQtDragEvent(const QDropEvent* event, const QMimeData* mimeData, const QObject* thisView,
                                 const std::vector<std::string>& formats)
{
    DragEvent result;
    result.position = PointF::fromQPointF(event->position());
    result.modifiers = fromQtModifiers(event->modifiers());
    result.possibleActions = DropActions(Flag(int(event->possibleActions())));
    result.dropAction = DropAction(int(event->dropAction()));
    result.isFromThisView = event->source() == thisView;
    result.setAccepted(event->isAccepted());

    result.mimeData = fromQtMimeData(mimeData, formats);

    return result;
}

inline void applyToQtDragEvent(const DragEvent& dragEvent, QDropEvent* event)
{
    event->setDropAction(Qt::DropAction(int(dragEvent.dropAction)));
    event->setAccepted(dragEvent.isAccepted());
}
}

#endif // NO_QT_SUPPORT
