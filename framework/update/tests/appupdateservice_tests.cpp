/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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

#include <QIODevice>
#include <QThreadPool>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

#include "framework/network/networktypes.h"

#include "global/tests/mocks/systeminfomock.h"
#include "global/tests/mocks/filesystemmock.h"
#include "global/tests/mocks/applicationmock.h"
#include "network/tests/mocks/networkmanagercreatormock.h"
#include "network/tests/mocks/networkmanagermock.h"
#include "ui/tests/mocks/uiconfigurationmock.h"
#include "mocks/updateconfigurationmock.h"
#include "mocks/updateinstallermock.h"

#include "update/internal/appupdateservice.h"
#include "update/updateerrors.h"

#include "async/processevents.h"

using namespace muse;
using namespace muse::update;
using namespace muse::async;
using namespace muse::network;

namespace muse::update {
class AppUpdateServiceTests : public ::testing::Test, public ::async::Asyncable
{
public:
    void SetUp() override
    {
        m_service = new AppUpdateService(muse::modularity::globalCtx());

        m_configuration = std::make_shared<NiceMock<UpdateConfigurationMock> >();
        m_service->configuration.set(m_configuration);

        m_networkManagerCreator = std::make_shared<NiceMock<muse::network::NetworkManagerCreatorMock> >();
        m_service->networkManagerCreator.set(m_networkManagerCreator);

        m_networkManager = std::make_shared<muse::network::NetworkManagerMock>();
        ON_CALL(*m_networkManagerCreator, makeNetworkManager())
        .WillByDefault(Return(m_networkManager));

        m_systemInfoMock = std::make_shared<NiceMock<SystemInfoMock> >();
        m_service->systemInfo.set(m_systemInfoMock);

        ON_CALL(*m_systemInfoMock, productType())
        .WillByDefault(Return(ISystemInfo::ProductType::Linux));

        m_fileSystem = std::make_shared<NiceMock<io::FileSystemMock> >();
        m_service->fileSystem.set(m_fileSystem);

        m_updateInstaller = std::make_shared<NiceMock<UpdateInstallerMock> >();
        m_service->updateInstaller.set(m_updateInstaller);

        m_application = std::make_shared<NiceMock<ApplicationMock> >();
        m_service->application.set(m_application);

        m_uiConfiguration = std::make_shared<NiceMock<ui::UiConfigurationMock> >();
        m_service->uiConfiguration.set(m_uiConfiguration);
        ON_CALL(*m_uiConfiguration, currentTheme())
        .WillByDefault(testing::ReturnRef(m_theme));

        ON_CALL(*m_application, fullVersion())
        .WillByDefault(Return(Version(CURRENT_VERSION)));
        ON_CALL(*m_application, title())
        .WillByDefault(Return(String(u"App")));
    }

    void TearDown() override
    {
        delete m_service;

        //! NOTE: Drop the calls queued for the deleted service, so that they do
        //! not reach the next test's service if it is allocated at the same address
        pump();
    }

    void makeReleaseInfo()
    {
        std::string checkForAppUpdateUrl = "checkForAppUpdateUrl";
        EXPECT_CALL(*m_configuration, checkForAppUpdateUrl())
        .WillOnce(Return(checkForAppUpdateUrl));

        QString releasesNotes = "{"
                                "\"tag_name\": \"v1000.0\","
                                "\"assets\": ["
                                "{ \"name\": \"MuseScore.dmg\", \"browser_download_url\": \"blabla\", \"size\": 12345 },"
                                "{ \"name\": \"MuseScore.zip\", \"browser_download_url\": \"blabla\" },"
                                "{ \"name\": \"MuseScore.msi\", \"browser_download_url\": \"blabla\" },"
                                "{ \"name\": \"MuseScore.AppImage\", \"browser_download_url\": \"blabla\" }"
                                "],"
                                "\"assetsNew\": ["
                                "{ \"name\": \"MuseScore-arm.AppImage\", \"browser_download_url\": \"blabla\" },"
                                "{ \"name\": \"MuseScore-aarch64.AppImage\", \"browser_download_url\": \"blabla\" }"
                                "]"
                                "}";

        EXPECT_CALL(*m_networkManager, get(QUrl(QString::fromStdString(checkForAppUpdateUrl)), _, _))
        .WillOnce(testing::Invoke(
                      [this, releasesNotes](const QUrl&, IncomingDevicePtr buf, const RequestHeaders&) {
            buf->open(muse::network::IncomingDevice::WriteOnly);
            buf->write(releasesNotes.toUtf8());
            buf->close();

            return muse::RetVal<Progress>::make_ok(m_getReleaseInfoProgress);
        }));
    }

