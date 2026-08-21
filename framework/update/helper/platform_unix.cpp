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

#include "platform.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>

#include <spawn.h>

extern char** environ;

namespace {
void sleepMs(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}

//! An AppImage is an ELF executable carrying "AI" plus the format version in the
//! bytes the ELF header reserves for the OS ABI. Only type 2 is published.
bool hasAppImageHeader(const std::string& path)
{
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        return false;
    }

    unsigned char header[11] = { 0 };
    const size_t read = std::fread(header, 1, sizeof(header), file);
    std::fclose(file);

    if (read != sizeof(header)) {
        return false;
    }

    static const unsigned char ELF_MAGIC[] = { 0x7f, 'E', 'L', 'F' };
    if (std::memcmp(header, ELF_MAGIC, sizeof(ELF_MAGIC)) != 0) {
        return false;
    }

    return header[8] == 'A' && header[9] == 'I' && header[10] == 0x02;
}
}

namespace platform {
bool waitForProcessExit(long long pid, int timeoutMs)
{
    const int step = 100;
    int waited = 0;
    while (waited < timeoutMs) {
        if (::kill(static_cast<pid_t>(pid), 0) != 0) {
            // EPERM means the process still exists
            return errno == ESRCH;
        }
        sleepMs(step);
        waited += step;
    }
    return ::kill(static_cast<pid_t>(pid), 0) != 0 && errno == ESRCH;
}

bool verifyInstall(const std::string& path)
{
    //! NOTE: There is no signature to check against on Linux, so this only
    //! establishes that what was swapped in is a launchable AppImage - enough to
    //! catch a truncated or half-copied file and roll back instead of leaving
    //! the user without a working application.
    //! The executable bit is not an integrity property - `relaunch` sets it
    //! unconditionally - so it is deliberately not checked here.
    struct stat st;
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return false;
    }

    return hasAppImageHeader(path);
}

bool relaunch(const std::string& path)
{
    // Ensure the AppImage is executable, then launch it detached.
    ::chmod(path.c_str(), 0755);

    char* const argv[] = {
        const_cast<char*>(path.c_str()),
        nullptr
    };

    pid_t pid = 0;
    int rc = posix_spawn(&pid, path.c_str(), nullptr, nullptr, argv, environ);
    return rc == 0;
}
}
