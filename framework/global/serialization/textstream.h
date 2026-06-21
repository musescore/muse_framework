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
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#ifndef NO_QT_SUPPORT
#include <QString>
#endif

namespace muse {
class ByteArray;
class String;

namespace io {
class IODevice;
}

template<typename T>
concept NonCharInteger
    =std::integral<T>
      && !std::same_as<std::remove_cv_t<T>, char>
      && !std::same_as<std::remove_cv_t<T>, signed char>
      && !std::same_as<std::remove_cv_t<T>, unsigned char>
      && !std::same_as<std::remove_cv_t<T>, wchar_t>
      && !std::same_as<std::remove_cv_t<T>, char8_t>
      && !std::same_as<std::remove_cv_t<T>, char16_t>
      && !std::same_as<std::remove_cv_t<T>, char32_t>;

class TextStream
{
public:
    TextStream() = default;
    explicit TextStream(io::IODevice* device);
    virtual ~TextStream();

    void setDevice(io::IODevice* device);

    void flush();

    TextStream& operator<<(char ch);
    TextStream& operator<<(signed char ch) { return *this << static_cast<char>(ch); }
    TextStream& operator<<(unsigned char ch) { return *this << static_cast<char>(ch); }

    TextStream& operator<<(NonCharInteger auto val)
    {
        if constexpr (sizeof(val) <= 4) {
            if constexpr (std::is_signed_v<decltype(val)>) {
                return write_int32_t(static_cast<int32_t>(val));
            } else {
                return write_uint32_t(static_cast<uint32_t>(val));
            }
        } else {
            if constexpr (std::is_signed_v<decltype(val)>) {
                return write_int64_t(static_cast<int64_t>(val));
            } else {
                return write_uint64_t(static_cast<uint64_t>(val));
            }
        }
    }

    TextStream& operator<<(double);

    TextStream& operator<<(const char* s);
    TextStream& operator<<(std::string_view);
    TextStream& operator<<(const ByteArray& b);
    TextStream& operator<<(const String& s);

#ifndef NO_QT_SUPPORT
    TextStream& operator<<(const QString& s);
#endif

private:
    void write(const char* ch, size_t len);

    TextStream& write_int32_t(int32_t val);
    TextStream& write_uint32_t(uint32_t val);
    TextStream& write_int64_t(int64_t val);
    TextStream& write_uint64_t(uint64_t val);

    template<size_t MaxDigits>
    TextStream& writeInt(NonCharInteger auto val);

    io::IODevice* m_device = nullptr;
    std::vector<uint8_t> m_buf;
};
}
