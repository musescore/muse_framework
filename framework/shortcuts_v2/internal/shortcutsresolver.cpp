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

#include "shortcutsresolver.h"

#include <algorithm>

#include "global/stringutils.h"

#include "log.h"

using namespace muse;
using namespace muse::shortcuts;

void ShortcutsResolver::setScope(const Scope& scope)
{
    m_scopes[strings::toUpper(scope.code)] = scope;
}

const IShortcutsResolver::Scope* ShortcutsResolver::findScope(const std::string& code) const
{
    auto it = m_scopes.find(strings::toUpper(code));
    return it != m_scopes.end() ? &it->second : nullptr;
}

bool ShortcutsResolver::isScopeActive(const std::string& code) const
{
    const Scope* scope = findScope(code);
    return !scope || !scope->isActive || scope->isActive();
}

int ShortcutsResolver::scopePriority(const std::string& code) const
{
    const Scope* scope = findScope(code);
    return scope ? scope->priority : 0;
}

Shortcut ShortcutsResolver::selectActive(const ShortcutList& list) const
{
    IF_ASSERT_FAILED(!list.empty()) {
        return Shortcut();
    }

    //! NOTE An active scope first, then higher priority, then a real command before the none command.
    //! The shortcuts with the same key keep their order.
    ShortcutList sorted = list;
    std::stable_sort(sorted.begin(), sorted.end(), [this](const Shortcut& a, const Shortcut& b) {
        const bool aIsActive = isScopeActive(a.scope);
        const bool bIsActive = isScopeActive(b.scope);
        if (aIsActive != bIsActive) {
            return aIsActive;
        }

        const int aPriority = scopePriority(a.scope);
        const int bPriority = scopePriority(b.scope);
        if (aPriority != bPriority) {
            return aPriority > bPriority;
        }

        const bool aIsNone = a.command == rcommand::NONE_COMMAND;
        const bool bIsNone = b.command == rcommand::NONE_COMMAND;
        return !aIsNone && bIsNone;
    });

    const Shortcut& probableShortcut = sorted.front();
    if (!isScopeActive(probableShortcut.scope)) {
        //! NOTE If the probable shortcut is not active, return an invalid shortcut.
        return Shortcut();
    }

    return probableShortcut;
}

TranslatableString ShortcutsResolver::scopeTitle(const std::string& scopeCode) const
{
    const std::string upperScopeCode = strings::toUpper(scopeCode);

    const Scope* scope = findScope(scopeCode);
    if (scope && !scope->title.isEmpty()) {
        return scope->title;
    }

    LOGW() << "Unknown scope: " << scopeCode;
    return TranslatableString::untranslatable(String::fromStdString(upperScopeCode));
}
