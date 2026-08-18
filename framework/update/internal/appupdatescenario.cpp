/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "appupdatescenario.h"

#include "updateerrors.h"

#include <QUrl>

#include "async/async.h"
#include "global/concurrency/concurrent.h"
#include "runtime.h"
#include "types/val.h"
#include "translation.h"
#include "log.h"

using namespace muse;
using namespace muse::update;
using namespace muse::actions;
using namespace muse::async;

bool AppUpdateScenario::needCheckForUpdate() const
{
    return configuration()->needCheckForUpdate();
}

void AppUpdateScenario::checkForUpdate(bool manual)
{
    if (m_checkInProgress) {
        return;
    }

    m_checkInProgress = true;

    service()->checkForUpdate().onResolve(this, [this, manual](const RetVal<ReleaseInfo>& res) {
        const bool noUpdate = res.ret.code() == static_cast<int>(Err::NoUpdate);

        if (manual) {
            if (noUpdate) {
                showNoUpdateMsg();
            } else if (!res.ret) {
                showServerErrorMsg();
            } else {
                showReleaseInfo(res.val);
            }
        } else if (!res.ret && !noUpdate) {
            LOGE() << res.ret.toString();
        }

        m_checkInProgress = false;

        if (!manual && res.ret) {
            downloadUpdateInBackground();
        }
    });
}

bool AppUpdateScenario::hasUpdate() const
{
    if (m_checkInProgress) {
        return false;
    }

    const RetVal<ReleaseInfo>& lastCheckResult = service()->lastCheckResult();
    if (!lastCheckResult.ret) {
        return false;
    }

    if (lastCheckResult.ret.code() == static_cast<int>(Err::NoUpdate)) {
        return false;
    }

    return !shouldIgnoreUpdate(lastCheckResult.val);
}

Promise<Ret> AppUpdateScenario::processUpdateError(int errorCode)
{
    const auto unknownError = async::make_promise<Ret>([](auto resolve, auto) {
        return resolve(muse::make_ret(Ret::Code::UnknownError));
    });

    IF_ASSERT_FAILED(errorCode >= static_cast<int>(Ret::Code::UpdateFirst)
                     && errorCode <= static_cast<int>(Ret::Code::UpdateLast)) {
        return unknownError;
    }

    const Err error = static_cast<Err>(errorCode);
    IF_ASSERT_FAILED(error != Err::NoError) {
        return unknownError;
    }

    auto message = error == Err::NoUpdate ? showNoUpdateMsg() : showServerErrorMsg();
    return message.then<Ret>(this, [errorCode](const IInteractive::Result&, auto resolve) {
        const Ret::Code code = static_cast<Ret::Code>(errorCode);
        return resolve(muse::make_ret(code));
    });
}

Promise<IInteractive::Result> AppUpdateScenario::showNoUpdateMsg()
{
    std::string webSiteUrl = configuration()->appWebSiteUrl();
    QUrl url(QString::fromStdString(webSiteUrl));
    const QString str = muse::qtrc("update", "You already have the latest version of %1. "
                                             "Please visit <a href=\"%2\">%3</a> for news on what’s coming next.")
                        .arg(application()->title().toQString(), QString::fromStdString(webSiteUrl), url.host());

    const IInteractive::Text text(str.toStdString(), IInteractive::TextFormat::RichText);
    const IInteractive::ButtonData okBtn = interactive()->buttonData(IInteractive::Button::Ok);

    return interactive()->info(muse::trc("update", "You’re up to date!"), text, { okBtn }, okBtn.btn,
                               IInteractive::Option::WithIcon);
}

