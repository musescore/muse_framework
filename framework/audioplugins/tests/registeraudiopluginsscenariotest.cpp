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
#include <gmock/gmock.h>

#include "audioplugins/internal/registeraudiopluginsscenario.h"
#include "audioplugins/audiopluginserrors.h"

#include "global/tests/mocks/globalconfigurationmock.h"
#include "global/tests/mocks/processmock.h"
#include "global/tests/mocks/filesystemmock.h"
#include "interactive/tests/mocks/interactivemock.h"

#include "mocks/knownaudiopluginsregistermock.h"
#include "mocks/audiopluginsscannerregistermock.h"
#include "mocks/audiopluginsscannermock.h"
#include "mocks/audiopluginmetareaderregistermock.h"
#include "mocks/audiopluginmetareadermock.h"
#include "mocks/audiopluginsloadguardmock.h"

#include "translation.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using namespace muse;
using namespace muse::audioplugins;
using namespace muse::io;

namespace muse::audioplugins {
class AudioPlugins_RegisterAudioPluginsScenarioTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_scenario = std::make_shared<RegisterAudioPluginsScenario>(modularity::globalCtx());
        m_globalConfiguration = std::make_shared<NiceMock<GlobalConfigurationMock> >();
        m_fileSystem = std::make_shared<NiceMock<io::FileSystemMock> >();
        m_interactive = std::make_shared<InteractiveMock>();
        m_process = std::make_shared<ProcessMock>();
        m_scannerRegister = std::make_shared<NiceMock<AudioPluginsScannerRegisterMock> >();
        m_knownPlugins = std::make_shared<NiceMock<KnownAudioPluginsRegisterMock> >();
        m_scanners = { std::make_shared<NiceMock<AudioPluginsScannerMock> >() };
        m_metaReaderRegister = std::make_shared<NiceMock<AudioPluginMetaReaderRegisterMock> >();
        m_loadGuard = std::make_shared<NiceMock<AudioPluginsLoadGuardMock> >();

        const auto metaReaderMock = std::make_shared<NiceMock<AudioPluginMetaReaderMock> >();
        m_metaReaders = { metaReaderMock };

        m_scenario->globalConfiguration.set(m_globalConfiguration);
        m_scenario->fileSystem.set(m_fileSystem);
        m_scenario->interactive.set(m_interactive);
        m_scenario->process.set(m_process);
        m_scenario->knownPluginsRegister.set(m_knownPlugins);
        m_scenario->scannerRegister.set(m_scannerRegister);
        m_scenario->metaReaderRegister.set(m_metaReaderRegister);
        m_scenario->loadGuard.set(m_loadGuard);

        ON_CALL(*m_globalConfiguration, appBinPath())
        .WillByDefault(Return(m_appPath));

        ON_CALL(*m_fileSystem, remove(_, _))
        .WillByDefault(Return(muse::make_ok()));

        ON_CALL(*m_fileSystem, temporaryDirectoryPath())
        .WillByDefault(Return(io::path_t("/tmp")));

        ON_CALL(*m_scannerRegister, scanners())
        .WillByDefault(ReturnRef(m_scanners));

        ON_CALL(*m_metaReaderRegister, readers())
        .WillByDefault(ReturnRef(m_metaReaders));

        ON_CALL(*metaReaderMock, metaType())
        .WillByDefault(Return("VstPlugin"));

        ON_CALL(*metaReaderMock, canReadMeta(_))
        .WillByDefault(Return(true));

        ON_CALL(*m_knownPlugins, setPluginsState(_, _))
        .WillByDefault(Return(muse::make_ok()));

        ON_CALL(*m_knownPlugins, removePluginsAtPath(_))
        .WillByDefault(Return(muse::make_ok()));

        ON_CALL(*m_knownPlugins, registerPlugins(_))
        .WillByDefault(Return(muse::make_ok()));

        ON_CALL(*m_knownPlugins, unregisterPlugins(_))
        .WillByDefault(Return(muse::make_ok()));

        ON_CALL(*m_knownPlugins, writePluginsTo(_, _))
        .WillByDefault(Return(muse::make_ok()));

