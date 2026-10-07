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
#ifndef MUSE_SHORTCUTS_SHORTCUTSCONTROLLERSTUB_H
#define MUSE_SHORTCUTS_SHORTCUTSCONTROLLERSTUB_H

#include "shortcuts_v2/ishortcutscontroller.h"

namespace muse::shortcuts {
class ShortcutsControllerStub : public IShortcutsController
{
public:

    bool active() override;
    void setActive(bool active) override;
    async::Notification activeChanged() const override;

    void activate(const std::string& sequence) override;
};
}

#endif // MUSE_SHORTCUTS_SHORTCUTSCONTROLLERSTUB_H
