/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
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

#include <string>
#include <QList>

#include "rcommand/commandtypes.h"

namespace muse::ui {
struct ToolConfig
{
    static inline const std::string ____________;  // separator - empty string

    struct Item
    {
        std::string intent;
        bool show = true;

        Item() = default;

        Item(const std::string& i, bool sh)
            : intent(i), show(sh) {}

        Item(const rcommand::Command& c, bool sh)
            : intent(c.toString()), show(sh) {}

        bool isSeparator() const { return intent == ____________; }

        bool operator ==(const Item& other) const
        {
            return intent == other.intent && show == other.show;
        }
    };

    QList<Item> items;

    bool isValid() const { return !items.empty(); }
};
}
