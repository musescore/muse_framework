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

#pragma once

#include "../iextensionsregister.h"

#include "modularity/ioc.h"
#include "../iextensionsconfiguration.h"

namespace muse::extensions {
class ExtensionsRegister : public IExtensionsRegister
{
    GlobalInject<IExtensionsConfiguration> configuration;
public:

    void reload() override;

    ManifestList manifestList(Filter filter = Filter::All) const override;
    async::Notification manifestListChanged() const override;

    KnownCategories knownCategories() const override;

    bool exists(const ExtensionUri& uri) const override;
    const Manifest& manifest(const ExtensionUri& uri) const override;

    void setEnabled(const ExtensionUri& uri, bool enabled) override;
    bool isEnabled(const ExtensionUri& uri) const override;
    async::Channel<ExtensionUri> enabledChanged() const override;

private:

    ManifestList m_manifests;
    async::Notification m_manifestListChanged;

    std::map<ExtensionUri, ExtensionConfig> m_configs;
    async::Channel<ExtensionUri> m_enabledChanged;
};
}
