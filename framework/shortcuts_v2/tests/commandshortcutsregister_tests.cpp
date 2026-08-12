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
#include <fstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QTemporaryDir>

#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "global/io/file.h"
#include "global/serialization/json.h"

#include "internal/commandshortcutsregister.h"

#include "mocks/shortcutsconfigurationmock.h"
#include "multiwindows/tests/mocks/multiwindowsprovidermock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

using namespace muse;
using namespace muse::shortcuts;

namespace muse::shortcuts {
class Shortcuts_CommandShortcutsRegisterTests : public ::testing::Test, public async::Asyncable
{
public:
    void SetUp() override
    {
        m_userDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(m_userDir->isValid());
        m_userDirPath = m_userDir->path().toStdString();

        m_configuration = std::make_shared<NiceMock<ShortcutsConfigurationMock> >();
        m_multiWindowsProvider = std::make_shared<NiceMock<mi::MultiWindowsProviderMock> >();

        modularity::globalIoc()->registerExport<IShortcutsConfiguration>("utests", m_configuration);
        modularity::globalIoc()->registerExport<mi::IMultiWindowsProvider>("utests", m_multiWindowsProvider);

        ON_CALL(*m_configuration, defaultShortcutsName())
        .WillByDefault(Return("shortcuts"));

        ON_CALL(*m_configuration, availableShortcutsPresets())
        .WillByDefault(Return(std::vector<std::string> { "shortcuts_azerty" }));

        ON_CALL(*m_configuration, commandShortcutsAppDataPath(_))
        .WillByDefault(Invoke([](const std::string& name) {
            return io::path_t(std::string(SHORTCUTS_TESTDATA_DIR) + "/" + name + ".json");
        }));

        ON_CALL(*m_configuration, commandShortcutsUserAppDataPath(_))
        .WillByDefault(Invoke([this](const std::string& name) {
            return userPath(name);
        }));

        ON_CALL(*m_configuration, currentShortcutsPresetName())
        .WillByDefault(Invoke([this]() {
            return m_currentPresetName;
        }));

        ON_CALL(*m_configuration, currentShortcutsPresetNameChanged())
        .WillByDefault(Return(m_presetNameChanged));

        ON_CALL(*m_multiWindowsProvider, resourceChanged())
        .WillByDefault(Return(m_resourceChanged));
    }

    void TearDown() override
    {
        m_register.reset();

        modularity::globalIoc()->unregister<IShortcutsConfiguration>("utests");
        modularity::globalIoc()->unregister<mi::IMultiWindowsProvider>("utests");
    }

    void initRegister()
    {
        m_register = std::make_unique<CommandShortcutsRegister>();
        m_register->init();
    }

    io::path_t userPath(const std::string& name) const
    {
        return io::path_t(m_userDirPath + "/" + name + ".json");
    }

    void writeUserFile(const std::string& name, const std::string& content)
    {
        std::ofstream file(m_userDirPath + "/" + name + ".json");
        ASSERT_TRUE(file.is_open());
        file << content;
    }

    void changeCurrentPreset(const std::string& name)
    {
        m_currentPresetName = name;
        m_presetNameChanged.send(name);
    }

    static const Shortcut* findShortcut(const ShortcutList& list, const std::string& command)
    {
        auto it = std::find_if(list.begin(), list.end(), [&command](const Shortcut& sc) {
            return sc.command == command;
        });

        return it != list.end() ? &(*it) : nullptr;
    }

    static std::vector<std::string> seqs(std::initializer_list<std::string> list)
    {
        return std::vector<std::string>(list);
    }

protected:
    std::unique_ptr<QTemporaryDir> m_userDir;
    std::string m_userDirPath;

    std::string m_currentPresetName;
    async::Channel<std::string> m_presetNameChanged;
    async::Channel<std::string> m_resourceChanged;

    std::shared_ptr<NiceMock<ShortcutsConfigurationMock> > m_configuration;
    std::shared_ptr<NiceMock<mi::MultiWindowsProviderMock> > m_multiWindowsProvider;

