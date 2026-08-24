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
#include "extensionstoolbarmodel.h"

#include "extensionstypes.h"
#include "uicomponents/qml/Muse/UiComponents/toolbaritem.h"

using namespace muse::extensions;
using namespace muse::uicomponents;

void ExtensionsToolBarModel::load()
{
    extensionsRegister()->manifestListChanged().onNotify(this, [this]() {
        load();
    }, async::Asyncable::Mode::SetReplace);

    extensionsRegister()->enabledChanged().onReceive(this, [this](const Uri&) {
        load();
    }, async::Asyncable::Mode::SetReplace);

    ToolBarItemList items;
    ManifestList manifests = extensionsRegister()->manifestList(Filter::Enabled);
    for (const Manifest& m : manifests) {
        for (const muse::extensions::Action& a : m.actions) {
            if (!a.showOnToolbar) {
                continue;
            }

            ToolBarItem* item = new ToolBarItem(this);
            item->setTitle(!a.title.empty()
                           ? TranslatableString::untranslatable(a.title)
                           : TranslatableString::untranslatable(m.title));

            ui::UiAction uiaction;
            uiaction.code = makeCommandQuery(m.uri, a.code).toString();
            item->setAction(uiaction);

            items << item;
        }
    }

    setItems(items);

    AbstractToolBarModel::load();
}