        ON_CALL(*m_knownPlugins, readPluginsFrom(_))
        .WillByDefault(Return(RetVal<AudioPluginInfoList>::make_ok({})));
    }

    std::shared_ptr<RegisterAudioPluginsScenario> m_scenario;
    std::shared_ptr<GlobalConfigurationMock> m_globalConfiguration;
    std::shared_ptr<io::FileSystemMock> m_fileSystem;
    std::shared_ptr<InteractiveMock> m_interactive;
    std::shared_ptr<ProcessMock> m_process;
    std::shared_ptr<KnownAudioPluginsRegisterMock> m_knownPlugins;
    std::shared_ptr<AudioPluginsScannerRegisterMock> m_scannerRegister;
    std::vector<IAudioPluginsScannerPtr> m_scanners;
    std::shared_ptr<AudioPluginMetaReaderRegisterMock> m_metaReaderRegister;
    std::shared_ptr<AudioPluginsLoadGuardMock> m_loadGuard;
    std::vector<IAudioPluginMetaReaderPtr> m_metaReaders;

    std::string m_appPath = "/some/path/to/MuseScore";
};

inline bool operator==(const AudioPluginInfo& info1, const AudioPluginInfo& info2)
{
    return info1.path == info2.path
           && info1.meta == info2.meta
           && info1.state == info2.state
           && info1.errorCode == info2.errorCode;
}
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, Init)
{
    // [THEN] The register is inited
    EXPECT_CALL(*m_knownPlugins, load())
    .WillOnce(Return(muse::make_ok()));

    // [WHEN] Init the scenario
    m_scenario->init();
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, UpdatePluginsRegistry)
{
    // [GIVEN] All found plugins
    paths_t foundPluginPaths {
        "/some/test/path/to/plugin/AAA.vst3", // already registered
        "/some/test/path/to/plugin/BBB.vst3", // already registered
        "/some/test/path/to/plugin/CCC.vst3",
        "/some/test/path/to/plugin/DDD.vst3",
        "/some/test/path/to/plugin/FFF.vst3", // incompatible (will crash)
    };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    AudioPluginInfo incompatiblePluginInfo;
    incompatiblePluginInfo.path = foundPluginPaths[4];
    incompatiblePluginInfo.meta.id = io::filename(incompatiblePluginInfo.path).toStdString();
    incompatiblePluginInfo.state = AudioPluginState::Error;
    incompatiblePluginInfo.errorCode = -1;

    // [GIVEN] Some plugins already exist in the register
    AudioPluginInfoList alreadyRegisteredPlugins;
    for (size_t i = 0; i < 2; ++i) {
        AudioPluginInfo info;
        info.path = foundPluginPaths[i];
        info.meta.id = io::completeBasename(foundPluginPaths[i]).toStdString();
        alreadyRegisteredPlugins.push_back(info);
    }

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(alreadyRegisteredPlugins));

    // [THEN] The progress bar is shown
    EXPECT_CALL(*m_interactive, showProgress(muse::trc("audio", "Validating audio plugins"), _))
    .Times(1);

    // [THEN] Processes started only for unregistered plugins; each gets an
    // --register-audio-plugin-out file, the main process is the sole cache writer.
    paths_t alreadyRegisteredPaths { foundPluginPaths[0], foundPluginPaths[1] };
    for (const path_t& pluginPath : foundPluginPaths) {
        auto argsMatch = ElementsAre("--register-audio-plugin", pluginPath.toStdString(), "--register-audio-plugin-out", _);

        if (muse::contains(alreadyRegisteredPaths, pluginPath)) {
            // Ignore already registered plugins
            EXPECT_CALL(*m_process, execute(_, argsMatch, _, _))
            .Times(0);
        } else if (incompatiblePluginInfo.path == pluginPath) {
            // Incompatible plugin: subprocess fails, main records the failure
            EXPECT_CALL(*m_process, execute(m_appPath, argsMatch, _, _))
            .WillOnce(Return(-1));
        } else {
            // Successfully registered plugins
            EXPECT_CALL(*m_process, execute(m_appPath, argsMatch, _, _))
            .WillOnce(Return(0));
        }
    }

    // [THEN] Completed Discovered placeholders are removed in one batch before
    // scanned results are registered.
    EXPECT_CALL(*m_knownPlugins, unregisterPlugins(_))
    .WillOnce(Return(make_ok()));

    // [THEN] The register is refreshed
    EXPECT_CALL(*m_knownPlugins, load())
    .Times(2)
    .WillRepeatedly(Return(muse::make_ok()));

    // [WHEN] Register new plugins
    Ret ret = m_scenario->updatePluginsRegistry();

    // [THEN] Plugins successfully registered
    EXPECT_TRUE(ret);
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, UpdatePluginsRegistry_NoNewPlugins)
{
    // [GIVEN] All found plugins (all are already registered)
    paths_t foundPluginPaths {
        "/some/test/path/to/plugin/AAA.vst3",
        "/some/test/path/to/plugin/BBB.vst3",
        "/some/test/path/to/plugin/CCC.vst3",
    };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    AudioPluginInfoList alreadyRegisteredPlugins;
    for (const path_t& pluginPath : foundPluginPaths) {
        AudioPluginInfo info;
        info.path = pluginPath;
        info.meta.id = io::completeBasename(pluginPath).toStdString();
        alreadyRegisteredPlugins.push_back(info);
    }

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(alreadyRegisteredPlugins));

    // [THEN] Don't register the plugins again
    EXPECT_CALL(*m_process, execute(_, _))
    .Times(0);
    EXPECT_CALL(*m_process, execute(_, _, _))
    .Times(0);
    EXPECT_CALL(*m_process, execute(_, _, _, _))
    .Times(0);

    EXPECT_CALL(*m_interactive, showProgress(_, _))
    .Times(0);

    EXPECT_CALL(*m_knownPlugins, load())
    .WillOnce(Return(muse::make_ok()));

    // [WHEN] Try to register the plugins again
    Ret ret = m_scenario->updatePluginsRegistry();

    // [THEN] No error
    EXPECT_TRUE(ret);
}

