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

#include <gtest/gtest.h>

#include <QEventLoop>
#include <QJSEngine>
#include <QTimer>

#include "async/asyncable.h"

#include "testflow/internal/testcaserunner.h"

#include "stepshandler.h"

using namespace muse;
using namespace muse::testflow;

//! NOTE Readable failure messages
inline void PrintTo(const QString& str, std::ostream* os)
{
    *os << '"' << str.toStdString() << '"';
}

namespace muse::testflow {
class Testflow_TestCaseRunnerTests : public ::testing::Test, public async::Asyncable
{
public:
    struct Event {
        QString name;
        StepStatus status = StepStatus::Undefined;
        Ret ret;
    };

    void SetUp() override
    {
        QJSEngine::setObjectOwnership(&m_handler, QJSEngine::CppOwnership);
        m_engine.globalObject().setProperty("h", m_engine.newQObject(&m_handler));

        m_runner.setDefaultInterval(0);

        m_runner.stepStatusChanged().onReceive(this, [this](const StepInfo& info, const Ret& ret) {
            m_events.push_back({ info.name, info.status, ret });
        });

        m_runner.allFinished().onReceive(this, [this](bool aborted) {
            m_allFinishedCount++;
            m_aborted = aborted;
        });
    }

    //! NOTE Each step calls the handler with its name, `func` replaces the default step body
    TestCase makeTestCase(const QString& steps)
    {
        QJSValue val = m_engine.evaluate("({ name: 'tc', steps: [" + steps + "] })");
        EXPECT_FALSE(val.isError()) << val.toString().toStdString();
        return TestCase(val);
    }

    static QString step(const QString& name)
    {
        return QString("{ name: '%1', func: function() { return h.call('%1') } }").arg(name);
    }

    std::vector<StepStatus> statusesOf(const QString& name) const
    {
        std::vector<StepStatus> result;
        for (const Event& e : m_events) {
            if (e.name == name) {
                result.push_back(e.status);
            }
        }
        return result;
    }

    //! NOTE Spins an event loop inside a step, like waiting for a control does
    static void spinEventLoop(int msec, const std::function<void()>& onTimer = nullptr)
    {
        QEventLoop loop;
        if (onTimer) {
            QTimer::singleShot(0, onTimer);
        }
        QTimer::singleShot(msec, &loop, &QEventLoop::quit);
        loop.exec();
    }

    //! NOTE Quits the test case loop from outside, like the application quitting does
    void quitFromOutside(int msec)
    {
        QTimer::singleShot(msec, [this]() {
            m_runner.m_testCase.loop.quit();
        });
    }

    bool hasErrorText(const std::string& text) const
    {
        return !m_events.empty() && m_events.back().ret.text().find(text) != std::string::npos;
    }

    QJSEngine m_engine;
    StepsHandler m_handler;
    TestCaseRunner m_runner;

