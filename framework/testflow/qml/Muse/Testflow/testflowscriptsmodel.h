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

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QMap>
#include <QProcess>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "itestflow.h"
#include "itestflowscriptsrepository.h"

namespace muse::testflow {
class TestflowScriptsModel : public QAbstractListModel, public Contextable, public async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool isRunAllTCMode READ isRunAllTCMode WRITE setIsRunAllTCMode NOTIFY isRunAllTCModeChanged)
    Q_PROPERTY(QString speedMode READ speedMode WRITE setSpeedMode NOTIFY speedModeChanged)

    Q_PROPERTY(bool runInNewWindows READ runInNewWindows WRITE setRunInNewWindows NOTIFY runInNewWindowsChanged)
    Q_PROPERTY(int jobs READ jobs WRITE setJobs NOTIFY jobsChanged)
    Q_PROPERTY(bool audioDisabled READ audioDisabled WRITE setAudioDisabled NOTIFY audioDisabledChanged)

    QML_ELEMENT

    ContextInject<ITestflowScriptsRepository> scriptsRepository = { this };
    ContextInject<ITestflow> testflow = { this };

public:
    explicit TestflowScriptsModel(QObject* parent = nullptr);
    ~TestflowScriptsModel();

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isRunAllTCMode() const;
    QString speedMode() const;
    void setSpeedMode(const QString& newSpeedMode);

    bool runInNewWindows() const;
    void setRunInNewWindows(bool arg);
    int jobs() const;
    void setJobs(int arg);
    bool audioDisabled() const;
    void setAudioDisabled(bool arg);

    Q_INVOKABLE void load();
    Q_INVOKABLE void runScript(int scriptIndex);

    Q_INVOKABLE void runAllTC();
    Q_INVOKABLE bool tryRunNextTC();
    Q_INVOKABLE void stopRunAllTC();

    Q_INVOKABLE void toggleSelect(int index);
    Q_INVOKABLE void toggleAllSelect(const QString& type);
    Q_INVOKABLE bool isAllSelected(const QString& type) const;

public slots:
    void setIsRunAllTCMode(bool arg);

signals:
    void isRunAllTCModeChanged();
    void requireStartTC(const QString& path);
    void isAllSelectedChanged(const QString& type, bool arg);
    void speedModeChanged();
    void runInNewWindowsChanged();
    void jobsChanged();
    void audioDisabledChanged();

private:

    enum Roles {
        rTitle = Qt::UserRole + 1,
        rDescription,
        rType,
        rPath,
        rStatus,
        rSelected
    };

    void setStatus(const io::path_t& path, ITestflow::Status st);
    bool isAllSelected(const ScriptType& type) const;

    int takeNextTCIndex();
    void runNextInNewWindows();
    void runInNewWindow(const io::path_t& path);
    void onRunInNewWindowFinished(QProcess* process, const io::path_t& path, bool passed, const QString& info);

    Scripts m_scripts;
    int m_currentTCIndex = -1;
    bool m_isRunAllTCMode = false;
    bool m_runInNewWindows = false;
    int m_jobs = 1;
    bool m_audioDisabled = true;
    QList<QProcess*> m_processes;
    QMap<io::path_t, ITestflow::Status> m_statuses;
    QMap<int, bool> m_selected;
};
}
