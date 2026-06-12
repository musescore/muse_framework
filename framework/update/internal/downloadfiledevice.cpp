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
#include "downloadfiledevice.h"

using namespace muse;
using namespace muse::update;

DownloadFileDevice::DownloadFileDevice(const io::path_t& path, io::IODevice::OpenMode mode, QObject* parent)
    : QIODevice(parent), m_stream(path), m_mode(mode)
{
}

DownloadFileDevice::~DownloadFileDevice()
{
    DownloadFileDevice::close();
}

bool DownloadFileDevice::isSequential() const
{
    return true;
}

bool DownloadFileDevice::open(QIODevice::OpenMode)
{
    //! NOTE: Ignore the mode requested by the network manager (WriteOnly, which
    //! truncates); open the file in the mode chosen at construction so that an
    //! interrupted download can be resumed via Append.
    if (!m_stream.open(m_mode)) {
        setErrorString(QString::fromStdString(m_stream.errorString()));
        return false;
    }

    QIODevice::open(QIODevice::WriteOnly);
    return true;
}

void DownloadFileDevice::close()
{
    if (m_stream.isOpen()) {
        m_stream.close();
    }
    QIODevice::close();
}

qint64 DownloadFileDevice::readData(char*, qint64)
{
    return -1;
}

qint64 DownloadFileDevice::writeData(const char* data, qint64 len)
{
    const size_t written = m_stream.write(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(len));
    return static_cast<qint64>(written);
}
