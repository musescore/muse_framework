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
#include "extensionsactioncontroller.h"

#include "translation.h"
#include "types/ret.h"

#include "../extensionscommands.h"

#include "log.h"

using namespace muse::extensions;

static const muse::UriQuery SHOW_APIDUMP_URI("muse://extensions/apidump?modal=false&floating=true");

void ExtensionsActionController::init()
{
    provider()->manifestListChanged().onNotify(this, [this](){
        registerExtensions();
    });

    registerExtensions();
}

void ExtensionsActionController::registerExtensions()
{
    commandDispatcher()->unreg(this);

    for (const Manifest& m : provider()->manifestList()) {
        for (const Action& a : m.actions) {
            ExtensionUri uri = m.uri;
            ExtensionActionCode actionCode = a.code;
            commandDispatcher()->onRequest(this, makeCommand(uri, actionCode), [this, uri, actionCode]() {
                return onExtensionTriggered(uri, actionCode);
            });
        }
    }

    commandDispatcher()->onRequest(this, OPEN_APIDUMP_COMMAND, [this]() { openUri(SHOW_APIDUMP_URI); return muse::make_ok(); });
}

muse::Ret ExtensionsActionController::onExtensionTriggered(const ExtensionUri& uri, const ExtensionActionCode& actionCode)
{
    if (provider()->isEnabled(uri)) {
        return provider()->perform(uri, actionCode);
    }

    const Manifest& m = provider()->manifest(uri);
    IF_ASSERT_FAILED(m.isValid()) {
        LOGE() << "Not found extension, uri: " << uri.toString();
        return muse::make_ret(Ret::Code::BadArgs);
    }

    auto promise = interactive()->warning(
        muse::qtrc("extensions", "The plugin “%1” is currently disabled. Do you want to enable it now?").arg(m.title).toStdString(),
        muse::trc("extensions", "Alternatively, you can enable it at any time from Home > Plugins."),
        { IInteractive::Button::No, IInteractive::Button::Yes });

    promise.onResolve(this, [this, uri, actionCode](const IInteractive::Result& res) {
        if (res.isButton(IInteractive::Button::Yes)) {
            provider()->setEnabled(uri, true);
            provider()->perform(uri, actionCode);
        }
    });

    return muse::make_ok();
}

void ExtensionsActionController::openUri(const UriQuery& uri, bool isSingle)
{
    if (isSingle && interactive()->isOpened(uri.uri()).val) {
        return;
    }

    interactive()->open(uri);
}
