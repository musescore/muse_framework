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
#include <gmock/gmock.h>

#include <optional>

#include <QThreadPool>

#include "global/tests/mocks/applicationmock.h"
#include "network/tests/mocks/networkinformationmock.h"
#include "interactive/tests/mocks/interactivemock.h"
#include "multiwindows/tests/mocks/multiwindowsprovidermock.h"
#include "toast/tests/mocks/toastservicemock.h"
#include "mocks/updateconfigurationmock.h"
#include "mocks/appupdateservicemock.h"

#include "async/processevents.h"

#include "update/internal/appupdatescenario.h"
#include "update/updateerrors.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using namespace muse;
using namespace muse::update;

namespace muse::update {
class AppUpdateScenarioTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_scenario = new AppUpdateScenario(modularity::globalCtx());

        m_configuration = std::make_shared<NiceMock<UpdateConfigurationMock> >();
        m_scenario->configuration.set(m_configuration);

        m_service = std::make_shared<NiceMock<AppUpdateServiceMock> >();
        m_scenario->service.set(m_service);

        m_networkInformation = std::make_shared<NiceMock<network::NetworkInformationMock> >();
        m_scenario->networkInformation.set(m_networkInformation);

        m_interactive = std::make_shared<NiceMock<InteractiveMock> >();
        m_scenario->interactive.set(m_interactive);

        ON_CALL(*m_interactive, buttonData(_))
        .WillByDefault(Invoke([](IInteractive::Button btn) {
            return IInteractive::ButtonData(btn, std::string());
        }));

        m_multiwindowsProvider = std::make_shared<NiceMock<mi::MultiWindowsProviderMock> >();
        m_scenario->multiwindowsProvider.set(m_multiwindowsProvider);

        m_toastService = std::make_shared<NiceMock<toast::ToastServiceMock> >();
        m_scenario->toastService.set(m_toastService);

        m_application = std::make_shared<NiceMock<ApplicationMock> >();
        m_scenario->application.set(m_application);

        ON_CALL(*m_application, fullVersion())
        .WillByDefault(Return(Version(CURRENT_VERSION)));
        ON_CALL(*m_application, title())
        .WillByDefault(Return(String(u"App")));

        //! [GIVEN] An update is available and automatic download is enabled
        ReleaseInfo info;
        info.version = "1000.0";
        m_lastCheckResult = RetVal<ReleaseInfo>::make_ok(info);

        ON_CALL(*m_service, lastCheckResult())
        .WillByDefault(ReturnRef(m_lastCheckResult));

        ON_CALL(*m_service, isReleaseDownloaded())
        .WillByDefault(Return(false));

