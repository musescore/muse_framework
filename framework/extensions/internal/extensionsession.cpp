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
#include "extensionsession.h"

#include "extensions/extensionstypes.h"

using namespace muse;
using namespace muse::extensions;

ExtensionSession::ExtensionSession(const modularity::ContextPtr& iocContext, const Manifest& manifest, const io::path_t& scriptPath)
    : m_engine(iocContext, muse::api::ApiContext {
    manifest.apiversion,
    io::dirpath(manifest.path),
    manifest.uri.toString(),
})
{
    m_engine.setScriptPath(scriptPath);
}

Ret ExtensionSession::evaluate()
{
    return m_engine.evaluate();
}

Ret ExtensionSession::call(const QString& function, const QJSValueList& arguments, QJSValue* result)
{
    return m_engine.call(function, arguments, result);
}

QJSValue ExtensionSession::exports() const
{
    return m_engine.exports();
}

QJSValue ExtensionSession::toScriptValue(const QVariant& value)
{
    return m_engine.jsEngine()->toScriptValue(value);
}

QJSValue ExtensionSession::wrapQObject(QObject* object)
{
    QJSEngine::setObjectOwnership(object, QJSEngine::CppOwnership);
    return m_engine.jsEngine()->newQObject(object);
}
