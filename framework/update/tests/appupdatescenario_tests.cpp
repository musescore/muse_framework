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

#include "mocks/updateconfigurationmock.h"
#include "mocks/appupdateservicemock.h"
#include "network/tests/mocks/networkinformationmock.h"

#include "update/internal/appupdatescenario.h"

#include "modularity/ioc.h"

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

        //! [GIVEN] An update is available and auto-install is enabled
        ReleaseInfo info;
        info.version = "1000.0";
        m_lastCheckResult = RetVal<ReleaseInfo>::make_ok(info);

        ON_CALL(*m_service, lastCheckResult())
        .WillByDefault(ReturnRef(m_lastCheckResult));

        ON_CALL(*m_service, isReleaseDownloaded())
        .WillByDefault(Return(false));

        ON_CALL(*m_configuration, autoInstallEnabled())
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

    AppUpdateScenario* m_scenario = nullptr;
    std::shared_ptr<UpdateConfigurationMock> m_configuration;
    std::shared_ptr<AppUpdateServiceMock> m_service;
    std::shared_ptr<network::NetworkInformationMock> m_networkInformation;
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

    //! [WHEN] The download finishes successfully
    m_downloadProgress.finish(ProgressResult::make_ok(Val(std::string("/tmp/upd/MuseScore.dmg"))));

    //! [THEN] The update is surfaced as ready to install
    EXPECT_TRUE(m_scenario->hasReadyUpdate());
    EXPECT_EQ(m_scenario->readyUpdateVersion(), "1000.0");
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
    .WillByDefault(Return(io::path_t("/tmp/upd/MuseScore.dmg")));

    //! [GIVEN] The network connection is metered
    ON_CALL(*m_networkInformation, isMetered())
    .WillByDefault(Return(true));

    //! [THEN] No download is started
    EXPECT_CALL(*m_service, downloadRelease())
    .Times(0);

    //! [WHEN] A background download is requested
    downloadUpdateInBackground();

    //! [THEN] The downloaded update is still surfaced as ready to install
    EXPECT_TRUE(m_scenario->hasReadyUpdate());
    EXPECT_EQ(m_scenario->readyUpdateVersion(), "1000.0");
}