        ON_CALL(*m_configuration, autoUpdateEnabled())
        .WillByDefault(Return(true));
    }

    void TearDown() override
    {
        delete m_scenario;
    }

    void downloadUpdateInBackground()
    {
        m_scenario->downloadUpdateInBackground();
    }

    void init()
    {
        m_scenario->init();
    }

    void skipRelease(const std::string& version)
    {
        m_scenario->skipRelease(version);
    }

    async::Promise<Ret> downloadRelease()
    {
        return m_scenario->downloadRelease();
    }

    async::Promise<Ret> prepareAndInstall(const io::path_t& packagePath)
    {
        return m_scenario->prepareAndInstall(packagePath);
    }

    //! A dialog action: the promise is created at call time (so the caller can
    //! subscribe to it) and resolves with the given button once messages are processed
    static auto dialog(IInteractive::Button btn)
    {
        return InvokeWithoutArgs([btn]() {
            return async::make_promise<IInteractive::Result>([btn](auto resolve) {
                return resolve(IInteractive::Result(static_cast<int>(btn)));
            });
        });
    }

    //! An "update available" toast: checks its content and buttons, then resolves
    //! (once messages are processed) as if the button at the given index was clicked,
    //! or as dismissed when no button is clicked
    static auto toast(const std::vector<std::string>& expectedButtons, int clickedButton = NO_CLICK)
    {
        return Invoke([expectedButtons, clickedButton](const std::string& title, const std::string& message,
                                                       ui::IconCode::Code iconCode, bool dismissible,
                                                       const std::vector<toast::ToastAction>& actions) {
            EXPECT_EQ(title, "New update available");
            EXPECT_EQ(message, "App 1000.0 is now ready to install.");
            EXPECT_EQ(iconCode, ui::IconCode::Code::INFO_FILLED);
            EXPECT_TRUE(dismissible);

            std::vector<std::string> buttons;
            for (const toast::ToastAction& action : actions) {
                buttons.push_back(action.text);
            }
            EXPECT_EQ(buttons, expectedButtons);
            EXPECT_TRUE(!actions.empty() && actions.back().accent);

            int code = static_cast<int>(toast::ToastActionCode::Dismiss);
            if (clickedButton != NO_CLICK && clickedButton < static_cast<int>(actions.size())) {
                code = actions.at(clickedButton).code;
            }

            return async::make_promise<toast::ToastResult>([code](auto resolve) {
                return resolve(toast::ToastResult(code));
            });
        });
    }

    static auto checkForUpdateResolves(const RetVal<ReleaseInfo>& result)
    {
        return InvokeWithoutArgs([result]() {
            return async::make_promise<RetVal<ReleaseInfo> >([result](auto resolve) {
                return resolve(result);
            });
        });
    }

    //! The release info dialog: checks the query, then resolves with the given action
    //! ("install", "skip", "remindLater") and the auto-update toggle state once messages are processed
    static auto releaseInfoDialog(bool expectedReadyToInstall, const std::string& action,
                                  std::optional<bool> autoUpdateEnabled = std::nullopt)
    {
        return Invoke([expectedReadyToInstall, action, autoUpdateEnabled](const UriQuery& query) {
            EXPECT_EQ(query.uri(), Uri("muse://update/appreleaseinfo"));
            EXPECT_EQ(query.param("appName").toString(), "App");
            EXPECT_EQ(query.param("version").toString(), "1000.0");
            EXPECT_EQ(query.param("readyToInstall").toBool(), expectedReadyToInstall);

            //! NOTE: The toggle keeps its initial value unless the test says otherwise
            const bool toggle = autoUpdateEnabled.value_or(query.param("autoUpdateEnabled").toBool());
            const Val result(ValMap {
                { "action", Val(action) },
                { "autoUpdateEnabled", Val(toggle) },
            });

            return async::make_promise<Val>([result](auto resolve) {
                return resolve(result);
            });
        });
    }

    static RetVal<Val> notEnoughDiskSpace()
    {
        return RetVal<Val>(make_ret(Err::NotEnoughDiskSpace, "Free up 250 MB"));
    }

    //! Drain background work and queued async calls
    static void pump()
    {
        for (int i = 0; i < 10; ++i) {
            QThreadPool::globalInstance()->waitForDone();
            async::processMessages();
        }
    }

    static constexpr const char* CURRENT_VERSION = "4.0.0";

    static constexpr int NO_CLICK = -1;
    static constexpr int SEE_DETAILS = 0;
    static constexpr int INSTALL = 1;

    inline static const std::vector<std::string> AVAILABLE_TOAST = { "See details", "Install update" };
    inline static const std::vector<std::string> DOWNLOADED_TOAST = { "See details", "Restart & update" };

    AppUpdateScenario* m_scenario = nullptr;
    std::shared_ptr<ApplicationMock> m_application;
    std::shared_ptr<UpdateConfigurationMock> m_configuration;
    std::shared_ptr<AppUpdateServiceMock> m_service;
    std::shared_ptr<network::NetworkInformationMock> m_networkInformation;
    std::shared_ptr<InteractiveMock> m_interactive;
    std::shared_ptr<mi::MultiWindowsProviderMock> m_multiwindowsProvider;
    std::shared_ptr<toast::ToastServiceMock> m_toastService;
    RetVal<ReleaseInfo> m_lastCheckResult;
    Progress m_downloadProgress;
};
}