    void makePreviousReleasesNotes()
    {
        std::string previousAppReleasesNotesUrl = "previousAppReleasesNotesUrl";
        EXPECT_CALL(*m_configuration, previousAppReleasesNotesUrl())
        .WillOnce(Return(previousAppReleasesNotesUrl));

        //! [GIVEN] Previous releases notes. Contains chaotic order of versions
        QString releasesNotes = QString("{"
                                        "\"releases\": ["
                                        "{ \"version\": \"40000.3\", \"notes\": \"blabla3\" },"
                                        "{ \"version\": \"40000.4\", \"notes\": \"blabla4\" },"
                                        "{ \"version\": \"%1\", \"notes\": \"blabla2\" },"
                                        "{ \"version\": \"0.4.1\", \"notes\": \"blabla1\" }"
                                        "]"
                                        "}").arg(CURRENT_VERSION);

        EXPECT_CALL(*m_networkManager, get(QUrl(QString::fromStdString(previousAppReleasesNotesUrl)), _, _))
        .WillOnce(testing::Invoke(
                      [this, releasesNotes](const QUrl&, IncomingDevicePtr buf, const RequestHeaders&) {
            buf->open(muse::network::IncomingDevice::WriteOnly);
            buf->write(releasesNotes.toUtf8());
            buf->close();

            return muse::RetVal<Progress>::make_ok(m_getPrevReleasesInfoProgress);
        }));
    }

    //! [GIVEN] An available release is ready to be downloaded.
    void givenAvailableRelease(const std::string& fileName = "MuseScore.dmg",
                               const std::string& dataPath = "upd",
                               uint64_t fileSize = 0)
    {
        ReleaseInfo info;
        info.version = "1000.0";
        info.fileName = fileName;
        info.fileUrl = "package-url";
        info.fileSize = fileSize;
        m_service->m_lastCheckResult = RetVal<ReleaseInfo>::make_ok(info);

        ON_CALL(*m_configuration, updateDataPath())
        .WillByDefault(Return(io::path_t(dataPath)));

        ON_CALL(*m_configuration, downloadsPath())
        .WillByDefault(Return(io::path_t(dataPath)));

        ON_CALL(*m_fileSystem, makePath(_))
        .WillByDefault(Return(muse::make_ok()));
    }

    //! [GIVEN] The release can be installed in-place and nothing is downloaded yet
    void givenInPlaceInstall(uint64_t fileSize = 0)
    {
        givenAvailableRelease("MuseScore.dmg", "upd", fileSize);

        ON_CALL(*m_updateInstaller, isInPlaceUpdateSupported())
        .WillByDefault(Return(true));
        ON_CALL(*m_fileSystem, exists(_))
        .WillByDefault(Return(Ret(false)));
        ON_CALL(*m_fileSystem, move(_, _, _))
        .WillByDefault(Return(muse::make_ok()));
    }

    void expectDownloadRequest()
    {
        EXPECT_CALL(*m_networkManager, get(_, _, _))
        .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
            return RetVal<Progress>::make_ok(m_downloadProgress);
        }));
    }

    void finishDownload()
    {
        ProgressResult res = ProgressResult::make_ok(Val());
        res.ret.setData("status", 200);
        m_downloadProgress.finish(res);
    }

    void cleanupStalePackages(const std::string& keepFileName)
    {
        m_service->cleanupStalePackages(keepFileName);
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

    inline static const io::path_t PACKAGE = io::path_t("upd/MuseScore.dmg");
    inline static const io::path_t STAGED = io::path_t("upd/staging/MuseScore.app");

    AppUpdateService* m_service = nullptr;
    std::shared_ptr<UpdateConfigurationMock> m_configuration;
    std::shared_ptr<muse::network::NetworkManagerCreatorMock> m_networkManagerCreator;
    std::shared_ptr<muse::network::NetworkManagerMock> m_networkManager;
    std::shared_ptr<SystemInfoMock> m_systemInfoMock;
    std::shared_ptr<io::FileSystemMock> m_fileSystem;
    std::shared_ptr<UpdateInstallerMock> m_updateInstaller;
    std::shared_ptr<ApplicationMock> m_application;
    std::shared_ptr<ui::UiConfigurationMock> m_uiConfiguration;
    ui::ThemeInfo m_theme;
    Progress m_getReleaseInfoProgress;
    Progress m_getPrevReleasesInfoProgress;
    Progress m_downloadProgress;
};
}

