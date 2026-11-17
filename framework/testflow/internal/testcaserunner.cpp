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
#include "testcaserunner.h"

#include <QTimer>

#include "log.h"

#include "scriptengine.h"

using namespace muse;
using namespace muse::testflow;

TestCaseRunner::TestCaseRunner()
{
    m_quitTimer.setSingleShot(true);
    QObject::connect(&m_quitTimer, &QTimer::timeout, [this]() {
        const QString msg = QString("the application did not quit in %1 msec").arg(m_testCase.quitTimeoutMsec);
        LOGE() << msg << ", at step: " << m_testCase.lastStepName;
        m_stepStatusChanged.send(StepInfo(m_testCase.lastStepName, StepStatus::Error),
                                 make_ret(Ret::Code::UnknownError, msg.toStdString()));
        doAbort();
    });
}

SpeedMode TestCaseRunner::speedMode() const
{
    return m_speedMode;
}

void TestCaseRunner::setSpeedMode(SpeedMode mode)
{
    if (m_speedMode == mode) {
        return;
    }

    m_speedMode = mode;
    m_speedModeChanged.send(mode);
}

async::Channel<SpeedMode> TestCaseRunner::speedModeChanged() const
{
    return m_speedModeChanged;
}

void TestCaseRunner::setDefaultInterval(int msec)
{
    m_intervalMsec = msec;
}

int TestCaseRunner::defaultInterval() const
{
    return m_intervalMsec;
}

void TestCaseRunner::run(const TestCase& testCase)
{
    m_abort = false;
    m_quitTimer.stop();
    m_testCase.reset();
    m_testCase.testCase = testCase;
    m_testCase.steps = testCase.steps();
    m_testCase.stepsCount = m_testCase.steps.count();

    nextStep();

    if (m_testCase.currentStepIdx < m_testCase.stepsCount) {
        m_testCase.loop.exec();
    }

    m_quitTimer.stop();

    //! NOTE The loop was quit from outside, because the application quit as the test case expected
    if (!m_testCase.stopped && m_testCase.stepsCount > 0 && m_testCase.quitExpected) {
        m_testCase.stopped = true;
        LOGI() << "test case finished by the expected quit, at step: " << m_testCase.lastStepName;
        m_allFinished.send(false);
        return;
    }

    //! NOTE The loop was quit from outside (for example, the application is exiting),
    //! the remaining steps were not run, so this is not a success
    if (!m_testCase.stopped && m_testCase.stepsCount > 0) {
        m_abort = true;
        m_testCase.stopped = true;
        LOGE() << "test case interrupted at step: " << m_testCase.lastStepName;
        m_stepStatusChanged.send(StepInfo(m_testCase.lastStepName, StepStatus::Aborted),
                                 make_ret(Ret::Code::Cancel, std::string("test case interrupted")));
        m_allFinished.send(true);
    }
}

void TestCaseRunner::pause()
{
    m_paused = true;
    m_stepStatusChanged.send(StepInfo(m_testCase.lastStepName, StepStatus::Paused), muse::make_ok());
}

void TestCaseRunner::unpause(bool isNextStep)
{
    m_paused = false;
    m_stepStatusChanged.send(StepInfo(m_testCase.lastStepName, StepStatus::Started), muse::make_ok());

    //! NOTE A step may wait in its own event loop (not the testflow sleep),
    //! the next step will be run when the current one finishes
    if (isNextStep && !m_stepExecuting) {
        nextStep(false);
    }
}

void TestCaseRunner::abort()
{
    m_abort = true;

    //! NOTE No step runs while waiting for the quit, so nothing else checks the abort flag
    if (m_quitTimer.isActive()) {
        doAbort();
    }
}

void TestCaseRunner::expectQuit(int timeoutMsec)
{
    m_testCase.quitExpected = true;
    m_testCase.quitTimeoutMsec = timeoutMsec;
}

void TestCaseRunner::waitForQuit()
{
    LOGI() << "waiting for the application to quit, at step: " << m_testCase.lastStepName;
    m_quitTimer.start(m_testCase.quitTimeoutMsec);
}

async::Channel<StepInfo, Ret> TestCaseRunner::stepStatusChanged() const
{
    return m_stepStatusChanged;
}

async::Channel<bool> TestCaseRunner::allFinished() const
{
    return m_allFinished;
}

void TestCaseRunner::doAbort()
{
    m_quitTimer.stop();
    m_testCase.stopped = true;
    m_allFinished.send(true);
    m_testCase.loop.quit();
}

int TestCaseRunner::intervalMsec() const
{
    switch (m_speedMode) {
    case SpeedMode::Undefined: return m_intervalMsec;
    case SpeedMode::Default: return m_intervalMsec;
    case SpeedMode::Fast: return 250;
    case SpeedMode::Normal: return 1000;
    case SpeedMode::Slow: return 2000;
    }
    return m_intervalMsec;
}

void TestCaseRunner::nextStep(bool byInterval)
{
    if (m_abort) {
        doAbort();
        return;
    }

    if (m_paused) {
        return;
    }

    m_testCase.currentStepIdx += 1;

    if (m_testCase.currentStepIdx >= m_testCase.stepsCount) {
        return;
    }

    //! NOTE The context cancels the timer with the runner
    QTimer::singleShot(byInterval ? intervalMsec() : 0, &m_stepTimerContext, [this]() {
        //! NOTE The test case was interrupted from outside while waiting for this step
        if (m_testCase.stopped) {
            return;
        }

        Step step = m_testCase.steps.step(m_testCase.currentStepIdx);
        QString name = step.name();
        m_testCase.lastStepName = name;

        if (step.skip()) {
            LOGD() << "step: " << name << " Skipped";
            m_stepStatusChanged.send(StepInfo(name, StepStatus::Skipped), muse::make_ok());
        } else {
            LOGD() << "step: " << name << " Started";
            m_stepStatusChanged.send(StepInfo(name, StepStatus::Started), muse::make_ok());
            m_elapsed.restart();

            m_stepExecuting = true;
            Ret ret = step.exec();
            m_stepExecuting = false;
            if (!ret) {
                LOGE() << "failed exec step: " << name << ", err: " << ret.toString();
                StepStatus status = static_cast<Ret::Code>(ret.code()) == Ret::Code::Cancel ? StepStatus::Aborted : StepStatus::Error;
                m_stepStatusChanged.send(StepInfo(step.name(), status), ret);
                doAbort();
                return;
            }

            LOGD() << "step: " << name << " Finished";
            m_stepStatusChanged.send(StepInfo(step.name(), StepStatus::Finished, m_elapsed.elapsed()), muse::make_ok());

            //! NOTE The remaining steps are not run, the test case is finished when the application quits
            if (m_testCase.quitExpected) {
                waitForQuit();
                return;
            }
        }

        bool withInterval = step.skip() ? false : true;
        nextStep(withInterval);

        m_testCase.finishedCount += 1;
        if (m_testCase.finishedCount == m_testCase.stepsCount) {
            m_testCase.stopped = true;
            m_allFinished.send(false);
            m_testCase.loop.quit();
        }
    });
}
