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
#include "testflow.h"

#include <QTimer>
#include <QWindow>

#include "global/io/file.h"
#include "global/serialization/json.h"

#include "draw/types/font.h"

#include "testcasecontext.h"
#include "../testflowutils.h"

#include "log.h"

using namespace muse;
using namespace muse::testflow;

void Testflow::init()
{
    m_testflowInteractive = std::make_shared<TestflowInteractive>();

    m_runner.stepStatusChanged().onReceive(this, [this](const StepInfo& stepInfo, const Ret& ret) {
        if (stepInfo.status == StepStatus::Started) {
            m_context->addStep(stepInfo.name);
        }

        m_report.onStepStatusChanged(stepInfo, m_context);

        if (stepInfo.status == StepStatus::Aborted || stepInfo.status == StepStatus::Error) {
            m_runResult.failedStep = stepInfo.name;
            m_runResult.ret = ret;
        }

        m_stepStatusChanged.send(stepInfo, ret);

        if (stepInfo.status == StepStatus::Aborted) {
            setStatus(Status::Aborted);
        } else if (stepInfo.status == StepStatus::Error) {
            setStatus(Status::Error);
        }
    });

    m_runner.allFinished().onReceive(this, [this](bool aborted) {
        m_report.endReport(aborted);
    });

    //! NOTE A script that dispatches an unknown action or command is out of date,
    //! fail the current step instead of silently doing nothing
    if (actionsDispatcher()) {
        actionsDispatcher()->notRegistered().onReceive(this, [this](const actions::ActionCode& code) {
            failRunningStep(QString("not registered action: ") + QString::fromStdString(code));
        });
    }

    if (commandDispatcher()) {
        commandDispatcher()->notRegistered().onReceive(this, [this](const rcommand::Command& command) {
            failRunningStep(QString("not registered command: ") + QString::fromStdString(command.toString()));
        });
    }

    setStatus(Status::Undefined);
    setSpeedMode(SpeedMode::Default);
}

void Testflow::affectOnServices()
{
    IApplication::RunMode runMode = application()->runMode();
    if (runMode == IApplication::RunMode::GuiApp) {
        //! NOTE Move focus to main window
        mainWindow()->qWindow()->requestActivate();

        //! NOTE Disable reset on mouse press for testing purpose
        navigation()->setIsResetOnMousePress(false);

        //! NOTE Set navigation highlight
        navigation()->setIsHighlight(true);

        //! NOTE Only defaults shortcuts
#ifdef MUSE_MODULE_SHORTCUTS_V2
        commandShortcutsRegister()->reload(true);
#else
        shortcutsRegister()->reload(true);
#endif

        //! NOTE Change Interactive implementation
        setTestflowInteractiveInstalled(true);
        m_testflowInteractive->setSkipDialogs(false);
    }

    m_affectedServiceState.fontDisabledMerging = muse::draw::Font::g_disableFontMerging;
    muse::draw::Font::g_disableFontMerging = true;
}

void Testflow::setTestflowInteractiveInstalled(bool installed)
{
    modularity::ModulesContextIoC* ioc = modularity::ioc(iocContext());
    const bool isInstalled = ioc->resolve<IInteractive>("testflow") == m_testflowInteractive;
    if (installed == isInstalled) {
        return;
    }

    if (installed) {
        m_testflowInteractive->setRealInteractive(ioc->resolve<IInteractive>("testflow"));
        ioc->unregister<IInteractive>("testflow");
        ioc->registerExport<IInteractive>("testflow", m_testflowInteractive);
    } else {
        ioc->unregister<IInteractive>("testflow");
        ioc->registerExport<IInteractive>("testflow", m_testflowInteractive->realInteractive());
    }
}

void Testflow::setSkipDialogsOutsideRun(bool skip)
{
    m_skipDialogsOutsideRun = skip;

    if (application()->runMode() != IApplication::RunMode::GuiApp || status() == Status::Running) {
        return;
    }

    setTestflowInteractiveInstalled(skip);
    m_testflowInteractive->setSkipDialogs(skip);
}