    std::unique_ptr<CommandShortcutsRegister> m_register;
};

TEST_F(Shortcuts_CommandShortcutsRegisterTests, LoadsDefaultShortcuts)
{
    //! [GIVEN] Default shortcuts name, no preset, no user file

    //! [WHEN] The register is initialized
    initRegister();

    //! [THEN] The base shortcuts are loaded as is
    const ShortcutList& shortcuts = m_register->shortcuts();
    EXPECT_EQ(shortcuts.size(), 4u);

    const Shortcut* a = findShortcut(shortcuts, "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->sequences, seqs({ "Ctrl+A" }));
    EXPECT_EQ(a->scope, "TEST");

    const Shortcut* d = findShortcut(shortcuts, "test://d");
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->scope, "OTHER");
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, AppliesPlatformDiff)
{
    //! [GIVEN] The platform default shortcuts file is a diff from the base one
    ON_CALL(*m_configuration, defaultShortcutsName())
    .WillByDefault(Return("shortcuts_mac"));

    //! [WHEN] The register is initialized
    initRegister();

    //! [THEN] The overridden shortcut is changed, the rest are intact
    const ShortcutList& shortcuts = m_register->shortcuts();
    EXPECT_EQ(shortcuts.size(), 4u);

    const Shortcut* b = findShortcut(shortcuts, "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Alt+B" }));

    const Shortcut* a = findShortcut(shortcuts, "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->sequences, seqs({ "Ctrl+A" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, AppliesPresetDiff)
{
    //! [GIVEN] The azerty preset is selected
    m_currentPresetName = "shortcuts_azerty";

    //! [WHEN] The register is initialized
    initRegister();

    //! [THEN] The preset diff is applied on top of the base shortcuts
    const ShortcutList& shortcuts = m_register->shortcuts();
    EXPECT_EQ(shortcuts.size(), 4u);

    const Shortcut* c = findShortcut(shortcuts, "test://c");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->sequences, seqs({ "&", "Num+1" }));

    const Shortcut* b = findShortcut(shortcuts, "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Ctrl+B" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, AppliesPresetDiffOverPlatformDiff)
{
    //! [GIVEN] The platform diff and the preset are set at the same time
    ON_CALL(*m_configuration, defaultShortcutsName())
    .WillByDefault(Return("shortcuts_mac"));

    m_currentPresetName = "shortcuts_azerty";

    //! [WHEN] The register is initialized
    initRegister();

    //! [THEN] Both diffs are applied
    const ShortcutList& shortcuts = m_register->shortcuts();

    const Shortcut* b = findShortcut(shortcuts, "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Alt+B" }));

    const Shortcut* c = findShortcut(shortcuts, "test://c");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->sequences, seqs({ "&", "Num+1" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, AppliesUserDiff)
{
    //! [GIVEN] A user diff file for the default shortcuts
    writeUserFile("shortcuts", R"({ "TEST": [ { "command": "test://a", "sequences": ["X"] } ] })");

    //! [WHEN] The register is initialized
    initRegister();

    //! [THEN] The user diff is applied, the rest is default
    const ShortcutList& shortcuts = m_register->shortcuts();
    EXPECT_EQ(shortcuts.size(), 4u);

    const Shortcut* a = findShortcut(shortcuts, "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->sequences, seqs({ "X" }));

    const Shortcut* b = findShortcut(shortcuts, "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Ctrl+B" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, UserDiffIsPerPreset)
{
    //! [GIVEN] A user diff file for the default shortcuts, but the azerty preset is selected
    writeUserFile("shortcuts", R"({ "TEST": [ { "command": "test://a", "sequences": ["X"] } ] })");
    m_currentPresetName = "shortcuts_azerty";

    //! [WHEN] The register is initialized
    initRegister();

    //! [THEN] The default user diff is not applied
    const Shortcut* a = findShortcut(m_register->shortcuts(), "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->sequences, seqs({ "Ctrl+A" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, SetShortcutsWritesOnlyDiff)
{
    //! [GIVEN] The register with default shortcuts
    initRegister();

    //! [WHEN] One shortcut is modified
    ShortcutList modified = m_register->shortcuts();
    for (Shortcut& sc : modified) {
        if (sc.command == "test://a") {
            sc.sequences = seqs({ "Y" });
        }
    }

    ASSERT_TRUE(m_register->setShortcuts(modified));

    //! [THEN] The user file contains only the modified shortcut
    ByteArray data;
    ASSERT_TRUE(io::File::readFile(userPath("shortcuts"), data));

    JsonObject root = JsonDocument::fromJson(data).rootObject();
    EXPECT_EQ(root.keys().size(), 1u);

    JsonArray scopeArr = root.value("TEST").toArray();
    ASSERT_EQ(scopeArr.size(), 1u);
    EXPECT_EQ(scopeArr.at(0).toObject().value("command").toString().toStdString(), "test://a");

    //! [THEN] After reload the modified shortcut is restored, the rest are default
    m_register->reload();

    const ShortcutList& shortcuts = m_register->shortcuts();
    EXPECT_EQ(shortcuts.size(), 4u);

    const Shortcut* a = findShortcut(shortcuts, "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->sequences, seqs({ "Y" }));

    const Shortcut* b = findShortcut(shortcuts, "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Ctrl+B" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, RevertingAllChangesRemovesUserFile)
{
    //! [GIVEN] The register with a modified shortcut
    initRegister();

    ShortcutList defaultShortcuts = m_register->shortcuts();

    ShortcutList modified = defaultShortcuts;
    for (Shortcut& sc : modified) {
        if (sc.command == "test://a") {
            sc.sequences = seqs({ "Y" });
        }
    }

    ASSERT_TRUE(m_register->setShortcuts(modified));
    ASSERT_TRUE(io::File::exists(userPath("shortcuts")));

    //! [WHEN] The shortcut is reverted back to the default value
    ASSERT_TRUE(m_register->setShortcuts(defaultShortcuts));

    //! [THEN] The user file is removed
    EXPECT_FALSE(io::File::exists(userPath("shortcuts")));
    EXPECT_EQ(m_register->shortcuts().size(), 4u);
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, ClearedShortcutIsSaved)
{
    //! [GIVEN] The register with default shortcuts
    initRegister();

    //! [WHEN] One shortcut is cleared
    ShortcutList modified = m_register->shortcuts();
    for (Shortcut& sc : modified) {
        if (sc.command == "test://a") {
            sc.sequences = {};
        }
    }

    ASSERT_TRUE(m_register->setShortcuts(modified));

    //! [THEN] The explicit clear survives reload
    m_register->reload();

    const Shortcut* a = findShortcut(m_register->shortcuts(), "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(a->sequences.empty());
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, ResetShortcutsRemovesUserFile)
{
    //! [GIVEN] The register with a modified shortcut
    initRegister();

    ShortcutList modified = m_register->shortcuts();
    modified.front().sequences = seqs({ "Y" });
    ASSERT_TRUE(m_register->setShortcuts(modified));
    ASSERT_TRUE(io::File::exists(userPath("shortcuts")));

    //! [WHEN] The shortcuts are reset
    m_register->resetShortcuts();

    //! [THEN] The user file is removed and the defaults are restored
    EXPECT_FALSE(io::File::exists(userPath("shortcuts")));

    const Shortcut* a = findShortcut(m_register->shortcuts(), "test://a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->sequences, seqs({ "Ctrl+A" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, PresetChangeReloadsShortcuts)
{
    //! [GIVEN] The register with default shortcuts
    initRegister();

    bool changedNotified = false;
    m_register->shortcutsChanged().onNotify(this, [&changedNotified]() {
        changedNotified = true;
    });

    //! [WHEN] The current preset is changed
    changeCurrentPreset("shortcuts_azerty");

    //! [THEN] The register is reloaded with the preset diff applied
    EXPECT_TRUE(changedNotified);

    const Shortcut* c = findShortcut(m_register->shortcuts(), "test://c");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->sequences, seqs({ "&", "Num+1" }));

    //! [WHEN] The preset is changed back
    changeCurrentPreset("");

    //! [THEN] The defaults are restored
    c = findShortcut(m_register->shortcuts(), "test://c");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->sequences, seqs({ "1", "Num+1" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, UserDiffIsWrittenForActivePreset)
{
    //! [GIVEN] The register with the azerty preset selected
    m_currentPresetName = "shortcuts_azerty";
    initRegister();

    //! [WHEN] One shortcut is modified
    ShortcutList modified = m_register->shortcuts();
    for (Shortcut& sc : modified) {
        if (sc.command == "test://b") {
            sc.sequences = seqs({ "Z" });
        }
    }

    ASSERT_TRUE(m_register->setShortcuts(modified));

    //! [THEN] The user diff is written for the preset, not for the default shortcuts
    EXPECT_TRUE(io::File::exists(userPath("shortcuts_azerty")));
    EXPECT_FALSE(io::File::exists(userPath("shortcuts")));

    //! [THEN] After switching to the default shortcuts and back, the modification is restored
    changeCurrentPreset("");
    const Shortcut* b = findShortcut(m_register->shortcuts(), "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Ctrl+B" }));

    changeCurrentPreset("shortcuts_azerty");
    b = findShortcut(m_register->shortcuts(), "test://b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->sequences, seqs({ "Z" }));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, IsPresetEditedReflectsUserFile)
{
    //! [GIVEN] The register with default shortcuts, no user file
    initRegister();
    EXPECT_FALSE(m_register->isPresetEdited(""));

    //! [WHEN] One shortcut is modified
    ShortcutList modified = m_register->shortcuts();
    modified.front().sequences = seqs({ "Y" });
    ASSERT_TRUE(m_register->setShortcuts(modified));

    //! [THEN] The active set is edited
    EXPECT_TRUE(m_register->isPresetEdited(""));

    //! [WHEN] The shortcuts are reset
    m_register->resetShortcuts();

    //! [THEN] The active set is not edited anymore
    EXPECT_FALSE(m_register->isPresetEdited(""));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, BuiltinPresetsCannotBeDeleted)
{
    //! [GIVEN] The register and an edited builtin preset
    writeUserFile("shortcuts_azerty", R"({ "TEST": [ { "command": "test://a", "sequences": ["X"] } ] })");
    initRegister();

    //! [THEN] Neither the default set nor builtin presets can be deleted
    EXPECT_FALSE(m_register->canDeletePreset(""));
    EXPECT_FALSE(m_register->canDeletePreset("shortcuts_azerty"));

    //! [WHEN] Trying to delete a builtin preset anyway
    m_register->deletePreset("shortcuts_azerty");

    //! [THEN] Its user file is intact
    EXPECT_TRUE(io::File::exists(userPath("shortcuts_azerty")));
}

TEST_F(Shortcuts_CommandShortcutsRegisterTests, DeletePresetRemovesUserFileAndFallsBackToDefault)
{
    //! [GIVEN] A custom preset (user file without a builtin twin) is selected
    writeUserFile("my_preset", R"({ "TEST": [ { "command": "test://a", "sequences": ["X"] } ] })");
    m_currentPresetName = "my_preset";
    initRegister();

    EXPECT_TRUE(m_register->canDeletePreset("my_preset"));

    //! [WHEN] The preset is deleted
    EXPECT_CALL(*m_configuration, setCurrentShortcutsPresetName(std::string()));
    m_register->deletePreset("my_preset");

    //! [THEN] The user file is removed and the current preset is switched to default
    EXPECT_FALSE(io::File::exists(userPath("my_preset")));
}
}
