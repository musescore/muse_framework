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
#include <memory>
#include <string>
#include <vector>

#include "draw/types/pixmap.h"
#include "io/path.h"
#include "types/bytearray.h"
#include "types/string.h"

namespace muse {
//! NOTE A set of data in different mime formats, like QMimeData
class MimeData
{
public:
    static constexpr const char* TEXT_FORMAT = "text/plain";

    std::vector<std::string> formats() const
    {
        std::vector<std::string> ret;
        for (const auto& [format, _] : m_data) {
            ret.push_back(format);
        }
        return ret;
    }

    bool hasFormat(const std::string& format) const { return m_data.find(format) != m_data.end(); }

    ByteArray data(const std::string& format) const
    {
        auto it = m_data.find(format);
        return it != m_data.end() ? it->second : ByteArray();
    }

    void setData(const std::string& format, const ByteArray& data) { m_data[format] = data; }

    //! NOTE The plain text, it is the data of the text format
    bool hasText() const { return hasFormat(TEXT_FORMAT); }
    String text() const { return String::fromUtf8(data(TEXT_FORMAT)); }
    void setText(const String& text) { setData(TEXT_FORMAT, text.toUtf8()); }

    bool hasImage() const { return m_image != nullptr; }
    std::shared_ptr<draw::Pixmap> imageData() const { return m_image; }
    void setImageData(const std::shared_ptr<draw::Pixmap>& image) { m_image = image; }

    const std::vector<io::path_t>& localFiles() const { return m_localFiles; }
    void setLocalFiles(const std::vector<io::path_t>& files) { m_localFiles = files; }

    bool isEmpty() const { return m_data.empty() && !m_image && m_localFiles.empty(); }

private:
    std::map<std::string, ByteArray> m_data;
    std::shared_ptr<draw::Pixmap> m_image;
    std::vector<io::path_t> m_localFiles;
};
}
