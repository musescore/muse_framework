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

#include "swap.h"

#include "platform.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>

#ifdef __linux__
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE (1 << 1)
#endif
#endif

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
//! Only safe while the install location is untouched: the copy is not atomic
//! and a crash leaves a partial `to` behind.
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

//! Atomically exchange `a` and `b` in a single syscall, so that there is no
//! moment when the install location is empty or half-populated. Fails when the
//! filesystem does not support it; the caller then falls back to two renames.
bool exchangePaths(const fs::path& a, const fs::path& b)
{
#if defined(__APPLE__)
    return ::renamex_np(a.c_str(), b.c_str(), RENAME_SWAP) == 0;
#elif defined(SYS_renameat2)
    return ::syscall(SYS_renameat2, AT_FDCWD, a.c_str(), AT_FDCWD, b.c_str(), RENAME_EXCHANGE) == 0;
#else
    (void)a;
    (void)b;
    errno = ENOTSUP;
    return false;
#endif
}
}

int swapper::run(int argc, char** argv)
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
        if (!platform::waitForProcessExit(args.waitPid, /*timeoutMs*/ 60000)) {
            logLine("error: host process is still running");
            return 1;
        }
    }

    const fs::path src(args.src);
    const fs::path dst(args.dst);
    const fs::path staging = fs::path(dst).concat(".staging");
    const fs::path backup = fs::path(dst).concat(".bak");

    std::error_code ec;

    if (!fs::exists(src, ec)) {
        logLine("error: src does not exist");
        return 1;
    }

    // 2. Stage the update next to dst. This is the only non-atomic phase (the
    //    copy fallback when src is on another volume), and dst is still intact
    //    throughout it: a crash here loses nothing but the downloaded update.
    //    From here on every step that exposes dst is a same-volume rename.
    fs::remove_all(staging, ec);
    fs::remove_all(backup, ec);
    ec.clear();
    if (!movePath(src, staging, ec)) {
        logLine("error: failed to stage the update: " + ec.message());
        std::error_code rmEc;
        fs::remove_all(staging, rmEc);
        return 1;
    }

    // 3. Verify the staged install while dst is untouched (platform-specific
    //    check, e.g. code signature validity on macOS), instead of verifying
    //    after the swap and rolling back: this way a bad update never replaces
    //    a working install even for a moment.
    if (!platform::verifyInstall(staging.string())) {
        logLine("error: staged install failed verification");
        std::error_code rmEc;
        fs::remove_all(staging, rmEc);
        return 1;
    }

    // 4. Swap the staged install into place: preferably one atomic exchange
    //    (no window at all), otherwise two renames (the window is only between
    //    them, and the old install survives as a backup until the new one is
    //    in place).
    const bool dstExists = fs::exists(dst, ec);
    ec.clear();

    if (dstExists && exchangePaths(staging, dst)) {
        std::error_code rmEc;
        fs::remove_all(staging, rmEc); // now holds the old install
    } else {
        if (dstExists) {
            logLine(std::string("atomic exchange unavailable: ") + std::strerror(errno));
            fs::rename(dst, backup, ec);
            if (ec) {
                logLine("error: failed to move dst aside: " + ec.message());
                std::error_code rmEc;
                fs::remove_all(staging, rmEc);
                return 1;
            }
        }
        fs::rename(staging, dst, ec);
        if (ec) {
            logLine("error: failed to move staged install into place: " + ec.message());

            std::error_code rbEc;
            fs::rename(backup, dst, rbEc);
            return 1;
        }
        std::error_code rmEc;
        fs::remove_all(backup, rmEc);
    }

    // 5. Relaunch the updated application.
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
