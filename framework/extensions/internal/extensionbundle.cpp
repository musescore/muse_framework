/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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
#include "../extensionbundle.h"

#include <QDir>
#include <QFileInfo>

namespace muse::extensions {
std::optional<io::path_t> resolveBundleFile(const io::path_t& bundlePath, const io::path_t& relativePath)
{
    const QFileInfo declared(relativePath.toQString());
    if (relativePath.empty() || declared.isAbsolute()) {
        return std::nullopt;
    }

    const QFileInfo bundle(bundlePath.toQString());
    const QString root = bundle.canonicalFilePath();
    const QFileInfo file(QDir(root).filePath(declared.filePath()));
    const QString path = file.canonicalFilePath();
    if (!bundle.isDir() || root.isEmpty() || path.isEmpty() || !file.isFile() || !path.startsWith(root + QLatin1Char('/'))) {
        return std::nullopt;
    }
    return io::path_t(path);
}
} // namespace muse::extensions
