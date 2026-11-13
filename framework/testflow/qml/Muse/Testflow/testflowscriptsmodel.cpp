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
#include "testflowscriptsmodel.h"

#include <QCoreApplication>

#include "io/path.h"

#include "log.h"

using namespace muse::testflow;

static QString typeToString(ScriptType t)
{
    switch (t) {
    case ScriptType::Undefined: return "Undefined";
    case ScriptType::TestCase: return "TestCase";
    case ScriptType::Custom: return "Custom";
    }
    return "";
}

ScriptType typeFromString(const QString& str)
{
    if (str == "TestCase") {
        return ScriptType::TestCase;
    }
    if (str == "Custom") {
        return ScriptType::Custom;
    }
    return ScriptType::Undefined;
}

TestflowScriptsModel::TestflowScriptsModel(QObject* parent)
    : QAbstractListModel(parent), Contextable(muse::iocCtxForQmlObject(this))
{
}

TestflowScriptsModel::~TestflowScriptsModel()
{
    //! NOTE The runs in new windows must not report to the model being destroyed
    for (QProcess* process : m_processes) {
        process->disconnect(this);
    }

    stopRunAllTC();
}

QVariant TestflowScriptsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    const Script& script = m_scripts.at(index.row());
    switch (role) {
    case rTitle: return script.title;
    case rDescription: return script.description;
    case rPath: return script.path.toQString();
    case rType: return typeToString(script.type);
    case rStatus: return ITestflow::statusToString(m_statuses.value(script.path, ITestflow::Status::Undefined));
    case rSelected: return m_selected.value(index.row(), true);
    }
    return QVariant();
}

int TestflowScriptsModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_scripts.size());
}

QHash<int, QByteArray> TestflowScriptsModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        { rTitle, "title" },
        { rDescription, "description" },
        { rType, "type" },
        { rPath, "path" },
        { rStatus, "status" },
        { rSelected, "selected" },
    };
    return roles;
}

void TestflowScriptsModel::load()
{
    testflow()->statusChanged().onReceive(this, [this](const io::path_t& path, const ITestflow::Status& status) {
        setStatus(path, status);

        if (status == ITestflow::Status::Error) {
            stopRunAllTC();
        }
    });

    testflow()->speedModeChanged().onReceive(this, [this](const SpeedMode&) {
        emit speedModeChanged();
    });

    beginResetModel();

    m_scripts = scriptsRepository()->scripts();
    std::sort(m_scripts.begin(), m_scripts.end(), [](const Script& f, const Script& s) {
        if (f.type != s.type) {
            return f.type < s.type;
        }
        return f.title < s.title;
    });

    //! NOTE Select all
    for (size_t i = 0; i < m_scripts.size(); ++i) {
        m_selected[int(i)] = true;
    }

    endResetModel();
}

void TestflowScriptsModel::setStatus(const io::path_t& path, ITestflow::Status st)
{
    m_statuses[path] = st;
    for (size_t i = 0; i < m_scripts.size(); ++i) {
        if (m_scripts.at(i).path == path) {
            emit dataChanged(index(int(i)), index(int(i)), { rStatus });
            break;
        }
    }
}

void TestflowScriptsModel::runScript(int scriptIndex)
{
    const Script& script = m_scripts.at(scriptIndex);
    testflow()->execScript(script.path);
}

void TestflowScriptsModel::runAllTC()
{
    //! NOTE Reset all statuses
    for (const Script& s : m_scripts) {
        setStatus(s.path, ITestflow::Status::Undefined);
    }

    m_currentTCIndex = -1;

    setIsRunAllTCMode(true);

    if (m_runInNewWindows) {
        runNextInNewWindows();
    } else {
        tryRunNextTC();
    }
}

int TestflowScriptsModel::takeNextTCIndex()
{
    const int count = static_cast<int>(m_scripts.size());
    while (++m_currentTCIndex < count) {
        if (m_scripts.at(m_currentTCIndex).type == ScriptType::TestCase && m_selected.value(m_currentTCIndex, true)) {
            return m_currentTCIndex;
        }
    }

    return -1;
}

bool TestflowScriptsModel::tryRunNextTC()
{
    if (!isRunAllTCMode()) {
        LOGD() << "not is run all TC mode";
        return false;
    }

    const int index = takeNextTCIndex();
    if (index < 0) {
        LOGD() << "no more TC scripts";
        return false;
    }

    const Script& tc = m_scripts.at(index);
    LOGD() << "requireStartTC: " << tc.path;
    emit requireStartTC(tc.path.toQString());

    return true;
}

void TestflowScriptsModel::runNextInNewWindows()
{
    while (isRunAllTCMode() && m_processes.size() < m_jobs) {
        const int index = takeNextTCIndex();
        if (index < 0) {
            break;
        }
        runInNewWindow(m_scripts.at(index).path);
    }

    if (m_processes.empty()) {
        setIsRunAllTCMode(false);
    }
}

