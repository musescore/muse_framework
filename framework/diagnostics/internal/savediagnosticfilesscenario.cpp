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

#include "savediagnosticfilesscenario.h"

#include <QCursor>
#include <QGuiApplication>
#include <QTimer>

#include "diagnosticfileswriter.h"
#include "translation.h"

using namespace muse::diagnostics;
using namespace muse;

async::Promise<Ret> SaveDiagnosticFilesScenario::saveDiagnosticFiles()
{
    if (!configuration()->shouldWarnBeforeSavingDiagnosticFiles()) {
        return selectPathAndSave();
    }

    return async::make_promise<Ret>([this](auto resolve) {
        interactive()->warning(
            muse::trc("diagnostics", "Save diagnostic files?"),
            muse::qtrc("diagnostics", "This will create a .zip file with information about your %1 setup "
                                      "to help developers diagnose any problems you are having. "
                                      "You can inspect the contents of this file before sending it to anyone.")
            .arg(application()->title().toQString()).toStdString(),
            { IInteractive::Button::Cancel, IInteractive::Button::Save }, IInteractive::Button::Save,
            IInteractive::Option::WithIcon | IInteractive::Option::WithDontShowAgainCheckBox)
        .onResolve(this, [this, resolve](const IInteractive::Result& result) {
            if (result.standardButton() != IInteractive::Button::Save) {
                (void)resolve(make_ret(Ret::Code::Cancel));
                return;
            }

            configuration()->setShouldWarnBeforeSavingDiagnosticFiles(result.showAgain());

            selectPathAndSave().onResolve(this, [resolve](const Ret& ret) {
                (void)resolve(ret);
            });
        });

        return async::Promise<Ret>::dummy_result();
    }, async::PromiseType::AsyncByBody);
}

async::Promise<Ret> SaveDiagnosticFilesScenario::selectPathAndSave()
{
    return async::make_promise<Ret>([this](auto resolve) {
        interactive()->selectSavingFile(
            muse::trc("diagnostics", "Save diagnostic files"),
            configuration()->diagnosticFilesDefaultSavingPath(),
            { "(*.zip)" })
        .onResolve(this, [this, resolve](const io::path_t& path) {
            //! NOTE Writing processes the events, so it does not happen while the dialog is still closing
            QTimer::singleShot(0, &m_timersContext, [this, resolve, path]() {
                (void)resolve(saveTo(path));
            });
        })
        .onReject(this, [resolve](int code, const std::string& msg) {
            (void)resolve(make_ret(code, msg));
        });

        return async::Promise<Ret>::dummy_result();
    }, async::PromiseType::AsyncByBody);
}

Ret SaveDiagnosticFilesScenario::saveTo(const io::path_t& path)
{
    if (path.empty()) {
        return make_ret(Ret::Code::Cancel);
    }

    qApp->setOverrideCursor(Qt::WaitCursor);
    qApp->processEvents();

    Ret ret = DiagnosticFilesWriter(iocContext()).writeDiagnosticFiles(path);

    qApp->restoreOverrideCursor();

    if (!ret) {
        return ret;
    }

    platformInteractive()->revealInFileBrowser(path);

    return muse::make_ok();
}
