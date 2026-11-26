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
#include "qtclipboard.h"

#include <QClipboard>
#include <QGuiApplication>

#include "global/qt/qtmimedataconvert.h"

using namespace muse;
using namespace muse::ui;

MimeData QtClipboard::mimeData() const
{
    return fromQtMimeData(QGuiApplication::clipboard()->mimeData());
}

void QtClipboard::setMimeData(const MimeData& data)
{
    //! NOTE The clipboard takes the ownership
    QGuiApplication::clipboard()->setMimeData(toQtMimeData(data));
}

