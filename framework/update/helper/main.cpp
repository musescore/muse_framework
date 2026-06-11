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

//! NOTE: Standalone, zero-runtime-dependency update helper.
//! It is copied out of the application install location and run from there so
//! that replacing the install location never touches a file it is executing.
//! It waits for the host application to exit, atomically swaps the install
//! location with the freshly unpacked update (keeping a backup for rollback),
//! relaunches the application and removes itself.

#include "platform.h"

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <cstdio>

namespace fs = std::filesystem;

namespace {
struct Args {
    long long waitPid = 0;
    std::string src;      // freshly unpacked new install (dir or bundle)
    std::string dst;      // current install location to be replaced
    std::string relaunch; // path to launch after a successful swap
    std::string logPath;
};

FILE* g_log = nullptr;

void logLine(const std::string& msg)
{
    if (g_log) {
        std::fprintf(g_log, "%s\n", msg.c_str());
        std::fflush(g_log);
    }
}

Args parseArgs(int argc, char** argv)
{
    std::map<std::string, std::string> kv;
    for (int i = 1; i + 1 < argc; i += 2) {
        kv[argv[i]] = argv[i + 1];
    }

    Args a;
    if (kv.count("--wait-pid")) {
        a.waitPid = std::stoll(kv["--wait-pid"]);
    }
    a.src = kv.count("--src") ? kv["--src"] : std::string();
    a.dst = kv.count("--dst") ? kv["--dst"] : std::string();
    a.relaunch = kv.count("--relaunch") ? kv["--relaunch"] : std::string();
    a.logPath = kv.count("--log") ? kv["--log"] : std::string();
    return a;
}

//! Move `from` to `to`, falling back to copy+remove when rename crosses a
//! filesystem boundary (rename only succeeds within a single volume).
bool movePath(const fs::path& from, const fs::path& to, std::error_code& ec)
{
    fs::rename(from, to, ec);
    if (!ec) {
        return true;
    }

    ec.clear();
    fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    if (ec) {
        return false;
    }

    std::error_code rmEc;
    fs::remove_all(from, rmEc);
    return true;
}
}

int main(int argc, char** argv)
{
    Args args = parseArgs(argc, argv);

    if (!args.logPath.empty()) {
        g_log = std::fopen(args.logPath.c_str(), "w");
    }

    logLine("museupdater started");
    logLine("  src=" + args.src);
    logLine("  dst=" + args.dst);
    logLine("  relaunch=" + args.relaunch);
    logLine("  wait-pid=" + std::to_string(args.waitPid));

    if (args.src.empty() || args.dst.empty()) {
        logLine("error: --src and --dst are required");
        return 1;
    }

    // 1. Wait for the host application to fully exit before touching its files.
    if (args.waitPid > 0) {
        platform::waitForProcessExit(args.waitPid, /*timeoutMs*/ 60000);
    }

    const fs::path src(args.src);
    const fs::path dst(args.dst);
    const fs::path backup = fs::path(dst).concat(".bak");

    std::error_code ec;

    if (!fs::exists(src, ec)) {
        logLine("error: src does not exist");
        return 1;
    }

    // 2. Backup the current install location.
    fs::remove_all(backup, ec);
    ec.clear();
    if (fs::exists(dst, ec)) {
        if (!movePath(dst, backup, ec)) {
            logLine("error: failed to backup dst: " + ec.message());
            return 1;
        }
    }

    // 3. Swap in the new install.
    if (!movePath(src, dst, ec)) {
        logLine("error: failed to move src into place: " + ec.message());

        // Rollback.
        std::error_code rbEc;
        if (fs::exists(backup, rbEc)) {
            movePath(backup, dst, rbEc);
        }
        return 1;
    }

    // 4. Verify the result; rollback on failure (platform-specific check, e.g.
    //    code signature validity on macOS).
    if (!platform::verifyInstall(dst.string())) {
        logLine("error: post-swap verification failed, rolling back");

        std::error_code rbEc;
        fs::remove_all(dst, rbEc);
        if (fs::exists(backup, rbEc)) {
            movePath(backup, dst, rbEc);
        }
        return 1;
    }

    // 5. Success: drop the backup.
    fs::remove_all(backup, ec);

    // 6. Relaunch the updated application.
    const std::string relaunchTarget = args.relaunch.empty() ? dst.string() : args.relaunch;
    if (!platform::relaunch(relaunchTarget)) {
        logLine("error: failed to relaunch " + relaunchTarget);
        // The update itself succeeded; the user can start the app manually.
    }

    logLine("museupdater finished");

    if (g_log) {
        std::fclose(g_log);
        g_log = nullptr;
    }

    return 0;
}