    std::vector<Event> m_events;
    int m_allFinishedCount = 0;
    bool m_aborted = false;
};

TEST_F(Testflow_TestCaseRunnerTests, RunsAllStepsInOrder)
{
    m_runner.run(makeTestCase(step("a") + "," + step("b") + "," + step("c")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a", "b", "c" }));
    for (const QString& name : { "a", "b", "c" }) {
        EXPECT_EQ(statusesOf(name), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Finished }));
    }

    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, EmptyTestCase)
{
    m_runner.run(makeTestCase(""));

    EXPECT_TRUE(m_handler.calls.isEmpty());
    EXPECT_TRUE(m_events.empty());
}

TEST_F(Testflow_TestCaseRunnerTests, SkippedStepIsNotRun)
{
    m_runner.run(makeTestCase(step("a")
                              + ", { name: 'b', skip: true, func: function() { return h.call('b') } },"
                              + step("c")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a", "c" }));
    EXPECT_EQ(statusesOf("b"), std::vector<StepStatus>({ StepStatus::Skipped }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, FailedStepStopsTheTestCase)
{
    m_handler.handlers["b"] = []() { return false; };

    m_runner.run(makeTestCase(step("a") + "," + step("b") + "," + step("c")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a", "b" }));
    EXPECT_EQ(statusesOf("b"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Error }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, ExceptionInStepIsError)
{
    m_runner.run(makeTestCase(step("a")
                              + ", { name: 'b', func: function() { throw new Error('some error') } },"
                              + step("c")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a" }));
    ASSERT_EQ(statusesOf("b"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Error }));
    EXPECT_TRUE(m_events.back().ret.text().find("some error") != std::string::npos);
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, AbortExceptionInStepIsAborted)
{
    m_runner.run(makeTestCase("{ name: 'a', func: function() { throw new Error('abort') } }," + step("b")));

    EXPECT_TRUE(m_handler.calls.isEmpty());
    EXPECT_EQ(statusesOf("a"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Aborted }));
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, AbortDuringStepStopsBeforeNextStep)
{
    m_handler.handlers["a"] = [this]() {
        m_runner.abort();
        return true;
    };

    m_runner.run(makeTestCase(step("a") + "," + step("b")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a" }));
    EXPECT_EQ(statusesOf("a"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Finished }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, PauseDuringStepWaitsForUnpause)
{
    // [GIVEN] The step `a` pauses the test case
    m_handler.handlers["a"] = [this]() {
        m_runner.pause();

        // [GIVEN] The test case is unpaused after the step is finished
        QTimer::singleShot(50, [this]() {
            m_handler.calls << "unpause";
            m_runner.unpause(true);
        });
        return true;
    };

    // [WHEN] Run
    m_runner.run(makeTestCase(step("a") + "," + step("b")));

    // [THEN] The step `b` is run after the unpause, once
    EXPECT_EQ(m_handler.calls, QStringList({ "a", "unpause", "b" }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, UnpauseDuringStepDoesNotRunNextStepInsideIt)
{
    // [GIVEN] The step `a` is paused and unpaused while it waits in its own event loop
    m_handler.handlers["a"] = [this]() {
        spinEventLoop(50, [this]() {
            m_runner.pause();
            m_runner.unpause(true);
        });

        m_handler.calls << "a finished";
        return true;
    };

    // [WHEN] Run
    m_runner.run(makeTestCase(step("a") + "," + step("b") + "," + step("c")));

    // [THEN] The next steps are run after the step `a` is finished, each once
    EXPECT_EQ(m_handler.calls, QStringList({ "a", "a finished", "b", "c" }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, RunAgainAfterFinished)
{
    m_runner.run(makeTestCase(step("a")));
    m_runner.run(makeTestCase(step("b")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a", "b" }));
    EXPECT_EQ(m_allFinishedCount, 2);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, RunAgainAfterAborted)
{
    m_handler.handlers["a"] = []() { return false; };
    m_runner.run(makeTestCase(step("a")));
    EXPECT_TRUE(m_aborted);

    m_runner.run(makeTestCase(step("b")));

    EXPECT_EQ(m_handler.calls, QStringList({ "a", "b" }));
    EXPECT_EQ(m_allFinishedCount, 2);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, SpeedModeInterval)
{
    m_runner.setDefaultInterval(123);

    m_runner.setSpeedMode(SpeedMode::Default);
    EXPECT_EQ(m_runner.intervalMsec(), 123);

    m_runner.setSpeedMode(SpeedMode::Fast);
    EXPECT_EQ(m_runner.intervalMsec(), 250);

    m_runner.setSpeedMode(SpeedMode::Normal);
    EXPECT_EQ(m_runner.intervalMsec(), 1000);

    m_runner.setSpeedMode(SpeedMode::Slow);
    EXPECT_EQ(m_runner.intervalMsec(), 2000);
}

TEST_F(Testflow_TestCaseRunnerTests, ExpectedQuitFinishesTestCase)
{
    // [GIVEN] The step `a` quits the application, as expected
    m_handler.handlers["a"] = [this]() {
        m_runner.expectQuit(5000);
        quitFromOutside(10);
        return true;
    };

    // [WHEN] Run
    m_runner.run(makeTestCase(step("a") + "," + step("b")));

    // [THEN] The test case is finished, the remaining steps are not run
    EXPECT_EQ(m_handler.calls, QStringList({ "a" }));
    EXPECT_EQ(statusesOf("a"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Finished }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_FALSE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, UnexpectedQuitAbortsTestCase)
{
    // [GIVEN] The application quits during the test case, not expected
    m_runner.setDefaultInterval(100);
    m_handler.handlers["a"] = [this]() {
        quitFromOutside(10);
        return true;
    };

    // [WHEN] Run
    m_runner.run(makeTestCase(step("a") + "," + step("b")));

    // [THEN] The test case is aborted
    EXPECT_EQ(m_handler.calls, QStringList({ "a" }));
    EXPECT_EQ(statusesOf("a"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Finished, StepStatus::Aborted }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, ExpectedQuitDidNotHappen)
{
    // [GIVEN] The step `a` expects the application to quit, but it does not
    m_handler.handlers["a"] = [this]() {
        m_runner.expectQuit(50);
        return true;
    };

    // [WHEN] Run
    m_runner.run(makeTestCase(step("a") + "," + step("b")));

    // [THEN] The step fails after the timeout, the remaining steps are not run
    EXPECT_EQ(m_handler.calls, QStringList({ "a" }));
    EXPECT_EQ(statusesOf("a"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Finished, StepStatus::Error }));
    EXPECT_TRUE(hasErrorText("did not quit"));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, AbortWhileWaitingForQuit)
{
    // [GIVEN] The test case is aborted while it waits for the application to quit
    m_handler.handlers["a"] = [this]() {
        m_runner.expectQuit(5000);
        QTimer::singleShot(10, [this]() {
            m_runner.abort();
        });
        return true;
    };

    // [WHEN] Run
    m_runner.run(makeTestCase(step("a") + "," + step("b")));

    // [THEN] The test case is aborted without waiting for the timeout
    EXPECT_EQ(m_handler.calls, QStringList({ "a" }));
    EXPECT_EQ(statusesOf("a"), std::vector<StepStatus>({ StepStatus::Started, StepStatus::Finished }));
    EXPECT_EQ(m_allFinishedCount, 1);
    EXPECT_TRUE(m_aborted);
}

TEST_F(Testflow_TestCaseRunnerTests, RunAgainAfterExpectedQuit)
{
    m_handler.handlers["a"] = [this]() {
        m_runner.expectQuit(5000);
        quitFromOutside(10);
        return true;
    };
    m_runner.run(makeTestCase(step("a")));

    // [WHEN] Run another test case
    m_runner.run(makeTestCase(step("b") + "," + step("c")));

    // [THEN] It does not expect the quit, all its steps are run
    EXPECT_EQ(m_handler.calls, QStringList({ "a", "b", "c" }));
    EXPECT_EQ(m_allFinishedCount, 2);
    EXPECT_FALSE(m_aborted);
}
}
