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
#include <cstdlib>
#include <ctime>

#include <spawn.h>
#include <sys/wait.h>

extern char** environ;

namespace {
void sleepMs(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}

int runDetachedAndWait(const char* path, char* const argv[])
{
    pid_t pid = 0;
    int rc = posix_spawn(&pid, path, nullptr, nullptr, argv, environ);
    if (rc != 0) {
        return -1;
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
}

namespace platform {
void waitForProcessExit(long long pid, int timeoutMs)
{
    const int step = 100;
    int waited = 0;
    while (waited < timeoutMs) {
        if (::kill(static_cast<pid_t>(pid), 0) != 0) {
            // No such process -> it has exited.
            return;
        }
        sleepMs(step);
        waited += step;
    }
}

bool verifyInstall(const std::string& path)
{
    std::string cmd = "/usr/bin/codesign --verify --deep --strict \"" + path + "\"";
    return std::system(cmd.c_str()) == 0;
}

bool relaunch(const std::string& path)
{
    char* const argv[] = {
        const_cast<char*>("open"),
        const_cast<char*>(path.c_str()),
        nullptr
    };
    return runDetachedAndWait("/usr/bin/open", argv) == 0;
}
}
