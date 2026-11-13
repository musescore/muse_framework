/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
#ifndef MUSE_TESTFLOW_ITESTFLOW_H
#define MUSE_TESTFLOW_ITESTFLOW_H

#include <vector>
#include <QJSValue>

#include "modularity/imoduleinterface.h"
#include "io/path.h"
#include "async/channel.h"
#include "testflowtypes.h"
#include "itestcasecontext.h"
#include "internal/testflowinteractive.h"

namespace muse::testflow {
class ITestflow : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(ITestflow)
public:
    virtual ~ITestflow() = default;

    enum class Status {
        Undefined = 0,
        Running,
        Paused,
        Aborted,
        Error,
        Finished
    };

    static QString statusToString(Status st)
    {
        switch (st) {
        case ITestflow::Status::Undefined: return "";
        case ITestflow::Status::Running: return "Running";
        case ITestflow::Status::Paused: return "Paused";
        case ITestflow::Status::Aborted: return "Aborted";
        case ITestflow::Status::Error: return "Error";
        case ITestflow::Status::Finished: return "Finished";
        }
        return QString();
    }

    virtual Status status() const = 0;
    virtual async::Channel<io::path_t, Status> statusChanged() const = 0;
    virtual async::Channel<StepInfo, Ret> stepStatusChanged() const = 0;

    virtual SpeedMode speedMode() const = 0;
    virtual void setSpeedMode(SpeedMode mode) = 0;
    virtual async::Channel<SpeedMode> speedModeChanged() const = 0;
    virtual void setDefaultIntervalMsec(int msec) = 0;
    virtual int defaultIntervalMsec() const = 0;
    virtual int intervalMsec() const = 0;

    struct RunResult {
        Status status = Status::Undefined;
        Ret ret;
        int testCasesCount = 0;
        QString failedStep;
        io::path_t reportPath;
    };

    virtual RunResult execScript(const io::path_t& path, const ExecOptions& opt = ExecOptions()) = 0;

    virtual void runTestCase(const TestCase& testCase) = 0;
    virtual void sleep(int msec) = 0;
    virtual void pause() = 0;
    virtual void unpause() = 0;
    virtual void abort() = 0;
    virtual void fatal(const QString& msg) = 0;
    virtual bool isInteractive() const = 0;

    //! NOTE Nobody can answer dialogs while no test case runs (e.g. on the application startup in a command line run),
    //! so they are skipped (see TestflowInteractive::UnattendedDialogs)
    virtual void setSkipDialogsOutsideRun(bool skip) = 0;

    virtual ITestCaseContextPtr context() const = 0;
    virtual TestflowInteractivePtr testflowInteractive() const = 0;
};
}

#endif // MUSE_TESTFLOW_ITESTFLOW_H