//! See: https://github.com/musescore/MuseScore/issues/16458
TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, UpdatePluginsRegistry_MarkUninstalledAsMissing)
{
    auto createPluginInfo = [](const io::path_t& path, AudioPluginState state = AudioPluginState::Validated) {
        AudioPluginInfo info;
        info.meta.id = io::completeBasename(path).toStdString();
        info.meta.type = "VstPlugin";
        info.path = path;
        info.state = state;
        return info;
    };

    // [GIVEN] Already registered plugins
    AudioPluginInfoList knownPlugins;
    knownPlugins.push_back(createPluginInfo("/some/path/AAA.vst3"));
    knownPlugins.push_back(createPluginInfo("/some/path/BBB.vst3"));
    knownPlugins.push_back(createPluginInfo("/some/path/CCC.vst3"));
    knownPlugins.push_back(createPluginInfo("/some/path/DDD.vst3"));

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(knownPlugins));

    // [GIVEN] Scanner only finds some plugins (AAA and BBB have been uninstalled)
    paths_t foundPluginPaths {
        "/some/path/CCC.vst3",
        "/some/path/DDD.vst3",
    };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    // [THEN] Uninstalled plugins transition to Missing (kept in cache)
    paths_t uninstalledPluginPaths {
        knownPlugins[0].path, knownPlugins[1].path
    };

    EXPECT_CALL(*m_knownPlugins, setPluginsState(uninstalledPluginPaths, AudioPluginState::Missing))
    .WillOnce(Return(make_ok()));

    EXPECT_CALL(*m_knownPlugins, unregisterPlugins(_))
    .Times(0);

    EXPECT_CALL(*m_knownPlugins, load())
    .WillOnce(Return(muse::make_ok()));

    // [WHEN] Update registry
    Ret ret = m_scenario->updatePluginsRegistry();

    // [THEN] Successfully transitioned
    EXPECT_TRUE(ret);
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, ScanPlugins_FormerlyMissingFoundAgainIsRevalidated)
{
    auto createPluginInfo = [](const io::path_t& path, AudioPluginState state) {
        AudioPluginInfo info;
        info.meta.id = io::completeBasename(path).toStdString();
        info.meta.type = "VstPlugin";
        info.path = path;
        info.state = state;
        return info;
    };

    // [GIVEN] One Missing entry that reappears, one untouched Validated entry
    AudioPluginInfoList knownPlugins;
    knownPlugins.push_back(createPluginInfo("/some/path/AAA.vst3", AudioPluginState::Missing));
    knownPlugins.push_back(createPluginInfo("/some/path/BBB.vst3", AudioPluginState::Validated));

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(knownPlugins));

    // [GIVEN] Scanner now finds both
    paths_t foundPluginPaths {
        "/some/path/AAA.vst3",
        "/some/path/BBB.vst3",
    };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    // [WHEN] Scanning
    const PluginScanResult result = m_scenario->scanPlugins();

    // [THEN] The reappeared plugin is re-validated rather than trusted back
    // to Validated; the untouched Validated plugin is left alone.
    EXPECT_TRUE(muse::contains(result.newPluginPaths, path_t("/some/path/AAA.vst3")));
    EXPECT_FALSE(muse::contains(result.newPluginPaths, path_t("/some/path/BBB.vst3")));
    EXPECT_TRUE(result.missingPluginPaths.empty());
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, UpdatePluginsRegistry_MissingMultiPluginBinaryReportedOnceByPath)
{
    auto createPluginInfo = [](const io::path_t& path, const PluginResourceId& id, AudioPluginState state) {
        AudioPluginInfo info;
        info.meta.id = id;
        info.meta.type = "VstPlugin";
        info.path = path;
        info.state = state;
        return info;
    };

    // [GIVEN] One path hosting two plugin ids (shell bundle), plus a standalone plugin
    const io::path_t shellPath = "/some/path/SHELL.vst3";
    const io::path_t soloPath = "/some/path/SOLO.vst3";

    AudioPluginInfoList knownPlugins;
    knownPlugins.push_back(createPluginInfo(shellPath, "Shell FxA", AudioPluginState::Validated));
    knownPlugins.push_back(createPluginInfo(shellPath, "Shell FxB", AudioPluginState::Validated));
    knownPlugins.push_back(createPluginInfo(soloPath, "Solo Fx", AudioPluginState::Validated));

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(knownPlugins));

    // [GIVEN] The scanner no longer finds the shell binary; the solo plugin stays.
    paths_t foundPluginPaths { soloPath };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    // [THEN] The vanished binary is reported once by path; flipping every id
    // under it is the register's job.
    paths_t expectedMissing { shellPath };

    EXPECT_CALL(*m_knownPlugins, setPluginsState(expectedMissing, AudioPluginState::Missing))
    .WillOnce(Return(make_ok()));

    EXPECT_CALL(*m_knownPlugins, load())
    .WillOnce(Return(make_ok()));

    Ret ret = m_scenario->updatePluginsRegistry();
    EXPECT_TRUE(ret);
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, ScanPlugins_FormerlyMissingMultiIdBinaryQueuedOnce)
{
    auto createPluginInfo = [](const io::path_t& path, const PluginResourceId& id, AudioPluginState state) {
        AudioPluginInfo info;
        info.meta.id = id;
        info.meta.type = "VstPlugin";
        info.path = path;
        info.state = state;
        return info;
    };

    // [GIVEN] A bundle whose two ids are both currently Missing
    const io::path_t shellPath = "/some/path/SHELL.vst3";

    AudioPluginInfoList knownPlugins;
    knownPlugins.push_back(createPluginInfo(shellPath, "Shell FxA", AudioPluginState::Missing));
    knownPlugins.push_back(createPluginInfo(shellPath, "Shell FxB", AudioPluginState::Missing));

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(knownPlugins));

    // [GIVEN] The scanner finds the binary again.
    paths_t foundPluginPaths { shellPath };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    // [WHEN] Scanning
    const PluginScanResult result = m_scenario->scanPlugins();

    // [THEN] The reappeared binary is queued once for re-validation; nothing
    // is trusted back to Validated, nothing is left Missing.
    EXPECT_EQ(result.newPluginPaths, paths_t { shellPath });
    EXPECT_TRUE(result.missingPluginPaths.empty());
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, UpdatePluginsRegistry_LeftoverDiscoveredRevalidates)
{
    // [GIVEN] A leftover Discovered entry; the scanner still sees the path on disk
    auto createPluginInfo = [](const io::path_t& path, AudioPluginState state) {
        AudioPluginInfo info;
        info.meta.id = io::completeBasename(path).toStdString();
        info.meta.type = "VstPlugin";
        info.path = path;
        info.state = state;
        return info;
    };

    AudioPluginInfoList knownPlugins;
    knownPlugins.push_back(createPluginInfo("/some/path/CRASHED.vst3", AudioPluginState::Discovered));

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(knownPlugins));

    paths_t foundPluginPaths { "/some/path/CRASHED.vst3" };

    for (const IAudioPluginsScannerPtr& scanner : m_scanners) {
        AudioPluginsScannerMock* mock = dynamic_cast<AudioPluginsScannerMock*>(scanner.get());
        ASSERT_TRUE(mock);

        ON_CALL(*mock, scanPlugins(_))
        .WillByDefault(Return(foundPluginPaths));
    }

    // [THEN] The Discovered path is re-validated, not marked Missing
    EXPECT_CALL(*m_knownPlugins, setPluginsState(paths_t {}, AudioPluginState::Missing))
    .WillOnce(Return(make_ok()));

    // [THEN] A Discovered placeholder is written before spawning the subprocess;
    // completed results land in a later registerPlugins call (the catch-all below).
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(AnyNumber())
    .WillRepeatedly(Return(make_ok()));
    AudioPluginInfo expectedPlaceholder = createPluginInfo("/some/path/CRASHED.vst3",
                                                           AudioPluginState::Discovered);
    EXPECT_CALL(*m_knownPlugins, registerPlugins(AudioPluginInfoList { expectedPlaceholder }))
    .WillOnce(Return(make_ok()));

    // [THEN] The path is cleared before writing the Discovered placeholder;
    // completed placeholders are removed in one batch.
    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(io::path_t("/some/path/CRASHED.vst3")))
    .WillOnce(Return(make_ok()));

    EXPECT_CALL(*m_process, execute(m_appPath,
                                    ElementsAre("--register-audio-plugin", "/some/path/CRASHED.vst3", "--register-audio-plugin-out", _), _,
                                    _))
    .WillOnce(Return(0));

    // [THEN] register loaded twice (once after registerNewPlugins, once at end of updatePluginsRegistry)
    EXPECT_CALL(*m_knownPlugins, load())
    .Times(2)
    .WillRepeatedly(Return(make_ok()));

    Ret ret = m_scenario->updatePluginsRegistry();
    EXPECT_TRUE(ret);
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, ValidatePlugin)
{
    // [GIVEN] A plugin to validate out-of-process and the result file to write
    path_t pluginPath = "/some/test/path/to/plugin/AAA.vst3";
    path_t outputFile = "/tmp/scan_result.json";

    PluginMetaList metaList;

    PluginMeta pluginMeta1;
    pluginMeta1.id = "Mono plugin";
    pluginMeta1.attributes.insert({ String(u"categories"), u"Fx|Mono" });
    metaList.push_back(pluginMeta1);

    PluginMeta pluginMeta2;
    pluginMeta2.id = "Stereo plugin";
    pluginMeta2.attributes.insert({ String(u"categories"), u"Fx|Stereo" });
    metaList.push_back(pluginMeta2);

    ASSERT_FALSE(m_metaReaders.empty());
    AudioPluginMetaReaderMock* mock = dynamic_cast<AudioPluginMetaReaderMock*>(m_metaReaders[0].get());
    ASSERT_TRUE(mock);

    ON_CALL(*mock, readMeta(pluginPath))
    .WillByDefault(Return(RetVal<PluginMetaList>::make_ok(metaList)));

    // [THEN] The validated metadata is written to the result file
    AudioPluginInfoList expectedInfoList;
    expectedInfoList.reserve(metaList.size());

    for (const PluginMeta& meta : metaList) {
        AudioPluginInfo expectedPluginInfo;
        expectedPluginInfo.meta = meta;
        expectedPluginInfo.path = pluginPath;
        expectedPluginInfo.state = AudioPluginState::Validated;
        expectedPluginInfo.errorCode = 0;
        expectedInfoList.emplace_back(std::move(expectedPluginInfo));
    }

    EXPECT_CALL(*m_knownPlugins, writePluginsTo(outputFile, expectedInfoList))
    .WillOnce(Return(make_ok()));

    // [THEN] The subprocess does not touch the cache; the main process owns it
    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(_))
    .Times(0);
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(0);

    // [WHEN] Validate the plugin
    Ret ret = m_scenario->validatePlugin(pluginPath, outputFile);

    // [THEN] Validation succeeded
    EXPECT_TRUE(ret);
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, RegisterPlugin)
{
    // [GIVEN] A trusted plugin registered in-process (no subprocess)
    path_t pluginPath = "/some/test/path/to/plugin/AAA.vst3";

    PluginMetaList metaList;
    PluginMeta pluginMeta;
    pluginMeta.id = "Mono plugin";
    pluginMeta.attributes.insert({ String(u"categories"), u"Fx|Mono" });
    metaList.push_back(pluginMeta);

    ASSERT_FALSE(m_metaReaders.empty());
    AudioPluginMetaReaderMock* mock = dynamic_cast<AudioPluginMetaReaderMock*>(m_metaReaders[0].get());
    ASSERT_TRUE(mock);

    ON_CALL(*mock, readMeta(pluginPath))
    .WillByDefault(Return(RetVal<PluginMetaList>::make_ok(metaList)));

    AudioPluginInfo expectedPluginInfo;
    expectedPluginInfo.meta = pluginMeta;
    expectedPluginInfo.path = pluginPath;
    expectedPluginInfo.state = AudioPluginState::Validated;

    // [THEN] The placeholder is cleared, then the validated entry is written
    // straight to the register (no result file).
    ::testing::InSequence seq;
    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(pluginPath))
    .WillOnce(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, registerPlugins(AudioPluginInfoList { expectedPluginInfo }))
    .WillOnce(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, writePluginsTo(_, _))
    .Times(0);

    // [WHEN] Register the plugin in-process
    Ret ret = m_scenario->registerPlugin(pluginPath);

    // [THEN] Registration succeeded
    EXPECT_TRUE(ret);
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, FailedValidationRecordsErrorEntry)
{
    // [GIVEN] A new plugin whose subprocess validation fails
    path_t pluginPath = "/some/test/path/to/plugin/AAA.vst3";
    paths_t paths { pluginPath };

    ON_CALL(*m_knownPlugins, load())
    .WillByDefault(Return(make_ok()));

    // [GIVEN] The subprocess exits with a failure code
    EXPECT_CALL(*m_process, execute(m_appPath,
                                    ElementsAre("--register-audio-plugin",
                                                pluginPath.toStdString(), "--register-audio-plugin-out", _), _, _))
    .WillOnce(Return(-42));

    // [THEN] The main process records the Error entry itself (no second
    // subprocess), with the basename as id and the exit code.
    AudioPluginInfo expectedError;
    expectedError.meta.id = io::completeBasename(pluginPath).toStdString();
    expectedError.meta.type = "VstPlugin";
    expectedError.path = pluginPath;
    expectedError.state = AudioPluginState::Error;
    expectedError.errorCode = -42;

    // the placeholder write is absorbed by this catch-all; the Error entry
    // below is the assertion
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(AnyNumber())
    .WillRepeatedly(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, registerPlugins(AudioPluginInfoList { expectedError }))
    .WillOnce(Return(make_ok()));

    // [WHEN] Register the new plugin with validation enabled
    EXPECT_TRUE(m_scenario->registerNewPlugins(paths, /*validate*/ true));
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, MarkCrashedPluginsAsBroken)
{
    // [GIVEN] Two validated plugins; loading the first one crashed the
    // application in a previous run, leaving a dangling load sentinel
    AudioPluginInfo crashedPlugin;
    crashedPlugin.meta.id = "CrashingPlugin";
    crashedPlugin.meta.type = "VstPlugin";
    crashedPlugin.path = "/some/path/AAA.vst3";
    crashedPlugin.state = AudioPluginState::Validated;

    AudioPluginInfo healthyPlugin;
    healthyPlugin.meta.id = "HealthyPlugin";
    healthyPlugin.meta.type = "VstPlugin";
    healthyPlugin.path = "/some/path/BBB.vst3";
    healthyPlugin.state = AudioPluginState::Validated;

    ON_CALL(*m_knownPlugins, pluginInfoList(_))
    .WillByDefault(Return(AudioPluginInfoList { crashedPlugin, healthyPlugin }));

    ON_CALL(*m_loadGuard, danglingLoads())
    .WillByDefault(Return(PluginResourceIdList { crashedPlugin.meta.id }));

    // [THEN] Only the crashed plugin is re-registered, as broken
    AudioPluginInfo expectedBroken = crashedPlugin;
    expectedBroken.state = AudioPluginState::Error;
    expectedBroken.errorCode = static_cast<int>(Err::PluginCrashedOnLoad);

    ::testing::InSequence seq;
    EXPECT_CALL(*m_knownPlugins, unregisterPlugins(PluginResourceIdList { crashedPlugin.meta.id }))
    .WillOnce(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, registerPlugins(AudioPluginInfoList { expectedBroken }))
    .WillOnce(Return(make_ok()));

    // [THEN] The sentinels are cleared
    EXPECT_CALL(*m_loadGuard, clearDanglingLoads())
    .WillOnce(Return(make_ok()));

    // [WHEN] Handle crashed plugins
    EXPECT_TRUE(m_scenario->markCrashedPluginsAsBroken());
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, MarkCrashedPluginsAsBroken_NoDanglingLoads)
{
    // [GIVEN] No dangling load sentinels
    ON_CALL(*m_loadGuard, danglingLoads())
    .WillByDefault(Return(PluginResourceIdList {}));

    // [THEN] The register is left untouched
    EXPECT_CALL(*m_knownPlugins, unregisterPlugins(_))
    .Times(0);
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(0);
    EXPECT_CALL(*m_loadGuard, clearDanglingLoads())
    .Times(0);

    // [WHEN] Handle crashed plugins
    EXPECT_TRUE(m_scenario->markCrashedPluginsAsBroken());
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, TimedOutValidationRecordsErrorEntry)
{
    // [GIVEN] A new plugin whose subprocess validation times out
    path_t pluginPath = "/some/test/path/to/plugin/MuseHub.vst3";
    paths_t paths { pluginPath };

    ON_CALL(*m_knownPlugins, load())
    .WillByDefault(Return(make_ok()));

    // [GIVEN] The subprocess wrapper killed the hung validator
    EXPECT_CALL(*m_process, execute(m_appPath,
                                    ElementsAre("--register-audio-plugin",
                                                pluginPath.toStdString(), "--register-audio-plugin-out", _), _, _))
    .WillOnce(Return(muse::IProcess::ExecuteTimeoutCode));

    // [THEN] The plugin is recorded as failed with the timeout code
    AudioPluginInfo expectedError;
    expectedError.meta.id = io::completeBasename(pluginPath).toStdString();
    expectedError.meta.type = "VstPlugin";
    expectedError.path = pluginPath;
    expectedError.state = AudioPluginState::Error;
    expectedError.errorCode = muse::IProcess::ExecuteTimeoutCode;

    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(AnyNumber())
    .WillRepeatedly(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, registerPlugins(AudioPluginInfoList { expectedError }))
    .WillOnce(Return(make_ok()));

    EXPECT_TRUE(m_scenario->registerNewPlugins(paths, /*validate*/ true));
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, CanceledValidationDoesNotRecordErrorEntry)
{
    // [GIVEN] A new plugin is being validated when the user cancels scanning.
    path_t pluginPath = "/some/test/path/to/plugin/Canceled.vst3";
    paths_t paths { pluginPath };

    ON_CALL(*m_knownPlugins, load())
    .WillByDefault(Return(make_ok()));

    // init() wires the progress cancellation notification to m_aborted.
    m_scenario->init();

    Progress progress;
    EXPECT_CALL(*m_interactive, showProgress(_, _))
    .WillOnce(::testing::SaveArg<1>(&progress));

    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(pluginPath))
    .WillOnce(Return(make_ok()));

    // Only the Discovered placeholder should be registered. The canceled
    // validator must not be converted into an Error cache entry.
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(1)
    .WillOnce(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, unregisterPlugins(_))
    .Times(0);
    EXPECT_CALL(*m_knownPlugins, readPluginsFrom(_))
    .Times(0);

    EXPECT_CALL(*m_process, execute(m_appPath,
                                    ElementsAre("--register-audio-plugin",
                                                pluginPath.toStdString(), "--register-audio-plugin-out", _), _, _))
    .WillOnce([&progress](const std::string&, const std::vector<std::string>&, int,
                          const std::function<bool()>& shouldCancel) {
        progress.cancel();
        EXPECT_TRUE(shouldCancel());
        return IProcess::ExecuteCanceledCode;
    });

    EXPECT_TRUE(m_scenario->registerNewPlugins(paths, /*validate*/ true));
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, RegisterNewPlugins_ValidateFalsePersistsDiscoveredOnly)
{
    // [GIVEN] Two paths recorded without validating ("Skip this time")
    paths_t paths {
        "/some/path/AAA.vst3",
        "/some/path/BBB.vst3",
    };

    // [THEN] One removePluginsAtPath per path, then a single batch
    // registerPlugins of the Discovered placeholders.
    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(io::path_t("/some/path/AAA.vst3")))
    .WillOnce(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(io::path_t("/some/path/BBB.vst3")))
    .WillOnce(Return(make_ok()));
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .WillOnce(Return(make_ok()));

    // [THEN] No out-of-process validation: no subprocess, no progress dialog.
    EXPECT_CALL(*m_process, execute(_, _))
    .Times(0);
    EXPECT_CALL(*m_process, execute(_, _, _))
    .Times(0);
    EXPECT_CALL(*m_process, execute(_, _, _, _))
    .Times(0);
    EXPECT_CALL(*m_interactive, showProgress(_, _))
    .Times(0);

    // [THEN] Final load() resyncs the register.
    EXPECT_CALL(*m_knownPlugins, load())
    .WillOnce(Return(make_ok()));

    // [WHEN] Register with validation deferred
    EXPECT_TRUE(m_scenario->registerNewPlugins(paths, /*validate*/ false));
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, RegisterNewPlugins_MainAppliesEachResult)
{
    // [GIVEN] Three new plugin paths to scan
    paths_t paths {
        "/some/path/AAA.vst3",
        "/some/path/BBB.vst3",
        "/some/path/CCC.vst3",
    };

    // [THEN] One removePluginsAtPath per path; completed placeholders are
    // removed in one unregisterPlugins batch.
    EXPECT_CALL(*m_knownPlugins, removePluginsAtPath(_))
    .Times(3)
    .WillRepeatedly(Return(make_ok()));

    EXPECT_CALL(*m_knownPlugins, unregisterPlugins(_))
    .WillOnce(Return(make_ok()));

    // [THEN] registerPlugins is called once for the placeholder batch plus once
    // for the collected subprocess results.
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .Times(2)
    .WillRepeatedly(Return(make_ok()));

    // [THEN] One subprocess invocation per path, each handed an --register-audio-plugin-out file
    for (const path_t& path : paths) {
        EXPECT_CALL(*m_process, execute(m_appPath,
                                        ElementsAre("--register-audio-plugin", path.toStdString(), "--register-audio-plugin-out", _), _, _))
        .WillOnce(Return(0));
    }

    // [THEN] Each successful result is read back from its file
    EXPECT_CALL(*m_knownPlugins, readPluginsFrom(_))
    .Times(3)
    .WillRepeatedly(Return(RetVal<AudioPluginInfoList>::make_ok({})));

    EXPECT_CALL(*m_interactive, showProgress(_, _))
    .Times(1);

    EXPECT_CALL(*m_knownPlugins, load())
    .WillOnce(Return(make_ok()));

    // [WHEN] Register with validation enabled
    EXPECT_TRUE(m_scenario->registerNewPlugins(paths, /*validate*/ true));
}