TEST_F(AppUpdateScenarioTests, BgDownload_UnmeteredNetwork_StartsDownload)
{
    //! [GIVEN] The network connection is not metered
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));

    //! [THEN] The download is started
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));

    //! [WHEN] A background download is requested
    downloadUpdateInBackground();

    //! [THEN] The "ready to install" toast is shown once the download finishes
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST));

    //! [WHEN] The download finishes successfully
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));

    //! [THEN] The update is surfaced as ready to install
    EXPECT_TRUE(m_scenario->hasReadyUpdate());
    EXPECT_EQ(m_scenario->readyUpdateVersion(), "1000.0");
}

TEST_F(AppUpdateScenarioTests, BgDownload_AutoUpdateDisabled_SkipsDownload)
{
    //! [GIVEN] The user turned automatic download off
    ON_CALL(*m_configuration, autoUpdateEnabled())
    .WillByDefault(Return(false));
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));

    //! [THEN] No download is started
    EXPECT_CALL(*m_service, downloadRelease())
    .Times(0);

    //! [WHEN] A background download is requested
    downloadUpdateInBackground();

    EXPECT_FALSE(m_scenario->hasReadyUpdate());
}

TEST_F(AppUpdateScenarioTests, BgDownload_MeteredNetwork_SkipsDownload)
{
    //! [GIVEN] The network connection is metered
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(true));

    //! [THEN] No download is started
    EXPECT_CALL(*m_service, downloadRelease())
    .Times(0);

    //! [WHEN] A background download is requested
    downloadUpdateInBackground();

    //! [THEN] No update is surfaced as ready
    EXPECT_FALSE(m_scenario->hasReadyUpdate());
}

TEST_F(AppUpdateScenarioTests, BgDownload_MeteredThenUnmetered_DownloadsOnRetry)
{
    //! [GIVEN] The network connection is metered at first, unmetered later
    EXPECT_CALL(*m_networkInformation, isMetered())
    .WillOnce(Return(true))
    .WillOnce(Return(false));

    //! [THEN] Only the second request starts a download
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));

    //! [WHEN] A background download is requested on the metered network,
    //! then again after the network became unmetered
    downloadUpdateInBackground();
    downloadUpdateInBackground();
}

TEST_F(AppUpdateScenarioTests, BgDownload_AlreadyDownloaded_SurfacedEvenOnMetered)
{
    //! [GIVEN] The release was already downloaded in a previous session
    ON_CALL(*m_service, isReleaseDownloaded())
    .WillByDefault(Return(true));
    ON_CALL(*m_service, downloadedReleasePath())
    .WillByDefault(Return(io::path_t("upd/MuseScore.dmg")));

    //! [GIVEN] The network connection is metered
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(true));

    //! [THEN] No download is started
    EXPECT_CALL(*m_service, downloadRelease())
    .Times(0);

    //! [THEN] The "ready to install" toast is shown
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST));

    //! [WHEN] A background download is requested
    downloadUpdateInBackground();

    //! [THEN] The downloaded update is still surfaced as ready to install
    EXPECT_TRUE(m_scenario->hasReadyUpdate());
    EXPECT_EQ(m_scenario->readyUpdateVersion(), "1000.0");
}

TEST_F(AppUpdateScenarioTests, BgDownload_NotEnoughDiskSpace_SkipsSilently)
{
    //! [GIVEN] The network connection is not metered
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));

    //! [GIVEN] The service refuses to download because the disk is full
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ret(make_ret(Err::NotEnoughDiskSpace))));

    //! [WHEN] A background download is requested
    downloadUpdateInBackground();

    //! [THEN] No update is surfaced as ready and a later retry is allowed
    EXPECT_FALSE(m_scenario->hasReadyUpdate());

    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));
    downloadUpdateInBackground();
}