TEST_F(AppUpdateServiceTests, ParseRelease_Linux_x86_64)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [GIVEN] System is Linux x86_64
    ON_CALL(*m_systemInfoMock, productType())
    .WillByDefault(Return(ISystemInfo::ProductType::Linux));

    ON_CALL(*m_systemInfoMock, cpuArchitecture())
    .WillByDefault(Return(ISystemInfo::CpuArchitecture::x86_64));

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.fileName, "MuseScore.AppImage");
}

TEST_F(AppUpdateServiceTests, ParseRelease_Linux_arm)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [GIVEN] System is Linux arm
    ON_CALL(*m_systemInfoMock, productType())
    .WillByDefault(Return(ISystemInfo::ProductType::Linux));

    ON_CALL(*m_systemInfoMock, cpuArchitecture())
    .WillByDefault(Return(ISystemInfo::CpuArchitecture::Arm));

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.fileName, "MuseScore-arm.AppImage");
}

TEST_F(AppUpdateServiceTests, ParseRelease_Linux_aarch64)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [GIVEN] System is Linux arm64
    ON_CALL(*m_systemInfoMock, productType())
    .WillByDefault(Return(ISystemInfo::ProductType::Linux));

    ON_CALL(*m_systemInfoMock, cpuArchitecture())
    .WillByDefault(Return(ISystemInfo::CpuArchitecture::Arm64));

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.fileName, "MuseScore-aarch64.AppImage");
}

TEST_F(AppUpdateServiceTests, ParseRelease_Linux_Unknown)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [GIVEN] System is Linux Unknown
    ON_CALL(*m_systemInfoMock, productType())
    .WillByDefault(Return(ISystemInfo::ProductType::Linux));

    ON_CALL(*m_systemInfoMock, cpuArchitecture())
    .WillByDefault(Return(ISystemInfo::CpuArchitecture::Unknown));

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.fileName, "MuseScore.AppImage");
}

TEST_F(AppUpdateServiceTests, ParseRelease_Windows)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [GIVEN] System is Windows, cpuArchitecture isn't important
    ON_CALL(*m_systemInfoMock, productType())
    .WillByDefault(Return(ISystemInfo::ProductType::Windows));

    EXPECT_CALL(*m_systemInfoMock, cpuArchitecture())
    .Times(1);

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.fileName, "MuseScore.msi");
}

TEST_F(AppUpdateServiceTests, ParseRelease_MacOS)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [GIVEN] System is MacOS, cpuArchitecture isn't important
    ON_CALL(*m_systemInfoMock, productType())
    .WillByDefault(Return(ISystemInfo::ProductType::MacOS));

    EXPECT_CALL(*m_systemInfoMock, cpuArchitecture())
    .Times(1);

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.fileName, "MuseScore.dmg");
    EXPECT_EQ(retVal.val.fileSize, 12345u);
}

TEST_F(AppUpdateServiceTests, CheckForUpdate_ReleasesNotes)
{
    //! [GIVEN] Release info
    makeReleaseInfo();
    makePreviousReleasesNotes();

    //! [THEN] Versions should be in correct order and don't contain current version
    PrevReleasesNotesList expectedReleasesNotes {
        { "40000.3", "blabla3" },
        { "40000.4", "blabla4" },
    };

    //! [WHEN] Check for update
    RetVal<ReleaseInfo> retVal;
    m_service->checkForUpdate().onResolve(this, [&retVal](const RetVal<ReleaseInfo>& res) {
        retVal = res;
    });

    //! [WHEN] Process messages
    async::processMessages();

    //! [WHEN] Successfully downloaded release info
    m_getReleaseInfoProgress.finish(ProgressResult::make_ok({}));

    //! [WHEN] Successfully downloaded previous releases info
    m_getPrevReleasesInfoProgress.finish(ProgressResult::make_ok({}));

    //! [THEN] Should return correct release file
    EXPECT_TRUE(retVal.ret);
    EXPECT_EQ(retVal.val.previousReleasesNotes, expectedReleasesNotes);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_FreshDownload_NoRangeHeader)
{
    //! [GIVEN] An available release and no partial download on disk
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));

    //! [WHEN] Download the release
    RequestHeaders capturedHeaders;
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this, &capturedHeaders](const QUrl&, IncomingDevicePtr, const RequestHeaders& headers) {
        capturedHeaders = headers;
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    m_service->downloadRelease();

    //! [THEN] No Range header is sent (download starts from scratch)
    EXPECT_FALSE(capturedHeaders.rawHeaders.contains("Range"));
}