Promise<Ret> AppUpdateScenario::showReleaseInfo(const ReleaseInfo& info)
{
    UriQuery query("muse://update/appreleaseinfo");
    query.addParam("appName", Val(application()->title().toStdString()));
    query.addParam("notes", Val(info.notes));
    query.addParam("previousReleasesNotes", Val(releasesNotesToValList(info.previousReleasesNotes)));

    return interactive()->open(query).then<Ret>(this, [this, info](const Val& val, auto resolve) {
        const QString actionCode = val.toQString();
        if (actionCode == "remindLater") {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        if (actionCode == "skip") {
            configuration()->setSkippedReleaseVersion(info.version);
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        //! NOTE: In test mode we skip the progress dialog and jump straight to the "needs to close" dialog...
        const bool testMode = configuration()->checkForUpdateTestMode();
        auto promise = testMode ? askToCloseAppAndCompleteInstall(/*installerPath*/ String()) : downloadRelease();
        promise.onResolve(this, [resolve](const Ret& ret) {
            (void)resolve(ret);
        });

        return Promise<Ret>::dummy_result();
    });
}

Promise<IInteractive::Result> AppUpdateScenario::showServerErrorMsg()
{
    return interactive()->error(muse::trc("update", "Cannot connect to server"),
                                muse::trc("update", "Sorry - please try again later"),
                                {}, int(IInteractive::Button::NoButton), { IInteractive::WithIcon },
                                muse::trc("update", "Check for update"));
}

Promise<Ret> AppUpdateScenario::downloadRelease()
{
    io::path_t packagePath = service()->downloadedReleasePath();

    if (packagePath.empty()) {
        RetVal<Val> rv = interactive()->openSync("muse://update/app?mode=download");
        if (!rv.ret) {
            return processUpdateError(rv.ret.code());
        }
        packagePath = rv.val.toString();
    }

    //! NOTE: In-place auto-install currently supports a single window only;
    //! otherwise fall back to handing the installer to the user.
    if (service()->canAutoInstall() && multiwindowsProvider()->windowCount() == 1) {
        return askToRestartAndInstall(packagePath);
    }

    return askToCloseAppAndCompleteInstall(packagePath);
}

Promise<Ret> AppUpdateScenario::askToRestartAndInstall(const io::path_t& packagePath)
{
    const std::string info = muse::qtrc("update", "%1 has downloaded an update and is ready to install it. "
                                                  "%1 will restart to complete the installation. "
                                                  "If you have any unsaved changes, you will be prompted to save them first.")
                             .arg(application()->title().toQString()).toStdString();
    const int restartBtn = int(IInteractive::Button::CustomButton) + 1;
    const IInteractive::ButtonDatas buttons = {
        interactive()->buttonData(IInteractive::Button::Cancel),
        IInteractive::ButtonData(restartBtn, muse::trc("update", "Restart"), true)
    };

    return interactive()->info("", info, buttons, restartBtn)
           .then<Ret>(this, [this, packagePath](const IInteractive::Result& res, auto resolve) {
        if (res.isButton(IInteractive::Button::Cancel)) {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        applyUpdateAndQuit(packagePath);

        return resolve(muse::make_ok());
    });
}

void AppUpdateScenario::applyUpdateAndQuit(const io::path_t& packagePath)
{
    //! NOTE: Unpacking and verifying the package takes seconds; run it off
    //! the UI thread so the app stays responsive.
    Concurrent::run([this, packagePath]() {
        const Ret ret = service()->applyUpdate(packagePath);
        async::Async::call(this, [this, packagePath, ret]() {
            if (!ret) {
                LOGE() << "failed to apply update in-place, falling back to manual install: " << ret.toString();
                askToCloseAppAndCompleteInstall(packagePath).onResolve(this, [](const Ret&) {});
                return;
            }

            //! NOTE: The helper has been spawned and will replace the app and
            //! relaunch once we quit. Quit without an installer path so the
            //! legacy "open installer" path is not taken.
            dispatcher()->dispatch("quit", ActionData::make_arg2<bool, std::string>(false, std::string()));
        }, runtime::mainThreadId());
    });
}

Promise<Ret> AppUpdateScenario::askToCloseAppAndCompleteInstall(const io::path_t& packagePath)
{
    const std::string info = muse::qtrc("update", "%1 needs to close to complete the installation. "
                                                  "If you have any unsaved changes, you will be prompted to save them before %1 closes.")
                             .arg(application()->title().toQString()).toStdString();
    const int closeBtn = int(IInteractive::Button::CustomButton) + 1;
    const IInteractive::ButtonDatas buttons = {
        interactive()->buttonData(IInteractive::Button::Cancel),
        IInteractive::ButtonData(closeBtn, muse::trc("update", "Close"), true)
    };

    return interactive()->info("", info, buttons, closeBtn)
           .then<Ret>(this, [this, packagePath](const IInteractive::Result& res, auto resolve) {
        if (res.isButton(IInteractive::Button::Cancel)) {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        if (multiwindowsProvider()->windowCount() != 1) {
            multiwindowsProvider()->quitAllAndRunInstallation(packagePath);
        }

        dispatcher()->dispatch("quit", ActionData::make_arg2<bool, std::string>(false, packagePath.toStdString()));
        return resolve(muse::make_ok());
    });
}

bool AppUpdateScenario::shouldIgnoreUpdate(const ReleaseInfo& info) const
{
    return info.version == configuration()->skippedReleaseVersion() && !configuration()->checkForUpdateTestMode();
}

void AppUpdateScenario::downloadUpdateInBackground()
{
    if (m_bgDownloadInProgress || hasReadyUpdate()) {
        return;
    }

    if (!hasUpdate() || !configuration()->autoInstallEnabled()) {
        return;
    }

    //! NOTE: This release was already downloaded in a previous session and is
    //! waiting to be installed - surface it without downloading again.
    if (service()->isReleaseDownloaded()) {
        m_readyPackagePath = service()->downloadedReleasePath();
        m_readyUpdateVersion = service()->lastCheckResult().val.version;
        m_hasReadyUpdateChanged.notify();
        return;
    }

    RetVal<Progress> progress = service()->downloadRelease();
    if (!progress.ret) {
        LOGE() << progress.ret.toString();
        return;
    }

    m_bgDownloadInProgress = true;

    progress.val.progressChanged().onReceive(this, [](int64_t current, int64_t total, const std::string& msg) {
        LOGE() << "progress: " << current << " / " << total << " " << msg;
    });

    progress.val.finished().onReceive(this, [this](const ProgressResult& res) {
        m_bgDownloadInProgress = false;

        if (!res.ret) {
            LOGE() << res.ret.toString();
            return;
        }

        m_readyPackagePath = res.val.toString();
        m_readyUpdateVersion = service()->lastCheckResult().val.version;
        m_hasReadyUpdateChanged.notify();
    });
}

bool AppUpdateScenario::hasReadyUpdate() const
{
    return !m_readyPackagePath.empty();
}

async::Notification AppUpdateScenario::hasReadyUpdateChanged() const
{
    return m_hasReadyUpdateChanged;
}

std::string AppUpdateScenario::readyUpdateVersion() const
{
    return m_readyUpdateVersion;
}

void AppUpdateScenario::installReadyUpdate()
{
    if (m_readyPackagePath.empty()) {
        return;
    }

    const ReleaseInfo& info = service()->lastCheckResult().val;

    UriQuery query("muse://update/appreleaseinfo");
    query.addParam("appName", Val(application()->title().toStdString()));
    query.addParam("notes", Val(info.notes));
    query.addParam("previousReleasesNotes", Val(releasesNotesToValList(info.previousReleasesNotes)));
    query.addParam("version", Val(m_readyUpdateVersion));
    query.addParam("readyToInstall", Val(true));

    interactive()->open(query).onResolve(this, [this](const Val& val) {
        const QString actionCode = val.toQString();

        if (actionCode == "skip") {
            configuration()->setSkippedReleaseVersion(m_readyUpdateVersion);
            m_readyPackagePath = io::path_t();
            m_hasReadyUpdateChanged.notify();
            return;
        }

        if (actionCode != "install") {
            return;
        }

        if (!service()->canAutoInstall() || multiwindowsProvider()->windowCount() != 1) {
            askToCloseAppAndCompleteInstall(m_readyPackagePath).onResolve(this, [](const Ret&) {});
            return;
        }

        applyUpdateAndQuit(m_readyPackagePath);
    });
}
