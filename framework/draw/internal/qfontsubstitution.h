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

#include <QFont>
#include <QStringList>

#include "ifontsdatabase.h"
#include "../types/font.h"

namespace muse::draw {
inline QFont toQFont(const Font& font, const IFontsDatabase& database)
{
    QFont qf = font.toQFont();

    const std::vector<FontDataKey> substitutes = database.substitutionFonts(FontDataKey(font.family()));
    if (substitutes.empty()) {
        return qf;
    }

    QStringList families;
    families.reserve(static_cast<qsizetype>(substitutes.size()) + 1);
    families << font.family().id().toQString();
    for (const FontDataKey& key : substitutes) {
        families << key.family().id().toQString();
    }

    qf.setFamilies(families);
    return qf;
}
}
