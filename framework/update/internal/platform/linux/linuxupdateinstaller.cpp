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
#include "linuxupdateinstaller.h"

#include <cstring>
#include <unistd.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include "../../../updateerrors.h"

#include "log.h"

using namespace muse;
using namespace muse::update;

static const QString HELPER_NAME("museupdater");

namespace {
//! An AppImage is an ELF executable with a marker in the bytes the ELF header
//! reserves for the OS ABI: "AI" followed by the AppImage format version.
//! Only type 2 is produced nowadays, and it is what we publish.
constexpr int APPIMAGE_HEADER_SIZE = 11;
constexpr char APPIMAGE_TYPE_2 = 0x02;

bool isAppImageFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        LOGE() << "failed to open for reading: " << path;
        return false;
    }

    char header[APPIMAGE_HEADER_SIZE] = { 0 };
    if (file.read(header, APPIMAGE_HEADER_SIZE) != APPIMAGE_HEADER_SIZE) {
        LOGE() << "file is too small to be an AppImage: " << path;
        return false;
    }

    static const char ELF_MAGIC[] = { 0x7f, 'E', 'L', 'F' };
    if (std::memcmp(header, ELF_MAGIC, sizeof(ELF_MAGIC)) != 0) {
        LOGE() << "not an ELF file: " << path;
        return false;
    }

    if (header[8] != 'A' || header[9] != 'I' || header[10] != APPIMAGE_TYPE_2) {
        LOGE() << "not a type 2 AppImage: " << path;
        return false;
    }

    return true;
}

bool isWritable(const QString& path)
{
    return ::access(path.toUtf8().constData(), W_OK) == 0;
}
}

io::path_t LinuxUpdateInstaller::currentAppImagePath() const
{
    //! NOTE: Set by the AppImage runtime. Absent for every other way of running
    //! the application, none of which can be updated by replacing one file.
    const QByteArray appImage = qgetenv("APPIMAGE");
    if (appImage.isEmpty()) {
        return {};
    }

    const QFileInfo info(QFile::decodeName(appImage));
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    return io::path_t(info.canonicalFilePath());
}

io::path_t LinuxUpdateInstaller::helperPath() const
{
    return io::path_t(QCoreApplication::applicationDirPath() + "/" + HELPER_NAME);
}

bool LinuxUpdateInstaller::isInPlaceUpdateSupported() const
{
    const QString appImage = currentAppImagePath().toQString();
    if (appImage.isEmpty()) {
        return false;
    }

    if (!QFileInfo::exists(helperPath().toQString())) {
        return false;
    }

    //! NOTE: Replacing the AppImage creates a new directory entry, so the
    //! containing directory has to be writable too - a writable file inside a
    //! read-only directory (a system-wide install) is not enough.
    if (!isWritable(appImage) || !isWritable(QFileInfo(appImage).absolutePath())) {
        return false;
    }

    return true;
}

Ret LinuxUpdateInstaller::applyUpdate(const muse::io::path_t& packagePath)
{
    const QString package = packagePath.toQString();
    if (!QFileInfo::exists(package)) {
        LOGE() << "update package does not exist: " << package;
        return make_ret(Err::UnknownError);
    }

    const QString appImagePath = currentAppImagePath().toQString();
    if (appImagePath.isEmpty()) {
        LOGE() << "not running from an AppImage, cannot update in place";
        return make_ret(Ret::Code::NotSupported);
    }

    // 1. The downloaded package replaces the running file as-is, so make sure it
    //    really is an AppImage before letting the helper move it into place.
    if (!isAppImageFile(package)) {
        LOGE() << "update package is not a valid AppImage: " << package;
        return make_ret(Err::UnknownError);
    }

    // 2. The download has no executable bit; the swapped-in file must be
    //    runnable.
    const QFile::Permissions permissions
        = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
          | QFile::ReadGroup | QFile::ExeGroup
          | QFile::ReadOther | QFile::ExeOther;

    if (!QFile::setPermissions(package, permissions)) {
        LOGE() << "failed to make update package executable: " << package;
        return make_ret(Err::UnknownError);
    }

    // 3. Copy the helper out of the AppImage. Its mount point disappears as soon
    //    as this process exits, which is exactly when the helper starts working.
    fileSystem()->makePath(configuration()->updateDataPath());

    const QString helperRun = configuration()->updateDataPath().toQString() + "/" + HELPER_NAME;
    QFile::remove(helperRun);
    if (!QFile::copy(helperPath().toQString(), helperRun)) {
        LOGE() << "failed to copy helper to " << helperRun;
        return make_ret(Err::UnknownError);
    }
    QFile::setPermissions(helperRun, permissions);

    // 4. Spawn the detached helper. It waits for us to quit, replaces the
    //    AppImage and relaunches it.
    const QString logPath = configuration()->updateDataPath().toQString() + "/museupdater.log";
    const QStringList args = {
        "--wait-pid", QString::number(QCoreApplication::applicationPid()),
        "--src", package,
        "--dst", appImagePath,
        "--relaunch", appImagePath,
        "--log", logPath
    };

    if (!QProcess::startDetached(helperRun, args)) {
        LOGE() << "failed to start update helper";
        return make_ret(Err::UnknownError);
    }

    LOGI() << "update helper started, will replace " << appImagePath << " after quit";
    return make_ok();
}
