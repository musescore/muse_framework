/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) MuseScore Limited and others
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

#include "extensionsregister.h"

#include "extensionsloader.h"
#include "legacy/extpluginsloader.h"

#include "log.h"

using namespace muse;
using namespace muse::extensions;
using namespace muse::rcommand;

void ExtensionsRegister::reload()
{
    ExtensionsLoader loader;
    m_manifests = loader.loadManifestList(configuration()->defaultPath(),
                                          configuration()->userPath());

    legacy::ExtPluginsLoader pluginsLoader;
    ManifestList plugins = pluginsLoader.loadManifestList(configuration()->pluginsDefaultPath(),
                                                          configuration()->pluginsUserPath());

    muse::join(m_manifests, plugins);

    m_configs = configuration()->extensionConfigs();

    m_manifestListChanged.notify();
}

ManifestList ExtensionsRegister::manifestList(Filter filter) const
{
    if (filter == Filter::Enabled) {
        ManifestList list;
        for (const Manifest& m : m_manifests) {
            if (isEnabled(m.uri)) {
                list.push_back(m);
            }
        }
        return list;
    }

    return m_manifests;
}

async::Notification ExtensionsRegister::manifestListChanged() const
{
    return m_manifestListChanged;
}

bool ExtensionsRegister::exists(const ExtensionUri& uri) const
{
    auto it = std::find_if(m_manifests.begin(), m_manifests.end(), [uri](const Manifest& m) {
        return m.uri == uri;
    });

    if (it != m_manifests.end()) {
        return true;
    }

    return false;
}

const Manifest& ExtensionsRegister::manifest(const ExtensionUri& uri) const
{
    auto it = std::find_if(m_manifests.begin(), m_manifests.end(), [uri](const Manifest& m) {
        return m.uri == uri;
    });

    if (it != m_manifests.end()) {
        return *it;
    }

    static Manifest _dummy;
    return _dummy;
}

KnownCategories ExtensionsRegister::knownCategories() const
{
    static KnownCategories categories {
        { "composing-arranging-tools", TranslatableString("extensions", "Composing/arranging tools") },
        { "color-notes", TranslatableString("extensions", "Color notes") },
        { "playback", TranslatableString("extensions", "Playback") },
        { "lyrics", TranslatableString("extensions", "Lyrics") }
    };

    return categories;
}

void ExtensionsRegister::setEnabled(const ExtensionUri& uri, bool enabled)
{
    if (m_configs[uri].enabled == enabled) {
        return;
    }

    m_configs[uri].enabled = enabled;
    configuration()->setExtensionConfigs(m_configs);

    LOGDA() << "enabled changed: " << uri.toString() << " to " << enabled;
    m_enabledChanged.send(uri);
}

bool ExtensionsRegister::isEnabled(const ExtensionUri& uri) const
{
    return muse::value(m_configs, uri).enabled;
}

async::Channel<ExtensionUri> ExtensionsRegister::enabledChanged() const
{
    return m_enabledChanged;
}
