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

#include "../iextensionsession.h"
#include "scriptengine.h"

namespace muse::extensions {
struct Manifest;

class ExtensionSession final : public IExtensionSession
{
public:
    ExtensionSession(const modularity::ContextPtr& iocContext, const Manifest& manifest, const io::path_t& scriptPath);

    Ret evaluate() override;
    Ret call(const QString& function, const QJSValueList& arguments = {}, QJSValue* result = nullptr) override;

    QJSValue exports() const override;
    QJSValue toScriptValue(const QVariant& value) override;
    QJSValue wrapQObject(QObject* object) override;

private:
    ScriptEngine m_engine;
};
} // namespace muse::extensions