TEST_F(AppUpdateScenarioTests, ManualDownload_NotEnoughDiskSpace_Cancel_Stops)
{
    //! [GIVEN] The download dialog refuses to start because the disk is full
    EXPECT_CALL(*m_interactive, openSync(_))
    .WillOnce(Return(notEnoughDiskSpace()));

    //! [THEN] An error with the details from the service is shown, and the user cancels
    EXPECT_CALL(*m_interactive, error(_, _, _, _, _, _))
    .WillOnce(Invoke([](const std::string&, const IInteractive::Text& text, const IInteractive::ButtonDatas&,
                        int, const IInteractive::Options&, const std::string&) {
        EXPECT_EQ(text.text, "Free up 250 MB");
        return async::make_promise<IInteractive::Result>([](auto resolve) {
            return resolve(IInteractive::Result(static_cast<int>(IInteractive::Button::Cancel)));
        });
    }));

    //! [WHEN] A manual download is requested
    Ret result;
    downloadRelease().onResolve(m_scenario, [&result](const Ret& ret) { result = ret; });
    pump();

    //! [THEN] The flow ends with Cancel
    EXPECT_EQ(result.code(), static_cast<int>(Ret::Code::Cancel));
}

TEST_F(AppUpdateScenarioTests, ManualDownload_NotEnoughDiskSpace_Retry_ReopensDownload)
{
    //! [GIVEN] The disk is still full on the second attempt
    EXPECT_CALL(*m_interactive, openSync(_))
    .Times(2)
    .WillRepeatedly(Return(notEnoughDiskSpace()));

    //! [THEN] The user retries once, then cancels
    EXPECT_CALL(*m_interactive, error(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Retry))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] A manual download is requested
    Ret result;
    downloadRelease().onResolve(m_scenario, [&result](const Ret& ret) { result = ret; });
    pump();

    EXPECT_EQ(result.code(), static_cast<int>(Ret::Code::Cancel));
}

TEST_F(AppUpdateScenarioTests, ManualDownload_NotEnoughDiskSpace_RetrySucceeds_ProceedsToInstall)
{
    //! [GIVEN] Space was freed up before the retry, so the second attempt downloads the package
    EXPECT_CALL(*m_interactive, openSync(_))
    .WillOnce(Return(notEnoughDiskSpace()))
    .WillOnce(Return(RetVal<Val>::make_ok(Val(std::string("upd/MuseScore.dmg")))));

    EXPECT_CALL(*m_interactive, error(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Retry));

    //! [GIVEN] Auto-install is not available, so the manual install prompt follows
    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));

    //! [THEN] The "close to complete installation" prompt is shown
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] A manual download is requested
    Ret result;
    downloadRelease().onResolve(m_scenario, [&result](const Ret& ret) { result = ret; });
    pump();

    EXPECT_EQ(result.code(), static_cast<int>(Ret::Code::Cancel));
}

TEST_F(AppUpdateScenarioTests, PrepareAndInstall_NotEnoughDiskSpace_Retry_PreparesAgain)
{
    //! [GIVEN] Staging fails for lack of space the first time and succeeds after a retry
    const io::path_t package("upd/MuseScore.dmg");
    EXPECT_CALL(*m_service, prepareUpdate(package))
    .WillOnce(Return(RetVal<io::path_t>(make_ret(Err::NotEnoughDiskSpace, "Free up 250 MB"))))
    .WillOnce(Return(RetVal<io::path_t>::make_ok(io::path_t("upd/staging/MuseScore.app"))));

    EXPECT_CALL(*m_interactive, error(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Retry));

    //! [THEN] The restart prompt is shown; no fallback to the manual install prompt
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] The downloaded package is installed
    Ret result;
    prepareAndInstall(package).onResolve(m_scenario, [&result](const Ret& ret) { result = ret; });
    pump();

    EXPECT_EQ(result.code(), static_cast<int>(Ret::Code::Cancel));
}

TEST_F(AppUpdateScenarioTests, SkipRelease_RemovesPackage_AndClearsReadyUpdate)
{
    //! [GIVEN] The release was downloaded in the background and is ready to install
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));

    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST));

    downloadUpdateInBackground();
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));
    ASSERT_TRUE(m_scenario->hasReadyUpdate());

    //! [THEN] The version is remembered as skipped and the package is deleted
    EXPECT_CALL(*m_configuration, setSkippedReleaseVersion("1000.0"));
    EXPECT_CALL(*m_service, removeDownloadedRelease());

    //! [WHEN] The user skips the release
    skipRelease("1000.0");

    //! [THEN] Nothing is left to install
    EXPECT_FALSE(m_scenario->hasReadyUpdate());
}

