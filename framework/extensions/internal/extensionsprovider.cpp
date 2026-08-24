/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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
#include "extensionsprovider.h"

#include "global/io/path.h"

#include "extensionsession.h"
#include "../extensionbundle.h"
#include "legacy/extpluginrunner.h"

#include "../extensionserrors.h"

#include "log.h"

using namespace muse;
using namespace muse::extensions;

void ExtensionsProvider::reloadExtensions()
{
    extensionsRegister()->reload();
}

ManifestList ExtensionsProvider::manifestList(Filter filter) const
{
    return extensionsRegister()->manifestList(filter);
}

muse::async::Notification ExtensionsProvider::manifestListChanged() const
{
    return extensionsRegister()->manifestListChanged();
}

bool ExtensionsProvider::exists(const ExtensionUri& uri) const
{
    return extensionsRegister()->exists(uri);
}

const Manifest& ExtensionsProvider::manifest(const ExtensionUri& uri) const
{
    return extensionsRegister()->manifest(uri);
}

void ExtensionsProvider::setEnabled(const ExtensionUri& uri, bool enabled)
{
    extensionsRegister()->setEnabled(uri, enabled);
}

bool ExtensionsProvider::isEnabled(const ExtensionUri& uri) const
{
    return extensionsRegister()->isEnabled(uri);
}

async::Channel<ExtensionUri> ExtensionsProvider::enabledChanged() const
{
    return extensionsRegister()->enabledChanged();
}

Action ExtensionsProvider::action(const ExtensionQuery& query) const
{
    const Manifest& m = manifest(query.uri());
    IF_ASSERT_FAILED(m.actions.size() > 0) {
        return Action();
    }

    if (m.actions.size() == 1) {
        return m.actions.at(0);
    }

    std::string code = query.param("action").toString();
    for (const Action& a : m.actions) {
        if (a.code == code) {
            return a;
        }
    }

    LOGE() << "not found action: " << code << ", query: " << query;
    return Action();
}

muse::Ret ExtensionsProvider::perform(const ExtensionQuery& query)
{
    Action a = action(query);
    switch (a.type) {
    case Type::Form: {
        ExtensionQuery q = query;
        if (!q.contains("modal")) {
            q.addParam("modal", Val(a.modal));
        }
        return interactive()->openSync(q).ret;
    } break;
    case Type::Macros:
        return run(query);
    default:
        break;
    }

    return make_ret(Ret::Code::UnknownError);
}

muse::Ret ExtensionsProvider::run(const ExtensionQuery& query)
{
    const Action extensionAction = action(query);
    const Manifest& extensionManifest = manifest(query.uri());
    return run(extensionAction, extensionManifest);
}

std::unique_ptr<IExtensionSession> ExtensionsProvider::newSession(const ExtensionUri& uri, const io::path_t& relativeScriptPath) const
{
    const Manifest& manifest = this->manifest(uri);
    if (!manifest.isValid()) {
        LOGE() << "manifest not found: " << uri;
        return {};
    }
    const auto scriptPath = resolveBundleFile(io::dirpath(manifest.path), relativeScriptPath);
    if (!scriptPath) {
        LOGE() << "extension script not found in bundle: " << relativeScriptPath << ", manifest: " << manifest.path;
        return {};
    }

    return newSession(manifest, *scriptPath);
}

muse::Ret ExtensionsProvider::run(const Action& a, const Manifest& manifest)
{
    if (!a.isValid()) {
        return make_ret(Err::ExtNotFound);
    }

    //! TODO Add check of type

    Ret ret;
    if (a.legacyPlugin) {
        legacy::ExtPluginRunner runner(iocContext());
        ret = runner.run(a);
    } else {
        auto session = newSession(manifest, a.path);
        if (!session) {
            return make_ret(Err::ExtLoadError);
        }
        ret = session->evaluate();
        if (!ret) {
            LOGE() << "failed evaluate extension script: " << a.path << ", err: " << ret.toString();
            return make_ret(Err::ExtLoadError);
        }
        ret = session->call(QString::fromStdString(a.func));
        if (!ret) {
            LOGE() << "failed call extension function: " << a.func << ", script: " << a.path << ", err: " << ret.toString();
            return make_ret(Err::ExtBadFormat);
        }
    }

    return ret;
}

std::unique_ptr<IExtensionSession> ExtensionsProvider::newSession(const Manifest& extensionManifest, const io::path_t& scriptPath) const
{
    return std::make_unique<ExtensionSession>(iocContext(), extensionManifest, scriptPath);
}