void Testflow::restoreAffectOnServices()
{
    IApplication::RunMode runMode = application()->runMode();
    if (runMode == IApplication::RunMode::GuiApp) {
        navigation()->setIsResetOnMousePress(true);
#ifdef MUSE_MODULE_SHORTCUTS_V2
        commandShortcutsRegister()->reload(false);
#else
        shortcutsRegister()->reload(false);
#endif

        m_testflowInteractive->setSkipDialogs(m_skipDialogsOutsideRun);
        setTestflowInteractiveInstalled(m_skipDialogsOutsideRun);
    }

    muse::draw::Font::g_disableFontMerging = m_affectedServiceState.fontDisabledMerging;
}

void Testflow::loadContext(ITestCaseContextPtr ctx, const io::path_t& context, const std::string& contextVal, ScriptEngine* e)
{
    auto loadFromJson = [ctx, e](const ByteArray& json) {
        std::string err;
        JsonObject ctxObj = JsonDocument::fromJson(json, &err).rootObject();
        if (!err.empty()) {
            LOGE() << "failed parse context value, err: " << err;
            return;
        }

        std::vector<std::string> keys = ctxObj.keys();
        for (const std::string& k : keys) {
            ctx->setGlobalVal(QString::fromStdString(k), toQJSValue(ctxObj.value(k), e));
        }
    };

    // from file
    if (!context.empty()) {
        ctx->setGlobalVal("context_path", context.toQString());

        ByteArray json;
        Ret ret = io::File::readFile(context, json);
        if (!ret) {
            LOGE() << "failed read file: " << context;
            return;
        }
        loadFromJson(json);
    }

    // from value (maybe override)
    if (!contextVal.empty()) {
        ByteArray json = ByteArray::fromRawData(contextVal.c_str(), contextVal.size());
        loadFromJson(json);
    }
}

QJSValueList Testflow::parseFuncArgs(const std::string& funcArgs, ScriptEngine* e) const
{
    ByteArray json = ByteArray::fromRawData(funcArgs.c_str(), funcArgs.size());
    std::string err;
    JsonArray arr = JsonDocument::fromJson(json, &err).rootArray();
    if (!err.empty()) {
        LOGE() << "failed parse context value, err: " << err;
        return QJSValueList();
    }

    QJSValueList args;
    for (size_t i = 0; i < arr.size(); ++i) {
        const JsonValue& a = arr.at(i);
        args.append(toQJSValue(a, e));
    }

    return args;
}

bool Testflow::isInteractive() const
{
    return m_interactive;
}

ITestflow::RunResult Testflow::execScript(const io::path_t& path, const ExecOptions& opt)
{
    LOGD() << "path: " << path
           << ", context: " << opt.context
           << ", contextVal: " << opt.contextVal
           << ", func: " << opt.func
           << ", funcArgs: " << opt.funcArgs;

    if (status() == Status::Running || status() == Status::Paused) {
        abort();
    }

    //! NOTE If an error occurred during the execution of TestCase and a dialog was opened at that time,
    //! the TestCase loop does not exit because of the open dialog,
    //! so we need to close all dialogs in order to complete the execution of the previous script.
    if (m_engine) {
        std::vector<Uri> stack = interactive()->stack();
        if (stack.size() > 1) {
            const Uri& uri = stack.back();
            interactive()->close(uri);
            QTimer::singleShot(1000, [this, path, opt]() {
                execScript(path, opt);
            });
            return RunResult();
        }
    }

    if (m_engine) {
        LOGE() << "unknown internal error with prev execScript";
        delete m_engine;
        m_engine = nullptr;
    }

    IF_ASSERT_FAILED(!m_engine) {
        delete m_engine;
        m_engine = nullptr;
    }

    affectOnServices();

    m_engine = new ScriptEngine(iocContext());
    m_engine->setScriptPath(path);

    m_context = std::make_shared<TestCaseContext>();
    m_context->setGlobalVal("script_path", path.toQString());
    loadContext(m_context, opt.context, opt.contextVal, m_engine);

    m_runResult = RunResult();
    m_interactive = opt.interactive;
    if (!m_interactive) {
        m_testflowInteractive->setUnattendedMessageHandler([this](const std::string& type, const std::string& title,
                                                                  const std::string& text) {
            failRunningStep(QString("unexpected message (%1): %2: %3")
                            .arg(QString::fromStdString(type), QString::fromStdString(title), QString::fromStdString(text)));
        });
    }

    setStatus(Status::Running);
    QString func = opt.func.empty() ? QString("main") : QString::fromStdString(opt.func);
    QJSValueList args = opt.funcArgs.empty() ? QJSValueList() : parseFuncArgs(opt.funcArgs, m_engine);
    Ret ret = m_engine->call(func, args);
    if (!ret) {
        LOGE() << ret.toString();
    }

    //! NOTE Also maybe abort or error (set by a failed step)
    if (status() == Status::Running) {
        setStatus(ret ? Status::Finished : Status::Error);
    }

    delete m_engine;
    m_engine = nullptr;

    restoreAffectOnServices();

    m_interactive = true;
    m_testflowInteractive->setUnattendedMessageHandler(nullptr);

    RunResult result = m_runResult;
    result.status = status();
    //! NOTE The failed step error is more precise than the script one
    if (!result.ret.valid()) {
        result.ret = ret;
    }
    result.reportPath = m_report.reportPath();

    return result;
}