TEST_F(AppUpdateScenarioTests, SkipRelease_WhileDownloading_DoesNotSurfaceUpdate)
{
    //! [GIVEN] A background download is running
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));

    downloadUpdateInBackground();

    //! [WHEN] The user skips the release before the download finishes
    ON_CALL(*m_configuration, skippedReleaseVersion())
    .WillByDefault(Return("1000.0"));
    skipRelease("1000.0");

    //! [THEN] No toast is shown for the skipped release
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .Times(0);

    //! [WHEN] The (not yet canceled) download still reports success
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));

    //! [THEN] The skipped release is not surfaced as ready to install
    EXPECT_FALSE(m_scenario->hasReadyUpdate());
}

TEST_F(AppUpdateScenarioTests, Init_LaunchedWithInstalledVersion_ReportsCompletedUpdate)
{
    //! [GIVEN] The app quit to install this very version and is now running it
    ON_CALL(*m_configuration, installingReleaseVersion())
    .WillByDefault(Return(CURRENT_VERSION));

    //! [THEN] The record is cleared so the banner shows only once
    EXPECT_CALL(*m_configuration, setInstallingReleaseVersion(""));

    //! [WHEN] The scenario starts
    init();

    //! [THEN] The update is reported as completed until dismissed
    EXPECT_TRUE(m_scenario->hasCompletedUpdate());

    m_scenario->dismissCompletedUpdate();
    EXPECT_FALSE(m_scenario->hasCompletedUpdate());
}

TEST_F(AppUpdateScenarioTests, Init_InstallDidNotHappen_NoCompletedUpdate)
{
    //! [GIVEN] The app quit to install a version, but still runs the old one
    ON_CALL(*m_configuration, installingReleaseVersion())
    .WillByDefault(Return("1000.0"));

    //! [THEN] The record is still cleared
    EXPECT_CALL(*m_configuration, setInstallingReleaseVersion(""));

    //! [WHEN] The scenario starts
    init();

    //! [THEN] Nothing is reported
    EXPECT_FALSE(m_scenario->hasCompletedUpdate());
}

TEST_F(AppUpdateScenarioTests, Init_NothingWasInstalling_NoCompletedUpdate)
{
    //! [GIVEN] A regular launch
    ON_CALL(*m_configuration, installingReleaseVersion())
    .WillByDefault(Return(""));

    EXPECT_CALL(*m_configuration, setInstallingReleaseVersion(_))
    .Times(0);

    //! [WHEN] The scenario starts
    init();

    EXPECT_FALSE(m_scenario->hasCompletedUpdate());
}

TEST_F(AppUpdateScenarioTests, DismissReadyUpdate_HidesBanner_KeepsPackage)
{
    //! [GIVEN] A ready update
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));

    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST));

    downloadUpdateInBackground();
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));
    ASSERT_TRUE(m_scenario->hasReadyUpdate());

    //! [THEN] The package is not removed
    EXPECT_CALL(*m_service, removeDownloadedRelease())
    .Times(0);

    //! [WHEN] The user closes the banner
    m_scenario->dismissReadyUpdate();

    //! [THEN] The banner is hidden, but the update can still be installed
    EXPECT_FALSE(m_scenario->hasReadyUpdate());

    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    m_scenario->installReadyUpdate();
    pump();
}

TEST_F(AppUpdateScenarioTests, InstallReadyUpdate_GoesStraightToInstall)
{
    //! [GIVEN] A ready update and no in-place install support
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));

    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST));

    downloadUpdateInBackground();
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));

    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));

    //! [THEN] No release info dialog is opened; the install prompt follows directly
    EXPECT_CALL(*m_interactive, open(_))
    .Times(0);
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] The user chooses "Restart and update"
    m_scenario->installReadyUpdate();
    pump();
}

TEST_F(AppUpdateScenarioTests, BgDownload_Finished_RestartAndUpdate_Installs)
{
    //! [GIVEN] A background download is running and in-place install is not available
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));
    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));

    downloadUpdateInBackground();

    //! [GIVEN] The user clicks "Restart & update" on the toast
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST, INSTALL));

    //! [THEN] The downloaded package is installed: no new download, no release info dialog
    EXPECT_CALL(*m_interactive, openSync(_))
    .Times(0);
    EXPECT_CALL(*m_interactive, open(_))
    .Times(0);
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] The download finishes
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));
    pump();
}

