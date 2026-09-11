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
#ifndef MUSE_UPDATE_UPDATETYPES_H
#define MUSE_UPDATE_UPDATETYPES_H

#include <string>

#include "types/val.h"

namespace muse::update {
struct PrevReleaseNotes {
    std::string version;
    std::string notes;

    PrevReleaseNotes() = default;
    PrevReleaseNotes(const std::string& version, const std::string& notes)
        : version(version), notes(notes) {}

    bool operator ==(const PrevReleaseNotes& other) const
    {
        return version == other.version && notes == other.notes;
    }
};
using PrevReleasesNotesList = std::vector<PrevReleaseNotes>;

struct InstallProgressUi {
    std::string title;              //!< window caption, e.g. "MuseScore Studio"
    std::string message;            //!< e.g. "Installing MuseScore Studio 4.6.1"
    std::string backgroundColor;    //!< "#RRGGBB"
    std::string accentColor;        //!< "#RRGGBB"
    std::string textColor;          //!< "#RRGGBB"
};

struct ReleaseInfo {
    std::string version;
    std::string fileName;
    std::string fileUrl;
    uint64_t fileSize = 0;

    std::string imageUrl;           // it can be base64 data, like "data:image/png;base64,iVBORw0KGgoA......"
    std::string notes;
    PrevReleasesNotesList previousReleasesNotes;

    ValMap additionInfo;

    std::string actionTitle;        // title of action button
    std::string cancelTitle;        // title of cancel button
    ValList actions;                // open app or web page url, try in order

    bool isValid() const
    {
        return !version.empty();
    }
};

static inline ValList releasesNotesToValList(const PrevReleasesNotesList& list)
{
    ValList valList;
    for (const PrevReleaseNotes& release : list) {
        valList.emplace_back(Val(ValMap {
                { "version", Val(release.version) },
                { "notes", Val(release.notes) }
            }));
    }

    return valList;
}
}

#endif // MUSE_UPDATE_UPDATETYPES_H
