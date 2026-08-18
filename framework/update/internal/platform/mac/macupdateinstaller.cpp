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
#include "macupdateinstaller.h"

#include <unistd.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QUuid>

#include "../../../updateerrors.h"

#include "log.h"

using namespace muse;
using namespace muse::update;

static const QString HELPER_NAME("museupdater");

io::path_t MacUpdateInstaller::currentBundlePath() const
{
    // applicationDirPath() == <bundle>.app/Contents/MacOS
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();   // Contents
    dir.cdUp();   // <bundle>.app
    return io::path_t(dir.absolutePath());
}

io::path_t MacUpdateInstaller::helperPath() const
{
    return io::path_t(QCoreApplication::applicationDirPath() + "/" + HELPER_NAME);
}

bool MacUpdateInstaller::isInPlaceUpdateSupported() const
{
    const QString bundlePath = currentBundlePath().toQString();
    if (bundlePath.isEmpty() || !bundlePath.endsWith(".app")) {
        return false;
    }

    if (!QFileInfo::exists(helperPath().toQString())) {
        return false;
    }

    // In-place replacement only works if we can write the bundle without
    // privilege escalation.
    if (::access(bundlePath.toUtf8().constData(), W_OK) != 0
        && ::access(io::dirpath(bundlePath.toStdString()).toStdString().c_str(), W_OK) != 0) {
        return false;
    }

    return true;
}

Ret MacUpdateInstaller::applyUpdate(const muse::io::path_t& packagePath, const InstallProgressUi&)
{
    const QString package = packagePath.toQString();
    if (!QFileInfo::exists(package)) {
        LOGE() << "update package does not exist: " << package;
        return make_ret(Err::UnknownError);
    }

    const QString stagingDir = configuration()->updateDataPath().toQString() + "/staging";
    QDir().rmpath(stagingDir);
    QDir staging(stagingDir);
    if (staging.exists()) {
        staging.removeRecursively();
    }
    QDir().mkpath(stagingDir);

    // 1. Unpack the dmg preserving extended attributes and code signatures.
    Ret unpackRet = unpackDmg(package, stagingDir);
    if (!unpackRet) {
        return unpackRet;
    }

    // 2. Locate the unpacked .app bundle.
    QString stagingApp;
    const QStringList apps = staging.entryList({ "*.app" }, QDir::Dirs | QDir::NoDotAndDotDot);
    if (!apps.isEmpty()) {
        stagingApp = stagingDir + "/" + apps.first();
    }
    if (stagingApp.isEmpty()) {
        LOGE() << "no .app bundle found in unpacked update";
        return make_ret(Err::UnknownError);
    }

    // 3. Remove the quarantine attribute set by the download.
    QProcess::execute("/usr/bin/xattr", { "-dr", "com.apple.quarantine", stagingApp });

    // 4. Verify the unpacked bundle is correctly signed before trusting it.
    int rc = QProcess::execute("/usr/bin/codesign", { "--verify", "--deep", "--strict", stagingApp });
    if (rc != 0) {
        LOGE() << "code signature verification failed for unpacked update, rc=" << rc;
        return make_ret(Err::UnknownError);
    }

    // 5. Copy the helper out of the bundle so replacing the bundle never
    //    touches the running helper file.
    const QString helperRun = configuration()->updateDataPath().toQString() + "/" + HELPER_NAME;
    QFile::remove(helperRun);
    if (!QFile::copy(helperPath().toQString(), helperRun)) {
        LOGE() << "failed to copy helper to " << helperRun;
        return make_ret(Err::UnknownError);
    }
    QFile::setPermissions(helperRun, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                          | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther);

    // 6. Spawn the detached helper. It waits for us to quit, swaps the bundle
    //    and relaunches.
    const QString bundlePath = currentBundlePath().toQString();
    const QString logPath = configuration()->updateDataPath().toQString() + "/museupdater.log";
    const QStringList args = {
        "--wait-pid", QString::number(QCoreApplication::applicationPid()),
        "--src", stagingApp,
        "--dst", bundlePath,
        "--relaunch", bundlePath,
        "--log", logPath
    };

    if (!QProcess::startDetached(helperRun, args)) {
        LOGE() << "failed to start update helper";
        return make_ret(Err::UnknownError);
    }

    LOGI() << "update helper started, will replace " << bundlePath << " after quit";
    return make_ok();
}

Ret MacUpdateInstaller::unpackDmg(const QString& package, const QString& stagingDir) const
{
    QString mountPoint;
    do {
        mountPoint = "/Volumes/" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    } while (QFileInfo::exists(mountPoint));

    QProcess attach;
    attach.setProgram("/usr/bin/hdiutil");
    attach.setArguments({ "attach", package, "-mountpoint", mountPoint,
                          "-noverify", "-nobrowse", "-noautoopen", "-stdinpass" });
    attach.start();
    if (!attach.waitForStarted()) {
        LOGE() << "failed to start hdiutil";
        return make_ret(Err::UnknownError);
    }

    // Empty null-terminated passphrase for -stdinpass, then a "yes" answer in
    // case the image carries a license agreement, so hdiutil never blocks on
    // an interactive prompt.
    attach.write(QByteArray("\0yes\n", 5));
    attach.closeWriteChannel();
    attach.waitForFinished(-1);
    if (attach.exitStatus() != QProcess::NormalExit || attach.exitCode() != 0) {
        LOGE() << "failed to mount update dmg, hdiutil rc=" << attach.exitCode();
        return make_ret(Err::UnknownError);
    }

    Ret ret = make_ok();

    const QStringList apps = QDir(mountPoint).entryList({ "*.app" }, QDir::Dirs | QDir::NoDotAndDotDot);
    if (apps.isEmpty()) {
        LOGE() << "no .app bundle found in mounted dmg";
        ret = make_ret(Err::UnknownError);
    } else {
        int rc = QProcess::execute("/usr/bin/ditto", { mountPoint + "/" + apps.first(), stagingDir + "/" + apps.first() });
        if (rc != 0) {
            LOGE() << "failed to copy app out of dmg, ditto rc=" << rc;
            ret = make_ret(Err::UnknownError);
        }
    }

    QProcess::execute("/usr/bin/hdiutil", { "detach", mountPoint, "-force" });

    return ret;
}