TEST_F(AppUpdateServiceTests, DownloadRelease_ResumesFromPartial_SendsRangeHeader)
{
    //! [GIVEN] An available release with a 1000-byte partial download on disk
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_fileSystem, exists(io::path_t("upd/MuseScore.dmg")))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, fileSize(_))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(static_cast<uint64_t>(1000))));

    //! [WHEN] Download the release
    RequestHeaders capturedHeaders;
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this, &capturedHeaders](const QUrl&, IncomingDevicePtr, const RequestHeaders& headers) {
        capturedHeaders = headers;
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    m_service->downloadRelease();

    //! [THEN] A Range header requests the remaining bytes
    EXPECT_EQ(capturedHeaders.rawHeaders.value("Range"), QByteArray("bytes=1000-"));
}

TEST_F(AppUpdateServiceTests, DownloadRelease_Success_PromotesPartialToFinal)
{
    //! [GIVEN] A fresh download of an available release
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    //! [THEN] On success the partial file is promoted to the final package name
    EXPECT_CALL(*m_fileSystem, move(io::path_t("upd/MuseScore.dmg.part"),
                                    io::path_t("upd/MuseScore.dmg"), true))
    .WillOnce(Return(muse::make_ok()));

    m_service->downloadRelease();

    //! [WHEN] The download finishes with HTTP 200 (full content received)
    ProgressResult res = ProgressResult::make_ok(Val());
    res.ret.setData("status", 200);
    m_downloadProgress.finish(res);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_AlreadyInProgress_AttachesToIt)
{
    //! [GIVEN] A download is already running
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));

    //! [THEN] Only one network request is made
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    RetVal<Progress> first = m_service->downloadRelease();
    EXPECT_TRUE(first.ret);

    //! [WHEN] A second download is requested while the first is in progress
    RetVal<Progress> second = m_service->downloadRelease();

    //! [THEN] The caller is attached to the running download instead
    EXPECT_TRUE(second.ret);

    //! [WHEN] The download finishes, a new one may be started again
    m_downloadProgress.finish(ProgressResult::make_ret(muse::make_ret(muse::Ret::Code::Cancel)));

    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    RetVal<Progress> third = m_service->downloadRelease();
    EXPECT_TRUE(third.ret);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_RangeNotHonoured_DiscardsPartial)
{
    //! [GIVEN] A resume attempt (partial on disk -> Range requested)
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_fileSystem, exists(io::path_t("upd/MuseScore.dmg")))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, fileSize(_))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(static_cast<uint64_t>(1000))));
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    //! [THEN] The now-stale partial file is removed so the next attempt starts clean
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("upd/MuseScore.dmg.part"), false))
    .WillOnce(Return(muse::make_ok()));

    m_service->downloadRelease();

    //! [WHEN] The server ignored the Range request and replied with HTTP 200
    ProgressResult res = ProgressResult::make_ok(Val());
    res.ret.setData("status", 200);
    m_downloadProgress.finish(res);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_NotEnoughDiskSpace_DoesNotStart)
{
    //! [GIVEN] A 100 MB release and only 50 MB available where it would be stored
    const uint64_t mb = 1024 * 1024;
    givenAvailableRelease("MuseScore.dmg", "upd", 100 * mb);
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, availableSpace(io::path_t("upd")))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(50 * mb)));

    //! [THEN] No network request is made
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .Times(0);

    //! [WHEN] Download the release
    RetVal<Progress> rv = m_service->downloadRelease();

    //! [THEN] The download is refused with a disk space error naming the missing amount
    //! (100 MB package + 100 MB reserve - 50 MB available = 150 MB)
    EXPECT_EQ(rv.ret.code(), static_cast<int>(Err::NotEnoughDiskSpace));
    EXPECT_NE(rv.ret.text().find("150 MB"), std::string::npos);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_EnoughDiskSpace_Starts)
{
    //! [GIVEN] A 100 MB release and plenty of space available
    const uint64_t mb = 1024 * 1024;
    givenAvailableRelease("MuseScore.dmg", "upd", 100 * mb);
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, availableSpace(io::path_t("upd")))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(10000 * mb)));

    //! [THEN] The download is started
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    //! [WHEN] Download the release
    RetVal<Progress> rv = m_service->downloadRelease();
    EXPECT_TRUE(rv.ret);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_UnknownPackageSize_SkipsDiskSpaceCheck)
{
    //! [GIVEN] The release has no size information and the disk is nearly full
    givenAvailableRelease("MuseScore.dmg", "upd", 0);
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, availableSpace(_))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(static_cast<uint64_t>(1))));

    //! [THEN] The download is still started
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    //! [WHEN] Download the release
    RetVal<Progress> rv = m_service->downloadRelease();
    EXPECT_TRUE(rv.ret);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_AbsurdPackageSize_Rejected)
{
    //! [GIVEN] The server reports a package size far beyond anything real
    //! (3 * size would wrap around uint64_t to 2 bytes)
    givenAvailableRelease("MuseScore.dmg", "upd", 6148914691236517206ull);
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, availableSpace(_))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(10000ull * 1024 * 1024)));

    //! [THEN] No network request is made
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .Times(0);

    //! [WHEN] Download the release
    RetVal<Progress> rv = m_service->downloadRelease();

    //! [THEN] The download is refused
    EXPECT_EQ(rv.ret.code(), static_cast<int>(Err::NotEnoughDiskSpace));
}

