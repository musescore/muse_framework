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

#include "types/val.h"
#include "translation.h"
#include "log.h"

using namespace muse;
using namespace muse::update;
using namespace muse::actions;
using namespace muse::async;

void AppUpdateScenario::delayedInit()
{
    const std::string installing = configuration()->installingReleaseVersion();
    if (installing.empty()) {
        return;
    }

    configuration()->setInstallingReleaseVersion(std::string());

    //! NOTE: The version differs if the user canceled the installer or it failed.
    bool hasCompletedUpdate = Version(installing) == application()->fullVersion();

    if (hasCompletedUpdate) {
        showUpdateCompletedToast();
    }
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
                showReleaseInfo(res.val, service()->isReleaseReadyToInstall());
            }
        } else if (!res.ret && !noUpdate) {
            LOGE() << res.ret.toString();
        }

        m_checkInProgress = false;

        if (!manual && res.ret && hasUpdate()) {
            if (configuration()->autoUpdateEnabled()) {
                downloadUpdateInBackground();
            } else {
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
    const int errorCode = error.code();
    if (errorCode == static_cast<int>(Ret::Code::Cancel)) {
        return async::make_promise<Ret>([](auto resolve, auto) {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        });
    }

    const auto unknownError = async::make_promise<Ret>([](auto resolve, auto) {
        return resolve(muse::make_ret(Ret::Code::UnknownError));
    });

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
    query.addParam("autoUpdateEnabled", Val(configuration()->autoUpdateEnabled()));

    return interactive()->open(query).then<Ret>(this, [this, info](const Val& val, auto resolve) {
        const std::string actionCode = applyReleaseInfoResult(val);
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

std::string AppUpdateScenario::applyReleaseInfoResult(const Val& result)
{
    const ValMap map = result.toMap();

    const auto autoUpdate = map.find("autoUpdateEnabled");
    if (autoUpdate != map.end()) {
        configuration()->setAutoUpdateEnabled(autoUpdate->second.toBool());
    }

    const auto action = map.find("action");
    return action != map.end() ? action->second.toString() : std::string();
}

void AppUpdateScenario::showUpdateCompletedToast()
{
    const std::string title = muse::qtrc("update", "Updated to %1 %2")
                              .arg(application()->title().toQString(), application()->fullVersion().toString().toQString())
                              .toStdString();

    toastService()->showWithTimeout(title, std::string(), std::chrono::seconds(10), muse::ui::IconCode::Code::TICK_FILLED);
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
                askToCloseAppAndCompleteInstall();
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
    if (!service()->isReleaseReadyToInstall()) {
        RetVal<Val> rv = interactive()->openSync("muse://update/app?mode=download");
        if (rv.ret.code() == static_cast<int>(Err::NotEnoughDiskSpace)) {
            return askToRetryOnNotEnoughDiskSpace(rv.ret, [this]() { return downloadRelease(); });
        }

        if (!rv.ret) {
            return processUpdateError(rv.ret);
        }
    }

    return askToCloseAppAndCompleteInstall();
}

Promise<Ret> AppUpdateScenario::askToCloseAppAndCompleteInstall()
{
    const std::string title = muse::trc("update", "Restart to finish updating");
    const std::string info = muse::qtrc("update", "%1 needs to close to complete the installation. "
                                                  "If you have any unsaved changes, you will be prompted to save them before %1 closes.")
                             .arg(application()->title().toQString()).toStdString();
    const int restartBtn = int(IInteractive::Button::Apply);
    const IInteractive::ButtonDatas buttons = {
        interactive()->buttonData(IInteractive::Button::Cancel),
        IInteractive::ButtonData(restartBtn, muse::trc("update", "Restart"), true)
    };

    return interactive()->info(title, info, buttons, restartBtn)
           .then<Ret>(this, [this](const IInteractive::Result& res, auto resolve) {
        if (res.isButton(IInteractive::Button::Cancel)) {
            return resolve(muse::make_ret(Ret::Code::Cancel));
        }

        io::path_t packagePath = service()->downloadedReleasePath();

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
    if (m_bgDownloadInProgress) {
        return;
    }

    if (!service()->isReleaseReadyToInstall() && networkInformation()->isMetered()) {
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

        //! NOTE: The release may have been skipped while the download was running.
        if (!hasUpdate()) {
            return;
        }

        if (res.ret) {
            showUpdateAvailableToast(service()->lastCheckResult().val, /*downloaded*/ true);
        }
    }, Asyncable::Mode::SetReplace);
}

void AppUpdateScenario::skipRelease(const std::string& version)
{
    configuration()->setSkippedReleaseVersion(version);
    service()->removeDownloadedRelease();
}
