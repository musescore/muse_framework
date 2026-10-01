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

#include <QUrl>

#include "updateerrors.h"

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

void AppUpdateScenario::init()
{
    const std::string installing = configuration()->installingReleaseVersion();
    if (installing.empty()) {
        return;
    }

    configuration()->setInstallingReleaseVersion(std::string());

    //! NOTE: The version differs if the user canceled the installer or it failed.
    m_hasCompletedUpdate = Version(installing) == application()->fullVersion();
}

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
                showReleaseInfo(res.val, service()->isReleaseDownloaded());
            }
        } else if (!res.ret && !noUpdate) {
            LOGE() << res.ret.toString();
        }

        m_checkInProgress = false;

        if (!manual && res.ret) {
            if (configuration()->autoUpdateEnabled()) {
                downloadUpdateInBackground();
            } else if (hasUpdate()) {
                showUpdateAvailableToast(res.val, /*downloaded*/ false);
            }
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

Promise<Ret> AppUpdateScenario::processUpdateError(const Ret& error)
{
    const auto unknownError = async::make_promise<Ret>([](auto resolve, auto) {
        return resolve(muse::make_ret(Ret::Code::UnknownError));
    });

    const int errorCode = error.code();
    IF_ASSERT_FAILED(errorCode >= static_cast<int>(Ret::Code::UpdateFirst)
                     && errorCode <= static_cast<int>(Ret::Code::UpdateLast)) {
        return unknownError;
    }

    const Err err = static_cast<Err>(errorCode);
    IF_ASSERT_FAILED(err != Err::NoError) {
        return unknownError;
    }

    auto message = err == Err::NoUpdate ? showNoUpdateMsg() : showServerErrorMsg();
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

Promise<Ret> AppUpdateScenario::showReleaseInfo(const ReleaseInfo& info, bool readyToInstall)
{
    UriQuery query("muse://update/appreleaseinfo");
    query.addParam("appName", Val(application()->title().toStdString()));
    query.addParam("version", Val(info.version));
    query.addParam("notes", Val(info.notes));
    query.addParam("previousReleasesNotes", Val(releasesNotesToValList(info.previousReleasesNotes)));
    query.addParam("readyToInstall", Val(readyToInstall));

    return interactive()->open(query).then<Ret>(this, [this, info](const Val& val, auto resolve) {
        const QString actionCode = val.toQString();
        if (actionCode == "remindLater") {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        if (actionCode == "skip") {
            skipRelease(info.version);
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        downloadRelease().onResolve(this, [resolve](const Ret& ret) {
            (void)resolve(ret);
        });

        return Promise<Ret>::dummy_result();
    });
}

void AppUpdateScenario::showUpdateAvailableToast(const ReleaseInfo& info, bool downloaded)
{
    constexpr int seeDetailsBtn = int(toast::ToastActionCode::Custom) + 1;
    constexpr int installBtn = int(toast::ToastActionCode::Custom) + 2;

    const std::string msg = muse::qtrc("update", "%1 %2 is now ready to install.")
                            .arg(application()->title().toQString(), QString::fromStdString(info.version)).toStdString();

    toastService()->show(muse::trc("update", "New update available"), msg,
                         muse::ui::IconCode::Code::INFO_FILLED, true,
    {
        { muse::trc("update", "See details"), seeDetailsBtn },
        { downloaded ? muse::trc("update", "Restart & update") : muse::trc("update", "Install update"), installBtn, /*accent*/ true },
    }).onResolve(this, [this, info, downloaded](const toast::ToastResult& result) {
        if (result.isCode(seeDetailsBtn)) {
            showReleaseInfo(info, downloaded).onResolve(this, [](const Ret&) {});
        } else if (result.isCode(installBtn)) {
            if (downloaded) {
                installReadyUpdate();
            } else {
                downloadRelease().onResolve(this, [](const Ret&) {});
            }
        }
    });
}

Promise<IInteractive::Result> AppUpdateScenario::showServerErrorMsg()
{
    return interactive()->error(muse::trc("update", "Cannot connect to server"),
                                muse::trc("update", "Sorry - please try again later"),
                                {}, int(IInteractive::Button::NoButton), { IInteractive::WithIcon },
                                muse::trc("update", "Check for update"));
}

Promise<Ret> AppUpdateScenario::askToRetryOnNotEnoughDiskSpace(const Ret& error, const std::function<Promise<Ret>()>& retry)
{
    const IInteractive::ButtonDatas buttons = {
        interactive()->buttonData(IInteractive::Button::Cancel),
        interactive()->buttonData(IInteractive::Button::Retry)
    };

    return interactive()->error(muse::trc("update", "Not enough disk space"), error.text(),
                                buttons, int(IInteractive::Button::Retry), { IInteractive::WithIcon },
                                muse::trc("update", "Check for update"))
           .then<Ret>(this, [this, retry](const IInteractive::Result& res, auto resolve) {
        if (!res.isButton(IInteractive::Button::Retry)) {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        retry().onResolve(this, [resolve](const Ret& ret) {
            (void)resolve(ret);
        });

        return Promise<Ret>::dummy_result();
    });
}

Promise<Ret> AppUpdateScenario::downloadRelease()
{
    io::path_t packagePath = service()->downloadedReleasePath();

    if (packagePath.empty()) {
        RetVal<Val> rv = interactive()->openSync("muse://update/app?mode=download");
        if (rv.ret.code() == static_cast<int>(Err::NotEnoughDiskSpace)) {
            return askToRetryOnNotEnoughDiskSpace(rv.ret, [this]() { return downloadRelease(); });
        }

        if (!rv.ret) {
            return processUpdateError(rv.ret);
        }
        packagePath = rv.val.toString();
    }

    //! NOTE: In-place auto-install currently supports a single window only;
    //! otherwise fall back to handing the installer to the user.
    if (service()->canAutoInstall() && multiwindowsProvider()->windowCount() == 1) {
        return prepareAndInstall(packagePath);
    }

    return askToCloseAppAndCompleteInstall(packagePath);
}

Promise<Ret> AppUpdateScenario::prepareAndInstall(const io::path_t& packagePath)
{
    //! NOTE: The heavy phase (unpacking and verification) runs in the
    //! background while the app keeps running, so failures can still fall
    //! back to the manual flow; the confirmation dialog is shown once
    //! everything is staged, making the restart itself instant.
    return make_promise<Ret>([this, packagePath](auto resolve, auto) {
        auto service = this->service();

        Concurrent::run([this, service, packagePath, resolve]() {
            const RetVal<io::path_t> prepared = service->prepareUpdate(packagePath);
            async::Async::call(this, [this, packagePath, prepared, resolve]() {
                auto complete = [resolve](const Ret& ret) { (void)resolve(ret); };
                if (!prepared.ret) {
                    if (prepared.ret.code() == static_cast<int>(Err::NotEnoughDiskSpace)) {
                        askToRetryOnNotEnoughDiskSpace(prepared.ret, [this, packagePath]() {
                            return prepareAndInstall(packagePath);
                        }).onResolve(this, complete);
                        return;
                    }

                    LOGE() << "failed to prepare update, falling back to manual install: " << prepared.ret.toString();
                    askToCloseAppAndCompleteInstall(packagePath).onResolve(this, complete);
                    return;
                }

                askToRestartAndInstall(packagePath, prepared.val).onResolve(this, complete);
            }, runtime::mainThreadId());
        });

        return Promise<Ret>::dummy_result();
    });
}

Promise<Ret> AppUpdateScenario::askToRestartAndInstall(const io::path_t& packagePath, const io::path_t& preparedPath)
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
           .then<Ret>(this, [this, packagePath, preparedPath](const IInteractive::Result& res, auto resolve) {
        if (res.isButton(IInteractive::Button::Cancel)) {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        const Ret ret = service()->finalizeUpdate(preparedPath);
        if (!ret) {
            LOGE() << "failed to finalize update, falling back to manual install: " << ret.toString();
            askToCloseAppAndCompleteInstall(packagePath).onResolve(this, [resolve](const Ret& r) {
                (void)resolve(r);
            });
            return Promise<Ret>::dummy_result();
        }

        configuration()->setInstallingReleaseVersion(service()->lastCheckResult().val.version);

        //! NOTE: The helper has been spawned and will replace the app and
        //! relaunch once we quit. Quit without an installer path so the
        //! legacy "open installer" path is not taken.
        dispatcher()->dispatch("quit", ActionData::make_arg2<bool, std::string>(false, std::string()));
        return resolve(muse::make_ok());
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

        configuration()->setInstallingReleaseVersion(service()->lastCheckResult().val.version);

        if (multiwindowsProvider()->windowCount() != 1) {
            multiwindowsProvider()->quitAllAndRunInstallation(packagePath);
        }

        dispatcher()->dispatch("quit", ActionData::make_arg2<bool, std::string>(false, packagePath.toStdString()));
        return resolve(muse::make_ok());
    });
}

bool AppUpdateScenario::shouldIgnoreUpdate(const ReleaseInfo& info) const
{
    return info.version == configuration()->skippedReleaseVersion();
}

void AppUpdateScenario::downloadUpdateInBackground()
{
    if (m_bgDownloadInProgress || !m_readyPackagePath.empty()) {
        return;
    }

    if (!hasUpdate()) {
        return;
    }

    if (!configuration()->autoUpdateEnabled()) {
        LOGI() << "background update download skipped: user has disabled";
        return;
    }

    //! NOTE: This release was already downloaded in a previous session and is
    //! waiting to be installed - surface it without downloading again.
    if (service()->isReleaseDownloaded()) {
        m_readyPackagePath = service()->downloadedReleasePath();
        m_readyUpdateVersion = service()->lastCheckResult().val.version;
        m_readyUpdateDismissed = false;
        m_hasReadyUpdateChanged.notify();

        showUpdateAvailableToast(service()->lastCheckResult().val, /*downloaded*/ true);
        return;
    }

    if (networkInformation()->isMetered()) {
        LOGI() << "background update download skipped: metered network connection";
        return;
    }

    RetVal<Progress> progress = service()->downloadRelease();
    if (!progress.ret) {
        LOGE() << progress.ret.toString();
        return;
    }

    m_bgDownloadInProgress = true;

    progress.val.finished().onReceive(this, [this](const ProgressResult& res) {
        m_bgDownloadInProgress = false;

        if (!res.ret) {
            LOGE() << res.ret.toString();
            return;
        }

        //! NOTE: The release may have been skipped while the download was running.
        if (!hasUpdate()) {
            return;
        }

        m_readyPackagePath = res.val.toString();
        m_readyUpdateVersion = service()->lastCheckResult().val.version;
        m_readyUpdateDismissed = false;
        m_hasReadyUpdateChanged.notify();

        showUpdateAvailableToast(service()->lastCheckResult().val, /*downloaded*/ true);
    }, Asyncable::Mode::SetReplace);
}

void AppUpdateScenario::skipRelease(const std::string& version)
{
    configuration()->setSkippedReleaseVersion(version);
    service()->removeDownloadedRelease();

    m_readyPackagePath = io::path_t();
    m_hasReadyUpdateChanged.notify();
}

bool AppUpdateScenario::hasCompletedUpdate() const
{
    return m_hasCompletedUpdate;
}

async::Notification AppUpdateScenario::hasCompletedUpdateChanged() const
{
    return m_hasCompletedUpdateChanged;
}

void AppUpdateScenario::dismissCompletedUpdate()
{
    if (!m_hasCompletedUpdate) {
        return;
    }

    m_hasCompletedUpdate = false;
    m_hasCompletedUpdateChanged.notify();
}

bool AppUpdateScenario::hasReadyUpdate() const
{
    return !m_readyPackagePath.empty() && !m_readyUpdateDismissed;
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

    if (!service()->canAutoInstall() || multiwindowsProvider()->windowCount() != 1) {
        askToCloseAppAndCompleteInstall(m_readyPackagePath).onResolve(this, [](const Ret&) {});
        return;
    }

    prepareAndInstall(m_readyPackagePath).onResolve(this, [](const Ret&) {});
}

void AppUpdateScenario::showReadyUpdateInfo()
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
            skipRelease(m_readyUpdateVersion);
            return;
        }

        if (actionCode == "install") {
            installReadyUpdate();
        }
    });
}

void AppUpdateScenario::dismissReadyUpdate()
{
    if (m_readyPackagePath.empty() || m_readyUpdateDismissed) {
        return;
    }

    m_readyUpdateDismissed = true;
    m_hasReadyUpdateChanged.notify();
}