SpeedMode Testflow::speedMode() const
{
    return m_runner.speedMode();
}

void Testflow::setSpeedMode(SpeedMode mode)
{
    m_runner.setSpeedMode(mode);
}

async::Channel<SpeedMode> Testflow::speedModeChanged() const
{
    return m_runner.speedModeChanged();
}

void Testflow::setDefaultIntervalMsec(int msec)
{
    m_runner.setDefaultInterval(msec);
}

int Testflow::defaultIntervalMsec() const
{
    return m_runner.defaultInterval();
}

int Testflow::intervalMsec() const
{
    return m_runner.intervalMsec();
}

void Testflow::runTestCase(const TestCase& testCase)
{
    m_runResult.testCasesCount++;
    m_report.beginReport(testCase);
    m_runner.run(testCase);
}

void Testflow::abort()
{
    fatal("abort");
}

void Testflow::fatal(const QString& msg)
{
    if (status() == Status::Paused) {
        unpause();
    }

    if (status() != Status::Running) {
        return;
    }

    if (m_engine) {
        m_engine->throwError(msg);
    }
    m_runner.abort();
}

void Testflow::sleep(int msec)
{
    //! NOTE If pause state, then we sleep until to unpause
    //! It's allowing to do pause during step execution
    if (status() == ITestflow::Status::Paused) {
        m_sleepLoop.exec();
        return;
    }

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &m_sleepLoop, &QEventLoop::quit);
    timer.start(msec);
    m_sleepLoop.exec();
}

void Testflow::pause()
{
    setStatus(Status::Paused);
    m_runner.pause();
}

void Testflow::unpause()
{
    bool isNextStep = true;

    //! NOTE If pause did on sleep (during step execution),
    //! then unpause current step (without perform next step)
    if (m_sleepLoop.isRunning()) {
        m_sleepLoop.quit();
        isNextStep = false;
    }

    m_runner.unpause(isNextStep);
    setStatus(Status::Running);
}

ITestCaseContextPtr Testflow::context() const
{
    return m_context;
}

TestflowInteractivePtr Testflow::testflowInteractive() const
{
    return m_testflowInteractive;
}

void Testflow::failRunningStep(const QString& what)
{
    if (!m_engine || status() != Status::Running) {
        return;
    }

    LOGE() << what;
    m_engine->throwError(what);
}

void Testflow::setStatus(Status st)
{
    if (m_status == st) {
        return;
    }

    if (m_sleepLoop.isRunning()) {
        m_sleepLoop.quit();
    }

    m_status = st;

    io::path_t path;
    if (m_engine) {
        path = m_engine->scriptPath();
    }
    m_statusChanged.send(path, st);
}

ITestflow::Status Testflow::status() const
{
    return m_status;
}

async::Channel<io::path_t, ITestflow::Status> Testflow::statusChanged() const
{
    return m_statusChanged;
}

async::Channel<StepInfo, Ret> Testflow::stepStatusChanged() const
{
    return m_stepStatusChanged;
}
