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
#include "testflowcmdline.h"

#include <QCommandLineParser>

#include "io/dir.h"

#include "log.h"

using namespace muse;
using namespace muse::testflow;

static const QString TEST_CASE("test-case");
static const QString TEST_CASE_CONTEXT("test-case-context");
static const QString TEST_CASE_CONTEXT_VALUE("test-case-context-value");
static const QString TEST_CASE_FUNC("test-case-func");
static const QString TEST_CASE_FUNC_ARGS("test-case-func-args");
static const QString TEST_CASE_SPEED("test-case-speed");
static const QString TEST_CASE_TIMEOUT("test-case-timeout");
static const QString TEST_CASE_MODE("test-case-mode");

void muse::testflow::addCmdOptions(QCommandLineParser& parser)
{
    parser.addOption(QCommandLineOption(TEST_CASE, "Run test case by name or file", "nameOrFile"));
    parser.addOption(QCommandLineOption(TEST_CASE_CONTEXT, "Set test case context by name or file", "nameOrFile"));
    parser.addOption(QCommandLineOption(TEST_CASE_CONTEXT_VALUE, "Set test case context value", "value"));
    parser.addOption(QCommandLineOption(TEST_CASE_FUNC, "Call test case function", "name"));
    parser.addOption(QCommandLineOption(TEST_CASE_FUNC_ARGS, "Call test case function args", "args"));
    parser.addOption(QCommandLineOption(TEST_CASE_SPEED, "Test case speed mode: Default|Fast|Normal|Slow", "mode"));
    parser.addOption(QCommandLineOption(TEST_CASE_TIMEOUT, "Test case timeout in seconds, including the application startup",
                                        "sec"));
    parser.addOption(QCommandLineOption(TEST_CASE_MODE, "Test case run mode: gui (default) | console", "mode"));
}

std::optional<RunRequest> muse::testflow::parseCmdOptions(const QCommandLineParser& parser, IApplication::RunMode& runMode)
{
    if (!parser.isSet(TEST_CASE)) {
        return std::nullopt;
    }

    auto fromUserInputPath = [](const QString& path) -> QString {
        return io::Dir::fromNativeSeparators(path).toQString();
    };

    RunRequest r;
    r.nameOrFile = fromUserInputPath(parser.value(TEST_CASE)).toStdString();

    if (parser.isSet(TEST_CASE_CONTEXT)) {
        r.context = fromUserInputPath(parser.value(TEST_CASE_CONTEXT));
    }

    if (parser.isSet(TEST_CASE_CONTEXT_VALUE)) {
        r.contextVal = parser.value(TEST_CASE_CONTEXT_VALUE).toStdString();
    }

    if (parser.isSet(TEST_CASE_FUNC)) {
        r.func = parser.value(TEST_CASE_FUNC).toStdString();
    }

    if (parser.isSet(TEST_CASE_FUNC_ARGS)) {
        r.funcArgs = parser.value(TEST_CASE_FUNC_ARGS).toStdString();
    }

    if (parser.isSet(TEST_CASE_SPEED)) {
        r.speed = speedModeFromString(parser.value(TEST_CASE_SPEED));
        if (r.speed == SpeedMode::Undefined) {
            LOGW() << "invalid --" << TEST_CASE_SPEED << ": " << parser.value(TEST_CASE_SPEED);
        }
    }

    if (parser.isSet(TEST_CASE_TIMEOUT)) {
        bool ok = false;
        int sec = parser.value(TEST_CASE_TIMEOUT).toInt(&ok);
        if (ok && sec > 0) {
            r.timeoutSec = sec;
        } else {
            LOGW() << "invalid --" << TEST_CASE_TIMEOUT << ": " << parser.value(TEST_CASE_TIMEOUT);
        }
    }

    if (parser.isSet(TEST_CASE_MODE)) {
        const QString mode = parser.value(TEST_CASE_MODE);
        if (mode == "console") {
            runMode = IApplication::RunMode::ConsoleApp;
        } else if (mode != "gui") {
            LOGW() << "unknown --" << TEST_CASE_MODE << ": " << mode << ", expected gui|console";
        }
    }

    return r;
}
