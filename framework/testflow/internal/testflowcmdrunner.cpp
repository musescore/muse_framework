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
#include "testflowcmdrunner.h"

#include <QCoreApplication>

#include "../itestflow.h"
#include "../itestflowscriptsrepository.h"
#include "../testflowtypes.h"

#include "log.h"

using namespace muse;
using namespace muse::testflow;

static constexpr int EXIT_PASSED = 0;
static constexpr int EXIT_FAILED = 1;

void TestflowCmdRunner::start(const RunRequest& request)
{
    m_request = request;

    if (m_request.timeoutSec.has_value()) {
        m_timeoutTimer.setSingleShot(true);
        m_timeoutTimer.setInterval(m_request.timeoutSec.value() * 1000);
        QObject::connect(&m_timeoutTimer, &QTimer::timeout, [this]() {
            LOGE() << "[testflow] FAILED: timeout " << m_request.timeoutSec.value() << " sec"
                   << (m_started ? "" : ", the application context did not become ready");
            finish(false);
        });
        m_timeoutTimer.start();
    }

    //! NOTE The test case runs in the first ready context
    for (const modularity::ContextPtr& ctx : application()->contexts()) {
        if (application()->isContextReady(ctx)) {
            run(ctx);
            return;
        }
    }

    application()->contextReady().onReceive(this, [this](const modularity::ContextPtr& ctx) {
        run(ctx);
    });
}

void TestflowCmdRunner::run(const modularity::ContextPtr& ctx)
{
    if (m_started) {
        return;
    }
    m_started = true;

    //! NOTE Let the ready notification finish before the script takes over the event loop
    QMetaObject::invokeMethod(qApp, [this, ctx]() {
        exec(ctx);
    }, Qt::QueuedConnection);
}

void TestflowCmdRunner::exec(const modularity::ContextPtr& ctx)
{
    auto repository = modularity::ioc(ctx)->resolve<ITestflowScriptsRepository>("testflow");
    auto testflow = modularity::ioc(ctx)->resolve<ITestflow>("testflow");
    IF_ASSERT_FAILED(repository && testflow) {
        finish(false);
        return;
    }

    const io::path_t scriptPath = repository->resolveScript(QString::fromStdString(m_request.nameOrFile));
    if (scriptPath.empty()) {
        LOGE() << "[testflow] FAILED: script not found: " << m_request.nameOrFile;
        finish(false);
        return;
    }

    if (m_request.speed != SpeedMode::Undefined) {
        testflow->setSpeedMode(m_request.speed);
    }

    ExecOptions opt;
    opt.context = m_request.context;
    opt.contextVal = m_request.contextVal;
    opt.func = m_request.func;
    opt.funcArgs = m_request.funcArgs;
    opt.interactive = false;

    LOGI() << "[testflow] running script: " << scriptPath;

    const ITestflow::RunResult result = testflow->execScript(scriptPath, opt);

    if (m_finished) {
        //! NOTE Finished by timeout during the run
        return;
    }

    const bool passed = result.status == ITestflow::Status::Finished && result.testCasesCount > 0;

    const std::string summary = "[testflow] " + std::string(passed ? "PASSED" : "FAILED")
                                + " status: " + ITestflow::statusToString(result.status).toStdString()
                                + ", test cases: " + std::to_string(result.testCasesCount)
                                + ", script: " + scriptPath.toStdString()
                                + ", report: " + result.reportPath.toStdString();
    if (passed) {
        LOGI() << summary;
    } else {
        LOGE() << summary;
        if (!result.failedStep.isEmpty()) {
            LOGE() << "[testflow] failed step: " << result.failedStep;
        }
        if (!result.ret) {
            LOGE() << "[testflow] error: " << result.ret.toString();
        }
        if (result.status == ITestflow::Status::Finished && result.testCasesCount == 0) {
            LOGE() << "[testflow] no test case was run, does the script call api.testflow.runTestCase?";
        }
    }

    finish(passed);
}

void TestflowCmdRunner::finish(bool passed)
{
    if (m_finished) {
        return;
    }
    m_finished = true;

    m_timeoutTimer.stop();

    //! NOTE Exits all nested event loops too (a running test case, opened dialogs)
    qApp->exit(passed ? EXIT_PASSED : EXIT_FAILED);
}