TEST_F(AppUpdateScenarioTests, BgDownload_Finished_SeeDetails_InstallUsesDownloadedPackage)
{
    //! [GIVEN] A background download is running and in-place install is not available
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));
    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));

    downloadUpdateInBackground();

    //! [GIVEN] The user clicks "See details" on the toast
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST, SEE_DETAILS));

    //! [THEN] The release info is opened in the "ready to install" mode, and the user clicks "Restart & update"
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ true, "install"));

    //! [THEN] The downloaded package is installed without downloading it again
    ON_CALL(*m_service, downloadedReleasePath())
    .WillByDefault(Return(io::path_t("upd/MuseScore.dmg")));
    EXPECT_CALL(*m_interactive, openSync(_))
    .Times(0);
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] The download finishes
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));
    pump();
}

TEST_F(AppUpdateScenarioTests, AutoCheck_AutoUpdateDisabled_InstallUpdate_Downloads)
{
    //! [GIVEN] The user turned automatic update off
    ON_CALL(*m_configuration, autoUpdateEnabled())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [THEN] No background download is started
    EXPECT_CALL(*m_service, downloadRelease())
    .Times(0);

    //! [THEN] The "update available" toast is shown, and the user clicks "Install update"
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(AVAILABLE_TOAST, INSTALL));

    //! [THEN] The download dialog is opened, followed by the install prompt
    EXPECT_CALL(*m_interactive, openSync(_))
    .WillOnce(Invoke([](const UriQuery& query) {
        EXPECT_EQ(query.uri(), Uri("muse://update/app"));
        EXPECT_EQ(query.param("mode").toString(), "download");
        return RetVal<Val>::make_ok(Val(std::string("upd/MuseScore.dmg")));
    }));
    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] An automatic check finds an update
    m_scenario->checkForUpdate(/*manual*/ false);
    pump();
}

TEST_F(AppUpdateScenarioTests, AutoCheck_AutoUpdateDisabled_SeeDetails_OpensReleaseInfo)
{
    //! [GIVEN] The user turned automatic update off
    ON_CALL(*m_configuration, autoUpdateEnabled())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [THEN] The "update available" toast is shown, and the user clicks "See details"
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(AVAILABLE_TOAST, SEE_DETAILS));

    //! [THEN] The release info is opened (not in the "ready to install" mode); the user postpones
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ false, "remindLater"));

    //! [THEN] Nothing is downloaded
    EXPECT_CALL(*m_interactive, openSync(_))
    .Times(0);

    //! [WHEN] An automatic check finds an update
    m_scenario->checkForUpdate(/*manual*/ false);
    pump();
}

TEST_F(AppUpdateScenarioTests, AutoCheck_AutoUpdateDisabled_SkippedRelease_NoToast)
{
    //! [GIVEN] The user turned automatic update off and skipped the available release
    ON_CALL(*m_configuration, autoUpdateEnabled())
    .WillByDefault(Return(false));
    ON_CALL(*m_configuration, skippedReleaseVersion())
    .WillByDefault(Return("1000.0"));
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [THEN] No toast is shown
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .Times(0);

    //! [WHEN] An automatic check finds the skipped release
    m_scenario->checkForUpdate(/*manual*/ false);
    pump();
}

TEST_F(AppUpdateScenarioTests, ManualCheck_ReleaseAlreadyDownloaded_OpensReadyToInstallInfo)
{
    //! [GIVEN] The available release was already downloaded
    ON_CALL(*m_service, isReleaseDownloaded())
    .WillByDefault(Return(true));
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [THEN] No toast is shown; the release info is opened in the "ready to install" mode
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .Times(0);
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ true, "remindLater"));

    //! [WHEN] The user checks for updates manually
    m_scenario->checkForUpdate(/*manual*/ true);
    pump();
}