TEST_F(AppUpdateServiceTests, DownloadRelease_Resume_OnlyRemainingBytesRequired)
{
    //! [GIVEN] A 100 MB release, 90 MB already downloaded, and 120 MB available
    //! (enough for the remaining 10 MB plus the reserve, but not for the whole file)
    const uint64_t mb = 1024 * 1024;
    givenAvailableRelease("MuseScore.dmg", "upd", 100 * mb);
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_fileSystem, exists(io::path_t("upd/MuseScore.dmg")))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, fileSize(_))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(90 * mb)));
    ON_CALL(*m_fileSystem, availableSpace(io::path_t("upd")))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(120 * mb)));

    //! [THEN] The download is resumed
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    //! [WHEN] Download the release
    RetVal<Progress> rv = m_service->downloadRelease();
    EXPECT_TRUE(rv.ret);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_InPlace_PreparesBeforeFinishing)
{
    //! [GIVEN] A download of a release that can be installed in-place
    givenInPlaceInstall();
    expectDownloadRequest();

    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    std::vector<std::pair<int64_t, int64_t> > progress;
    rv.val.progressChanged().onReceive(this, [&progress](int64_t current, int64_t total, const std::string&) {
        progress.emplace_back(current, total);
    });

    std::optional<ProgressResult> result;
    rv.val.finished().onReceive(this, [&result](const ProgressResult& res) {
        result = res;
    });

    //! [THEN] The download takes the first 90% of the progress
    m_downloadProgress.progress(50, 100);
    ASSERT_EQ(progress.size(), 1u);
    EXPECT_EQ(progress.back(), std::make_pair(int64_t(450), int64_t(1000)));

    //! [THEN] The downloaded package is prepared
    EXPECT_CALL(*m_updateInstaller, prepareUpdate(PACKAGE))
    .WillOnce(Return(RetVal<io::path_t>::make_ok(STAGED)));

    //! [WHEN] The download finishes
    finishDownload();

    //! [THEN] Nothing is reported as finished until the update is prepared
    EXPECT_FALSE(result.has_value());

    pump();

    //! [THEN] The preparation takes the rest of the progress, then the result is reported
    ASSERT_EQ(progress.size(), 3u);
    EXPECT_EQ(progress.at(1), std::make_pair(int64_t(900), int64_t(1000)));
    EXPECT_EQ(progress.at(2), std::make_pair(int64_t(1000), int64_t(1000)));

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ret);
    EXPECT_EQ(result->val.toString(), PACKAGE.toStdString());

    //! [THEN] The release is ready to install
    ON_CALL(*m_fileSystem, exists(PACKAGE))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_fileSystem, exists(STAGED))
    .WillByDefault(Return(Ret(true)));
    EXPECT_TRUE(m_service->isReleaseReadyToInstall());

    //! [THEN] Installing it hands the prepared update to the installer
    EXPECT_CALL(*m_updateInstaller, finalizeUpdate(STAGED, _))
    .WillOnce(Return(muse::make_ok()));
    EXPECT_TRUE(m_service->installUpdate());
}

