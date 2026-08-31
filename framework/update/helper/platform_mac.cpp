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

#include <cctype>
#include <cerrno>
#include <csignal>
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

bool verifyInstall(const std::string& path, const std::string& team)
{
    //! NOTE: A plain codesign verify accepts any validly signed bundle,
    //! including ad-hoc signed ones anyone can produce. The requirement pins
    //! the signer to the host application's team, so a bundle planted into
    //! the staging directory by someone else never passes.
    if (team.empty()) {
        return false;
    }

    for (const char c : team) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    const std::string requirement = "=anchor apple generic and certificate leaf[subject.OU] = \"" + team + "\"";

    char* const argv[] = {
        const_cast<char*>("codesign"),
        const_cast<char*>("--verify"),
        const_cast<char*>("--deep"),
        const_cast<char*>("--strict"),
        const_cast<char*>("-R"),
        const_cast<char*>(requirement.c_str()),
        const_cast<char*>(path.c_str()),
        nullptr
    };
    return runDetachedAndWait("/usr/bin/codesign", argv) == 0;
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
