/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include "audioplugins/internal/audiopluginsloadguard.h"

#include "global/tests/mocks/globalconfigurationmock.h"
#include "global/tests/mocks/filesystemmock.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

using namespace muse;
using namespace muse::audioplugins;

namespace muse::audioplugins {
class AudioPlugins_AudioPluginsLoadGuardTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_guard = std::make_shared<AudioPluginsLoadGuard>();
        m_globalConfiguration = std::make_shared<NiceMock<GlobalConfigurationMock> >();
        m_fileSystem = std::make_shared<NiceMock<io::FileSystemMock> >();

        m_guard->globalConfiguration.set(m_globalConfiguration);
        m_guard->fileSystem.set(m_fileSystem);

        ON_CALL(*m_globalConfiguration, userAppDataPath())
        .WillByDefault(Return(io::path_t("/appdata")));

        ON_CALL(*m_fileSystem, makePath(_))
        .WillByDefault(Return(make_ok()));

        ON_CALL(*m_fileSystem, writeFile(_, _))
        .WillByDefault(Return(make_ok()));

        ON_CALL(*m_fileSystem, remove(_, _))
        .WillByDefault(Return(make_ok()));
    }

    std::shared_ptr<AudioPluginsLoadGuard> m_guard;
    std::shared_ptr<GlobalConfigurationMock> m_globalConfiguration;
    std::shared_ptr<io::FileSystemMock> m_fileSystem;
};
}

TEST_F(AudioPlugins_AudioPluginsLoadGuardTest, BeginLoadWritesSentinelWithIdAsContent)
{
    // [GIVEN] Some plugin id (may contain characters not valid in file names)
    const PluginResourceId resourceId = "Effect/VST3: Weird\\Id";

    // [THEN] A sentinel file is written; its content is the exact id
    io::path_t writtenPath;
    ByteArray writtenData;
    EXPECT_CALL(*m_fileSystem, writeFile(_, _))
    .WillOnce(DoAll(SaveArg<0>(&writtenPath), SaveArg<1>(&writtenData), Return(make_ok())));

    // [WHEN] Begin the load
    EXPECT_TRUE(m_guard->beginLoad(resourceId));

    EXPECT_EQ(std::string(writtenData.constChar(), writtenData.size()), resourceId);

    // [THEN] Ending the load removes the very same file
    EXPECT_CALL(*m_fileSystem, remove(writtenPath, _))
    .WillOnce(Return(make_ok()));

    // [WHEN] End the load
    m_guard->endLoad(resourceId);
}

TEST_F(AudioPlugins_AudioPluginsLoadGuardTest, DanglingLoadsReturnsSentinelContents)
{
    // [GIVEN] Two sentinel files left over from a crashed run
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(make_ok()));

    const io::paths_t sentinelFiles { "/appdata/audio_plugins_loading/aaa.loading",
                                      "/appdata/audio_plugins_loading/bbb.loading" };
    ON_CALL(*m_fileSystem, scanFiles(_, _, _))
    .WillByDefault(Return(RetVal<io::paths_t>::make_ok(sentinelFiles)));

    ON_CALL(*m_fileSystem, readFile(sentinelFiles[0]))
    .WillByDefault(Return(RetVal<ByteArray>::make_ok(ByteArray("PluginA", 7))));
    ON_CALL(*m_fileSystem, readFile(sentinelFiles[1]))
    .WillByDefault(Return(RetVal<ByteArray>::make_ok(ByteArray("PluginB", 7))));

    // [WHEN] Query the dangling loads
    const PluginResourceIdList dangling = m_guard->danglingLoads();

    // [THEN] The ids stored in the sentinels are returned
    EXPECT_EQ(dangling, (PluginResourceIdList { "PluginA", "PluginB" }));
}

TEST_F(AudioPlugins_AudioPluginsLoadGuardTest, DanglingLoadsEmptyWhenNoSentinelDir)
{
    // [GIVEN] The sentinel dir does not exist (nothing ever crashed)
    ON_CALL(*m_fileSystem, exists(_))
    .WillByDefault(Return(Ret(false)));

    // [THEN] No scan is even attempted
    EXPECT_CALL(*m_fileSystem, scanFiles(_, _, _))
    .Times(0);

    // [WHEN] Query the dangling loads
    EXPECT_TRUE(m_guard->danglingLoads().empty());
}
