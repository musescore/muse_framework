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

#include <functional>
#include <map>

#include <QObject>
#include <QString>
#include <QStringList>

namespace muse::testflow {
//! NOTE Lets the test case steps (JS functions) call the test code
class StepsHandler : public QObject
{
    Q_OBJECT

public:
    std::map<QString, std::function<bool()> > handlers;
    QStringList calls;

    Q_INVOKABLE bool call(const QString& name)
    {
        calls << name;

        auto it = handlers.find(name);
        return it != handlers.end() ? it->second() : true;
    }
};
}
