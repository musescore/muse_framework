/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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

#include <QtQml/QJSValue>
#include <QVariant>

#include "global/types/retval.h"

class QObject;

namespace muse::extensions {
class IExtensionSession
{
public:
    virtual ~IExtensionSession() = default;

    virtual Ret evaluate() = 0;
    virtual Ret call(const QString& function, const QJSValueList& arguments = {}, QJSValue* result = nullptr) = 0;

    virtual QJSValue exports() const = 0;
    virtual QJSValue toScriptValue(const QVariant& value) = 0;

    // Keeps ownership in C++
    virtual QJSValue wrapQObject(QObject* object) = 0;
};
} // namespace muse::extensions
