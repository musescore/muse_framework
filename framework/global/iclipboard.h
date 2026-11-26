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

#include "types/mimedata.h"
#include "types/string.h"

namespace muse {
class IClipboard : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IClipboard)

public:
    virtual ~IClipboard() = default;

    //! The current content, it is a copy
    virtual MimeData mimeData() const = 0;

    //! Replaces the content
    virtual void setMimeData(const MimeData& data) = 0;

    String text() const { return mimeData().text(); }

    void setText(const String& text)
    {
        MimeData data;
        data.setText(text);
        setMimeData(data);
    }
};
}