TEST_F(AppUpdateScenarioTests, ManualCheck_ReleaseNotDownloaded_OpensReleaseInfo)
{
    //! [GIVEN] The available release is not downloaded yet
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [THEN] No toast is shown; the release info is opened in the regular mode
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .Times(0);
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ false, "remindLater"));

    //! [WHEN] The user checks for updates manually
    m_scenario->checkForUpdate(/*manual*/ true);
    pump();
}

TEST_F(AppUpdateScenarioTests, ReleaseInfo_PassesCurrentAutoUpdateSetting)
{
    //! [GIVEN] Automatic update is off
    ON_CALL(*m_configuration, autoUpdateEnabled())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [THEN] The dialog gets the current setting for its toggle
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(Invoke([](const UriQuery& query) {
        EXPECT_FALSE(query.param("autoUpdateEnabled").toBool());
        return async::make_promise<Val>([](auto resolve) {
            return resolve(Val(ValMap { { "action", Val(std::string("remindLater")) } }));
        });
    }));

    //! [THEN] A result without the toggle state does not touch the setting
    EXPECT_CALL(*m_configuration, setAutoUpdateEnabled(_))
    .Times(0);

    //! [WHEN] The user checks for updates manually
    m_scenario->checkForUpdate(/*manual*/ true);
    pump();
}

TEST_F(AppUpdateScenarioTests, ReleaseInfo_RemindLater_AppliesAutoUpdateToggle)
{
    //! [GIVEN] Automatic update is off
    ON_CALL(*m_configuration, autoUpdateEnabled())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [GIVEN] The user turns the toggle on and clicks "Remind me later"
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ false, "remindLater", /*autoUpdateEnabled*/ true));

    //! [THEN] The setting is saved, and nothing is downloaded
    EXPECT_CALL(*m_configuration, setAutoUpdateEnabled(true));
    EXPECT_CALL(*m_interactive, openSync(_))
    .Times(0);

    //! [WHEN] The user checks for updates manually
    m_scenario->checkForUpdate(/*manual*/ true);
    pump();
}

TEST_F(AppUpdateScenarioTests, ReleaseInfo_Skip_AppliesAutoUpdateToggle)
{
    //! [GIVEN] Automatic update is on
    EXPECT_CALL(*m_service, checkForUpdate())
    .WillOnce(checkForUpdateResolves(m_lastCheckResult));

    //! [GIVEN] The user turns the toggle off and skips the release
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ false, "skip", /*autoUpdateEnabled*/ false));

    //! [THEN] The setting is saved and the release is skipped
    EXPECT_CALL(*m_configuration, setAutoUpdateEnabled(false));
    EXPECT_CALL(*m_configuration, setSkippedReleaseVersion("1000.0"));

    //! [WHEN] The user checks for updates manually
    m_scenario->checkForUpdate(/*manual*/ true);
    pump();
}

TEST_F(AppUpdateScenarioTests, ReadyUpdateInfo_Install_AppliesAutoUpdateToggle)
{
    //! [GIVEN] A ready update and no in-place install support
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(false));
    EXPECT_CALL(*m_service, downloadRelease())
    .WillOnce(Return(RetVal<Progress>::make_ok(m_downloadProgress)));
    EXPECT_CALL(*m_toastService, show(_, _, _, _, _))
    .WillOnce(toast(DOWNLOADED_TOAST));

    downloadUpdateInBackground();
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("upd/MuseScore.dmg"))));

    ON_CALL(*m_service, canAutoInstall())
    .WillByDefault(Return(false));

    //! [GIVEN] The user turns the toggle off and clicks "Restart & update"
    EXPECT_CALL(*m_interactive, open(_))
    .WillOnce(releaseInfoDialog(/*readyToInstall*/ true, "install", /*autoUpdateEnabled*/ false));

    //! [THEN] The setting is saved and the install prompt follows
    EXPECT_CALL(*m_configuration, setAutoUpdateEnabled(false));
    EXPECT_CALL(*m_interactive, info(_, _, _, _, _, _))
    .WillOnce(dialog(IInteractive::Button::Cancel));

    //! [WHEN] The user opens the ready update details from the banner
    m_scenario->showReadyUpdateInfo();
    pump();
}
