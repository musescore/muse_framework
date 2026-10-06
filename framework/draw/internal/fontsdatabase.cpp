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
#include "fontsdatabase.h"

#include "muse_framework_config.h"

#ifdef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
#include <QFontDatabase>
#include <QFont>
#endif

#include "global/io/file.h"

#include "log.h"

using namespace muse;
using namespace muse::draw;

#ifndef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
static int s_fontID = -1;
#endif

void FontsDatabase::setDefaultFont(Font::Type type, const FontDataKey& key)
{
    m_defaults[type] = key;
    m_changed.notify();
}

void FontsDatabase::insertSubstitution(const String& familyName, const String& substituteName)
{
    FontDataKey familyKey(familyName);
    FontDataKey substituteKey(substituteName);
    m_familySubstitutions[familyKey].push_back(substituteKey);

#ifdef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
    QFont::insertSubstitution(familyName, substituteName);
#endif

    m_changed.notify();
}

void FontsDatabase::removeSubstitutions(const String& familyName, const std::vector<String>& substituteNames)
{
    auto it = m_familySubstitutions.find(FontDataKey(familyName));
    if (it == m_familySubstitutions.end()) {
        return;
    }

    std::vector<FontDataKey>& substitutes = it->second;
    size_t removed = 0;
    for (const String& substituteName : substituteNames) {
        removed += std::erase(substitutes, FontDataKey(substituteName));
    }

    if (removed == 0) {
        return;
    }

#ifdef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
    QFont::removeSubstitutions(familyName);
    for (const FontDataKey& key : substitutes) {
        QFont::insertSubstitution(familyName, key.family().id().toQString());
    }
#endif

    if (substitutes.empty()) {
        m_familySubstitutions.erase(it);
    }

    m_changed.notify();
}

const FontDataKey& FontsDatabase::defaultFont(Font::Type type) const
{
    auto it = m_defaults.find(type);
    if (it != m_defaults.end()) {
        return it->second;
    }

    it = m_defaults.find(Font::Type::Unknown);
    IF_ASSERT_FAILED(it != m_defaults.end()) {
        static FontDataKey null;
        return null;
    }
    return it->second;
}

int FontsDatabase::addFont(const FontDataKey& key, const io::path_t& path)
{
#ifdef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
    const int id = QFontDatabase::addApplicationFont(path.toQString());
    if (id < 0) {
        LOGW() << "failed register font file: " << path;
        return id;
    }
#else
    const int id = ++s_fontID;
#endif

    FileSource file;
    file.path = path;

    FontInfo info;
    info.id = id;
    info.key = key;
    info.source = std::move(file);
    insert(std::move(info));

    return id;
}

int FontsDatabase::addFontFromData(const FontDataKey& key, const ByteArray& data)
{
    if (data.empty()) {
        LOGW() << "empty font data: " << key.family().id();
        return -1;
    }

#ifdef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
    const int id = QFontDatabase::addApplicationFontFromData(data.toQByteArray());
    if (id < 0) {
        LOGW() << "failed register font data: " << key.family().id();
        return id;
    }
#else
    const int id = ++s_fontID;
#endif

    MemorySource memory;
    memory.data = data;

    FontInfo info;
    info.id = id;
    info.key = key;
    info.source = std::move(memory);
    insert(std::move(info));

    return id;
}

void FontsDatabase::insert(FontInfo info)
{
    auto it = m_fonts.find(info.key);
    if (it != m_fonts.end()) {
        const FontInfo replaced = it->second;
        m_fonts.erase(it);
        release(replaced);
    }

    m_fonts.insert({ info.key, std::move(info) });
    m_changed.notify();
}

void FontsDatabase::removeFont(const FontDataKey& key)
{
    auto it = m_fonts.find(key);
    if (it == m_fonts.end()) {
        return;
    }

    const FontInfo removed = it->second;
    m_fonts.erase(it);
    release(removed);
    m_changed.notify();
}

void FontsDatabase::release(const FontInfo& fi)
{
#ifdef MUSE_MODULE_DRAW_USE_QTFONTMETRICS
    if (fi.valid()) {
        QFontDatabase::removeApplicationFont(fi.id);
    }
#else
    UNUSED(fi);
#endif
}

FontDataKey FontsDatabase::actualFont(const FontDataKey& requireKey, Font::Type type) const
{
    const FontInfo& info = fontInfo(requireKey);
    if (info.valid()) {
        if (std::holds_alternative<MemorySource>(info.source)) {
            return requireKey;
        }

        const FileSource& file = std::get<FileSource>(info.source);
        if (!file.path.empty() && io::File::exists(file.path)) {
            return requireKey;
        }
    }

    FontDataKey def = defaultFont(type);
    LOGW() << "not found required font: " << requireKey.family().id() << ", will be using default: " << def.family().id();
    return def;
}

std::vector<FontDataKey> FontsDatabase::substitutionFonts(const FontDataKey& requireKey) const
{
    auto familyIt = m_familySubstitutions.find(FontDataKey(requireKey.family()));
    if (familyIt != m_familySubstitutions.end()) {
        return familyIt->second;
    }

    static std::vector<FontDataKey> null;
    return null;
}

FontData FontsDatabase::fontData(const FontDataKey& requireKey, Font::Type type) const
{
    FontDataKey key = actualFont(requireKey, type);
    const FontInfo& info = fontInfo(key);

    if (const MemorySource* memory = std::get_if<MemorySource>(&info.source)) {
        return FontData { key, memory->data };
    }

    const FileSource& file = std::get<FileSource>(info.source);
    if (file.loaded.empty()) {
        IF_ASSERT_FAILED(io::File::exists(file.path)) {
            return FontData();
        }

        if (!io::File::readFile(file.path, file.loaded)) {
            LOGE() << "failed read font file: " << file.path;
            return FontData();
        }
    }

    return FontData { key, file.loaded };
}

async::Notification FontsDatabase::changed() const
{
    return m_changed;
}

const FontsDatabase::FontInfo& FontsDatabase::fontInfo(const FontDataKey& key) const
{
    auto it = m_fonts.find(key);
    if (it != m_fonts.end()) {
        return it->second;
    }

    static FontInfo null;
    return null;
}