TEST_F(AudioPlugins_RegisterAudioPluginsScenarioTest, RegisterNewPlugins_TrailingSlashPathGetsNonEmptyId)
{
    // [GIVEN] An LV2 "<uri>@<bundle>/" composite path: completeBasename() is
    // empty, and load() would reject a row with an empty id.
    const path_t lv2Path = "http://calf.sourceforge.net/plugins/Analyzer@file:///usr/lib/lv2/calf.lv2/";
    paths_t paths { lv2Path };

    ON_CALL(*m_knownPlugins, removePluginsAtPath(_))
    .WillByDefault(Return(make_ok()));
    ON_CALL(*m_knownPlugins, load())
    .WillByDefault(Return(make_ok()));

    // [THEN] Capture the persisted Discovered placeholder
    AudioPluginInfoList persisted;
    EXPECT_CALL(*m_knownPlugins, registerPlugins(_))
    .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&persisted), Return(make_ok())));

    // [WHEN] Register with validation deferred (persistDiscoveredPlaceholders only)
    EXPECT_TRUE(m_scenario->registerNewPlugins(paths, /*validate*/ false));

    // [THEN] The placeholder id falls back to the full path, not empty
    ASSERT_EQ(persisted.size(), size_t(1));
    EXPECT_FALSE(persisted.front().meta.id.empty());
    EXPECT_EQ(persisted.front().meta.id, lv2Path.toStdString());
    EXPECT_EQ(persisted.front().state, AudioPluginState::Discovered);
}
