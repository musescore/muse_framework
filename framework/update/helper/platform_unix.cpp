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

#include <csignal>
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
}

namespace platform {
void waitForProcessExit(long long pid, int timeoutMs)
{
    const int step = 100;
    int waited = 0;
    while (waited < timeoutMs) {
        if (::kill(static_cast<pid_t>(pid), 0) != 0) {
            return;
        }
        sleepMs(step);
        waited += step;
    }
}

bool verifyInstall(const std::string& path)
{
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
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
