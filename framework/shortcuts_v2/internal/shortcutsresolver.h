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

#include <map>
#include <string>

#include "../ishortcutsresolver.h"

namespace muse::shortcuts {
//! NOTE Selects the active shortcut by the priority of its scope
class ShortcutsResolver : public IShortcutsResolver
{
public:
    Shortcut selectActive(const ShortcutList& list) const override;
    TranslatableString scopeTitle(const std::string& scopeCode) const override;

    void setScope(const Scope& scope) override;

private:
    bool isScopeActive(const std::string& code) const;
    int scopePriority(const std::string& code) const;

    const Scope* findScope(const std::string& code) const;

    std::map<std::string, Scope> m_scopes;
};
}
