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

#include <QIODevice>

#include "io/filestream.h"
#include "io/path.h"

namespace muse::update {
//! A write-only QIODevice that streams incoming network data straight to disk
//! via muse::io::FileStream. It opens the underlying file in the mode chosen at
//! construction (Append to resume an interrupted download), ignoring the open
//! mode requested by the network manager (which would otherwise truncate).
class DownloadFileDevice : public QIODevice
{
public:
    DownloadFileDevice(const io::path_t& path, io::IODevice::OpenMode mode, QObject* parent = nullptr);
    ~DownloadFileDevice() override;

    bool isSequential() const override;
    bool open(QIODevice::OpenMode mode) override;
    void close() override;

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    io::FileStream m_stream;
    io::IODevice::OpenMode m_mode = io::IODevice::WriteOnly;
};
}
