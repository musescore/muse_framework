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

#include "modularity/imoduleinterface.h"

#include "global/async/notification.h"
#include "global/async/channel.h"

#include "rcommand/commandtypes.h"

#include "extensionstypes.h"

namespace muse::extensions {
class IExtensionsRegister : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IExtensionsRegister)
public:
    virtual ~IExtensionsRegister() = default;

    virtual void reload() = 0;

    virtual ManifestList manifestList(Filter filter = Filter::All) const = 0;
    virtual async::Notification manifestListChanged() const = 0;

    virtual KnownCategories knownCategories() const = 0;

    virtual bool exists(const ExtensionUri& uri) const = 0;
    virtual const Manifest& manifest(const ExtensionUri& uri) const = 0;

    virtual void setEnabled(const ExtensionUri& uri, bool enabled) = 0;
    virtual bool isEnabled(const ExtensionUri& uri) const = 0;
    virtual async::Channel<ExtensionUri> enabledChanged() const = 0;
};
}