void TestflowScriptsModel::runInNewWindow(const io::path_t& path)
{
    QStringList args = { "--test-case", path.toQString() };
    if (testflow()->speedMode() != SpeedMode::Undefined) {
        args << "--test-case-speed" << speedMode();
    }
    //! NOTE Overrides the default of the test case run (disabled)
    args << "--audio-output-disabled" << (m_audioDisabled ? "true" : "false");

    QProcess* process = new QProcess(this);
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_processes.append(process);

    connect(process, &QProcess::finished, this, [this, process, path](int exitCode, QProcess::ExitStatus exitStatus) {
        const bool passed = exitStatus == QProcess::NormalExit && exitCode == 0;
        onRunInNewWindowFinished(process, path, passed, QString("exit code: %1").arg(exitCode));
    });

    connect(process, &QProcess::errorOccurred, this, [this, process, path](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) {
            return;
        }

        QMetaObject::invokeMethod(this, [this, process, path]() {
            onRunInNewWindowFinished(process, path, false, "failed to start: " + process->errorString());
        }, Qt::QueuedConnection);
    });

    LOGI() << "[testflow] run in a new window: " << path;
    setStatus(path, ITestflow::Status::Running);
    process->start(QCoreApplication::applicationFilePath(), args);
}

void TestflowScriptsModel::onRunInNewWindowFinished(QProcess* process, const io::path_t& path, bool passed, const QString& info)
{
    LOGI() << "[testflow] " << (passed ? "PASSED" : "FAILED") << ": " << path << ", " << info;
    setStatus(path, passed ? ITestflow::Status::Finished : ITestflow::Status::Error);

    m_processes.removeOne(process);
    process->deleteLater();

    //! NOTE Unlike the run in this window, a failed test case does not stop the run of the next ones
    runNextInNewWindows();
}

void TestflowScriptsModel::stopRunAllTC()
{
    setIsRunAllTCMode(false);

    if (!m_processes.empty()) {
        for (QProcess* process : m_processes) {
            process->kill();
        }
        return;
    }

    testflow()->abort();
}

bool TestflowScriptsModel::isRunAllTCMode() const
{
    return m_isRunAllTCMode;
}

void TestflowScriptsModel::setIsRunAllTCMode(bool arg)
{
    if (m_isRunAllTCMode == arg) {
        return;
    }
    m_isRunAllTCMode = arg;
    emit isRunAllTCModeChanged();
}

void TestflowScriptsModel::toggleSelect(int idx)
{
    bool isOldAllSel = isAllSelected(m_scripts.at(size_t(idx)).type);
    m_selected[idx] = !m_selected.value(idx, true);
    emit dataChanged(index(idx), index(idx), { rSelected });

    bool isAllSel = isAllSelected(m_scripts.at(size_t(idx)).type);

    if (isOldAllSel != isAllSel) {
        emit isAllSelectedChanged(typeToString(m_scripts.at(size_t(idx)).type), isAllSel);
    }
}

void TestflowScriptsModel::toggleAllSelect(const QString& typeStr)
{
    ScriptType type = typeFromString(typeStr);
    bool isAllSel = isAllSelected(type);
    for (size_t i = 0; i < m_scripts.size(); ++i) {
        if (m_scripts.at(i).type != type) {
            continue;
        }

        m_selected[int(i)] = !isAllSel;
        emit dataChanged(index(int(i)), index(int(i)), { rSelected });
    }

    emit isAllSelectedChanged(typeStr, !isAllSel);
}

bool TestflowScriptsModel::isAllSelected(const QString& typeStr) const
{
    ScriptType type = typeFromString(typeStr);
    return isAllSelected(type);
}

bool TestflowScriptsModel::isAllSelected(const ScriptType& type) const
{
    for (size_t i = 0; i < m_scripts.size(); ++i) {
        if (m_scripts.at(i).type != type) {
            continue;
        }

        if (!m_selected.value(int(i), true)) {
            return false;
        }
    }

    return true;
}

QString TestflowScriptsModel::speedMode() const
{
    return speedModeToString(testflow()->speedMode());
}

void TestflowScriptsModel::setSpeedMode(const QString& newSpeedMode)
{
    testflow()->setSpeedMode(speedModeFromString(newSpeedMode));
}

bool TestflowScriptsModel::runInNewWindows() const
{
    return m_runInNewWindows;
}

void TestflowScriptsModel::setRunInNewWindows(bool arg)
{
    if (m_runInNewWindows == arg) {
        return;
    }
    m_runInNewWindows = arg;
    emit runInNewWindowsChanged();
}

int TestflowScriptsModel::jobs() const
{
    return m_jobs;
}

void TestflowScriptsModel::setJobs(int arg)
{
    if (m_jobs == arg || arg < 1) {
        return;
    }
    m_jobs = arg;
    emit jobsChanged();
}

bool TestflowScriptsModel::audioDisabled() const
{
    return m_audioDisabled;
}

void TestflowScriptsModel::setAudioDisabled(bool arg)
{
    if (m_audioDisabled == arg) {
        return;
    }
    m_audioDisabled = arg;
    emit audioDisabledChanged();
}