TEST_F(AppUpdateServiceTests, DownloadRelease_AlreadyDownloaded_OnlyPrepares)
{
    //! [GIVEN] The package was downloaded in a previous session
    givenInPlaceInstall();
    ON_CALL(*m_fileSystem, exists(PACKAGE))
    .WillByDefault(Return(Ret(true)));

    //! [THEN] Nothing is downloaded, the package is only prepared
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .Times(0);
    EXPECT_CALL(*m_updateInstaller, prepareUpdate(PACKAGE))
    .WillOnce(Return(RetVal<io::path_t>::make_ok(STAGED)));

    //! [WHEN] Download the release
    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    //! [THEN] Subscribers attached right after the call get the result
    std::optional<ProgressResult> result;
    rv.val.finished().onReceive(this, [&result](const ProgressResult& res) {
        result = res;
    });

    pump();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ret);
    EXPECT_EQ(result->val.toString(), PACKAGE.toStdString());
}

TEST_F(AppUpdateServiceTests, DownloadRelease_NotInPlace_FinishesWithoutPreparing)
{
    //! [GIVEN] A download of a release that can not be installed in-place
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    ON_CALL(*m_fileSystem, move(_, _, _))
    .WillByDefault(Return(muse::make_ok()));
    expectDownloadRequest();

    //! [THEN] Nothing is prepared
    EXPECT_CALL(*m_updateInstaller, prepareUpdate(_))
    .Times(0);

    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    std::optional<ProgressResult> result;
    rv.val.finished().onReceive(this, [&result](const ProgressResult& res) {
        result = res;
    });

    //! [WHEN] The download finishes
    finishDownload();
    pump();

    //! [THEN] The downloaded package is reported
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ret);
    EXPECT_EQ(result->val.toString(), PACKAGE.toStdString());

    //! [THEN] The downloaded release is ready to install as is
    ON_CALL(*m_fileSystem, exists(PACKAGE))
    .WillByDefault(Return(Ret(true)));
    EXPECT_TRUE(m_service->isReleaseReadyToInstall());
}

TEST_F(AppUpdateServiceTests, DownloadRelease_NoRoomToPrepare_FinishesWithError)
{
    //! [GIVEN] A downloaded 100 MB package and only 150 MB left in the update data dir
    const uint64_t mb = 1024 * 1024;
    givenInPlaceInstall();
    ON_CALL(*m_fileSystem, exists(PACKAGE))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_fileSystem, fileSize(PACKAGE))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(100 * mb)));
    ON_CALL(*m_fileSystem, availableSpace(io::path_t("upd")))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(150 * mb)));

    //! [THEN] The installer is not asked to prepare anything
    EXPECT_CALL(*m_updateInstaller, prepareUpdate(_))
    .Times(0);

    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    std::optional<ProgressResult> result;
    rv.val.finished().onReceive(this, [&result](const ProgressResult& res) {
        result = res;
    });

    pump();

    //! [THEN] It fails with a disk space error
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ret.code(), static_cast<int>(Err::NotEnoughDiskSpace));
}

TEST_F(AppUpdateServiceTests, DownloadRelease_PrepareFailed_FinishesWithPackage)
{
    //! [GIVEN] A download of a release that can be installed in-place
    givenInPlaceInstall();
    expectDownloadRequest();

    //! [GIVEN] The package can not be prepared
    EXPECT_CALL(*m_updateInstaller, prepareUpdate(PACKAGE))
    .WillOnce(Return(RetVal<io::path_t>(make_ret(Err::UnknownError))));

    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    std::optional<ProgressResult> result;
    rv.val.finished().onReceive(this, [&result](const ProgressResult& res) {
        result = res;
    });

    //! [WHEN] The download finishes
    finishDownload();
    pump();

    //! [THEN] The package is still reported, so it can be installed manually
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->ret);
    EXPECT_EQ(result->val.toString(), PACKAGE.toStdString());

    //! [THEN] It is not ready to install in-place
    ON_CALL(*m_fileSystem, exists(PACKAGE))
    .WillByDefault(Return(Ret(true)));
    EXPECT_FALSE(m_service->isReleaseReadyToInstall());

    EXPECT_CALL(*m_updateInstaller, finalizeUpdate(_, _))
    .Times(0);
    EXPECT_FALSE(m_service->installUpdate());
}

