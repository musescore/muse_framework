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

#include <QIODevice>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

#include "framework/network/networktypes.h"

#include "mocks/updateconfigurationmock.h"
#include "network/tests/mocks/networkmanagercreatormock.h"
#include "network/tests/mocks/networkmanagermock.h"
#include "global/tests/mocks/systeminfomock.h"
#include "global/tests/mocks/filesystemmock.h"

#include "update/internal/appupdateservice.h"

#include "modularity/ioc.h"
#include "global/iapplication.h"

#include "async/processevents.h"
#include "async/async.h"

using namespace muse;
using namespace muse::update;
using namespace muse::async;
using namespace muse::network;

namespace muse::update {
class AppUpdateServiceTests : public ::testing::Test, public ::async::Asyncable
{
    muse::GlobalInject<muse::IApplication> application;

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
    }

    void TearDown() override
    {
        delete m_service;
    }

    void makeReleaseInfo()
    {
        std::string checkForAppUpdateUrl = "checkForAppUpdateUrl";
        EXPECT_CALL(*m_configuration, checkForAppUpdateUrl())
        .WillOnce(Return(checkForAppUpdateUrl));

        QString releasesNotes = "{"
                                "\"tag_name\": \"v1000.0\","
                                "\"assets\": ["
                                "{ \"name\": \"MuseScore.dmg\", \"browser_download_url\": \"blabla\" },"
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
                                        "}").arg(application()->fullVersion().toString());

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
                               const std::string& dataPath = "/tmp/upd")
    {
        ReleaseInfo info;
        info.version = "1000.0";
        info.fileName = fileName;
        info.fileUrl = "http://test/" + fileName;
        m_service->m_lastCheckResult = RetVal<ReleaseInfo>::make_ok(info);

        ON_CALL(*m_configuration, updateDataPath())
        .WillByDefault(Return(io::path_t(dataPath)));

        ON_CALL(*m_fileSystem, makePath(_))
        .WillByDefault(Return(muse::make_ok()));
    }

    AppUpdateService* m_service = nullptr;
    std::shared_ptr<UpdateConfigurationMock> m_configuration;
    std::shared_ptr<muse::network::NetworkManagerCreatorMock> m_networkManagerCreator;
    std::shared_ptr<muse::network::NetworkManagerMock> m_networkManager;
    std::shared_ptr<SystemInfoMock> m_systemInfoMock;
    std::shared_ptr<io::FileSystemMock> m_fileSystem;
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
    EXPECT_CALL(*m_fileSystem, move(io::path_t("/tmp/upd/MuseScore.dmg.part"),
                                    io::path_t("/tmp/upd/MuseScore.dmg"), true))
    .WillOnce(Return(muse::make_ok()));

    m_service->downloadRelease();

    //! [WHEN] The download finishes with HTTP 200 (full content received)
    ProgressResult res = ProgressResult::make_ok(Val());
    res.ret.setData("status", 200);
    m_downloadProgress.finish(res);
}

TEST_F(AppUpdateServiceTests, DownloadRelease_RangeNotHonoured_DiscardsPartial)
{
    //! [GIVEN] A resume attempt (partial on disk -> Range requested)
    givenAvailableRelease();
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(true)));
    ON_CALL(*m_fileSystem, fileSize(_))
    .WillByDefault(Return(RetVal<uint64_t>::make_ok(static_cast<uint64_t>(1000))));
    EXPECT_CALL(*m_networkManager, get(_, _, _))
    .WillOnce(testing::Invoke([this](const QUrl&, IncomingDevicePtr, const RequestHeaders&) {
        return RetVal<Progress>::make_ok(m_downloadProgress);
    }));

    //! [THEN] The now-stale partial file is removed so the next attempt starts clean
    EXPECT_CALL(*m_fileSystem, remove(io::path_t("/tmp/upd/MuseScore.dmg.part"), false))
    .WillOnce(Return(muse::make_ok()));

    m_service->downloadRelease();

    //! [WHEN] The server ignored the Range request and replied with HTTP 200
    ProgressResult res = ProgressResult::make_ok(Val());
    res.ret.setData("status", 200);
    m_downloadProgress.finish(res);
}
