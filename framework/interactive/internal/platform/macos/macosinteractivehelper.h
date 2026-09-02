/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "io/path.h"
#include "types/ret.h"

#include "async/promise.h"

namespace muse {
class UriQuery;

class MacOSInteractiveHelper
{
public:
    enum class EditAction {
        Undo,
        Redo,
        Cut,
        Copy,
        Paste,
        SelectAll
    };

    static void setEditMenuIndex(int menuIndex);
    static void setEditMenuStructure(const std::map<EditAction, int>& structure);

    static bool revealInFinder(const io::path_t& filePath);

    static Ret isAppExists(const std::string& appIdentifier);
    static Ret canOpenApp(const UriQuery& uri);
    static async::Promise<Ret> openApp(const UriQuery& uri);

    class NativeDialogScope
    {
    public:
        NativeDialogScope();
        ~NativeDialogScope();

        NativeDialogScope(const NativeDialogScope&) = delete;
        NativeDialogScope& operator=(const NativeDialogScope&) = delete;
        NativeDialogScope(NativeDialogScope&&) = delete;
        NativeDialogScope& operator=(NativeDialogScope&&) = delete;
    };
};
}