TEST_F(AppUpdateServiceTests, DownloadRelease_CanceledWhilePreparing_DropsPreparedUpdate)
{
    //! [GIVEN] The package is being prepared
    givenInPlaceInstall();
    ON_CALL(*m_fileSystem, exists(PACKAGE))
    .WillByDefault(Return(Ret(true)));
    EXPECT_CALL(*m_updateInstaller, prepareUpdate(PACKAGE))
    .WillOnce(Return(RetVal<io::path_t>::make_ok(STAGED)));

    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    std::vector<ProgressResult> results;
    rv.val.finished().onReceive(this, [&results](const ProgressResult& res) {
        results.push_back(res);
    });

    //! [THEN] The prepared files are removed
    EXPECT_CALL(*m_fileSystem, remove(STAGED, false))
    .WillOnce(Return(muse::make_ok()));

    //! [WHEN] The release is removed while it is being prepared
    async::processMessages();
    m_service->removeDownloadedRelease();
    pump();

    //! [THEN] Only the cancellation is reported, and nothing is prepared
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results.front().ret.code(), static_cast<int>(Ret::Code::Cancel));

    ON_CALL(*m_fileSystem, exists(STAGED))
    .WillByDefault(Return(Ret(true)));
    EXPECT_FALSE(m_service->isReleaseReadyToInstall());
}

TEST_F(AppUpdateServiceTests, CleanupStalePackages_KeepsPreparedUpdateOfKeptPackage)
{
    //! [GIVEN] A prepared update
    givenInPlaceInstall();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_updateInstaller, prepareUpdate(PACKAGE))
    .WillByDefault(Return(RetVal<io::path_t>::make_ok(STAGED)));

    m_service->downloadRelease();
    pump();
    ASSERT_TRUE(m_service->isReleaseReadyToInstall());

    ON_CALL(*m_fileSystem, scanFiles(io::path_t("upd"), _, _))
    .WillByDefault(Return(RetVal<io::paths_t>::make_ok(io::paths_t {
        PACKAGE, io::path_t("upd/staging"), io::path_t("upd/other")
    })));

    //! [THEN] Only unrelated files are removed
    EXPECT_CALL(*m_fileSystem, remove(PACKAGE, false))
    .Times(1)
    .WillRepeatedly(Return(muse::make_ok()));
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("upd/other"), false))
    .Times(2)
    .WillRepeatedly(Return(muse::make_ok()));
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("upd/staging"), false))
    .Times(0);

    //! [WHEN] The same release is found again
    cleanupStalePackages("MuseScore.dmg");
    EXPECT_TRUE(m_service->isReleaseReadyToInstall());

    //! [THEN] Everything is removed for another release
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("upd/staging"), false))
    .WillOnce(Return(muse::make_ok()));

    //! [WHEN] Another release is found
    cleanupStalePackages("MuseScore-new.dmg");
    EXPECT_FALSE(m_service->isReleaseReadyToInstall());
}

TEST_F(AppUpdateServiceTests, RemoveDownloadedRelease_RemovesPackageAndPartial)
{
    //! [GIVEN] A release whose package was recorded as downloaded
    givenAvailableRelease();
    ON_CALL(*m_configuration, lastDownloadedPackagePath())
    .WillByDefault(Return(io::path_t("upd/MuseScore.dmg")));
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));

    //! [THEN] Both the package and its partial file are removed and the record is cleared
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("upd/MuseScore.dmg"), false))
    .WillOnce(Return(muse::make_ok()));
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("upd/MuseScore.dmg.part"), false))
    .WillOnce(Return(muse::make_ok()));
    EXPECT_CALL(*m_configuration, setLastDownloadedPackagePath(io::path_t()));

    //! [WHEN] The release is removed
    m_service->removeDownloadedRelease();
}

TEST_F(AppUpdateServiceTests, RemoveDownloadedRelease_WhileDownloading_CancelsDownload)
{
    //! [GIVEN] A download is in progress
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    RetVal<Progress> rv = m_service->downloadRelease();
    ASSERT_TRUE(rv.ret);

    bool canceled = false;
    m_downloadProgress.canceled().onNotify(this, [&canceled]() { canceled = true; });

    //! [WHEN] The release is removed
    m_service->removeDownloadedRelease();

    //! [THEN] The network request is canceled and a new download may be started
    EXPECT_TRUE(canceled);

    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));
    EXPECT_TRUE(m_service->downloadRelease().ret);
}
