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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QQmlContext>
#include <QQmlEngine>

#include "modularity/ioc.h"

#include "macosshortcutsinstancemodel.h"

#include "mocks/commandshortcutsregistermock.h"
#include "mocks/shortcutscontrollermock.h"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using namespace muse;
using namespace muse::shortcuts;

namespace muse::shortcuts {
static const QString US_LAYOUT = "com.apple.keylayout.US";
static const QString GERMAN_LAYOUT = "com.apple.keylayout.German";
static const QString RUSSIAN_LAYOUT = "com.apple.keylayout.Russian";

class TestMacOSShortcutsInstanceModel : public MacOSShortcutsInstanceModel
{
public:
    TestMacOSShortcutsInstanceModel(const QString& keyboardLayoutId, QObject* parent)
        : MacOSShortcutsInstanceModel(parent), m_keyboardLayoutId(keyboardLayoutId) {}

protected:
    QString keyboardLayoutId() const override
    {
        return m_keyboardLayoutId;
    }

private:
    QString m_keyboardLayoutId;
};

class Shortcuts_MacOSShortcutsInstanceModelTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_iocCtx = std::make_shared<modularity::Context>(1);

        m_shortcutsRegister = std::make_shared<NiceMock<CommandShortcutsRegisterMock> >();
        m_controller = std::make_shared<NiceMock<ShortcutsControllerMock> >();

        modularity::globalIoc()->registerExport<ICommandShortcutsRegister>("utests", m_shortcutsRegister);
        modularity::ioc(m_iocCtx)->registerExport<IShortcutsController>("utests", m_controller);

        ON_CALL(*m_shortcutsRegister, shortcuts())
        .WillByDefault(ReturnRef(m_shortcuts));

        //! NOTE The model gets the IoC context from the QML context it belongs to
        m_engine = std::make_unique<QQmlEngine>();
        QmlIoCContext* qmlIoc = new QmlIoCContext(m_engine.get());
        qmlIoc->ctx = m_iocCtx;
        m_engine->rootContext()->setContextProperty("ioc_context", QVariant::fromValue(qmlIoc));
    }

    void TearDown() override
    {
        m_model.reset();
        m_engine.reset();

        modularity::globalIoc()->unregister<ICommandShortcutsRegister>("utests");
        modularity::removeIoC(m_iocCtx);
    }

    void addShortcut(const std::string& command, const std::vector<std::string>& sequences)
    {
        Shortcut shortcut;
        shortcut.command = rcommand::Command(command);
        shortcut.sequences = sequences;
        m_shortcuts.push_back(shortcut);
    }

    void loadShortcuts(const QString& keyboardLayoutId)
    {
        m_model = std::make_unique<TestMacOSShortcutsInstanceModel>(keyboardLayoutId, nullptr);
        QQmlEngine::setContextForObject(m_model.get(), m_engine->rootContext());
        m_model->init();
    }

    QStringList registeredSequences() const
    {
        return m_model->shortcuts().keys();
    }

    //! NOTE Simulates pressing the shortcut: `sequence` is what Qt sees and passes to the QML `Shortcut` object,
    //! returns the sequence that is passed to the shortcuts controller
    std::string activate(const QString& sequence)
    {
        std::string activated;
        EXPECT_CALL(*m_controller, activate(::testing::_))
        .WillOnce([&activated](const std::string& seq) {
            activated = seq;
        });

        m_model->activate(sequence);

        ::testing::Mock::VerifyAndClearExpectations(m_controller.get());

        return activated;
    }

    modularity::ContextPtr m_iocCtx;
    std::shared_ptr<NiceMock<CommandShortcutsRegisterMock> > m_shortcutsRegister;
    std::shared_ptr<NiceMock<ShortcutsControllerMock> > m_controller;

    ShortcutList m_shortcuts;

    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<TestMacOSShortcutsInstanceModel> m_model;
};
}

/**
 * @brief Shortcuts with a character that requires Shift must not be translated to the character of the same key without Shift
 * @details `:` is `Shift+;` on the US layout. It used to be translated to `;`, so the shortcut with `:` took over `;`
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, US_ColonDoesNotOverrideSemicolon)
{
    //! [GIVEN] `:` comes before `;`, so its wrong translation to `;` would be recorded first
    addShortcut("test://a", { ":" });
    addShortcut("test://b", { ";" });

    //! [WHEN] The shortcuts are loaded for the US layout
    loadShortcuts(US_LAYOUT);

    //! [THEN] Both sequences are registered
    EXPECT_TRUE(registeredSequences().contains(":"));
    EXPECT_TRUE(registeredSequences().contains(";"));

    //! [THEN] Each one activates its own shortcut
    EXPECT_EQ(activate(";"), ";");
    EXPECT_EQ(activate(":"), ":");
}

TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, US_SemicolonDoesNotOverrideColon)
{
    //! [GIVEN] The same shortcuts in the reverse order
    addShortcut("test://b", { ";" });
    addShortcut("test://a", { ":" });

    //! [WHEN] The shortcuts are loaded for the US layout
    loadShortcuts(US_LAYOUT);

    //! [THEN] Each sequence activates its own shortcut
    EXPECT_EQ(activate(";"), ";");
    EXPECT_EQ(activate(":"), ":");
}

/**
 * @brief The other default shortcuts with a character that requires Shift on the US layout
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, US_ShiftedCharactersDoNotOverrideUnshifted)
{
    //! [GIVEN] Pairs of shortcuts with a character and the character of the same key without Shift
    const std::vector<std::pair<std::string, std::string> > pairs = {
        { "_", "-" },
        { "(", "9" },
        { "+", "=" },
        { "Alt++", "Alt+=" },
        { "{", "[" },
        { "}", "]" },
    };

    for (size_t i = 0; i < pairs.size(); ++i) {
        addShortcut("test://shifted-" + std::to_string(i), { pairs[i].first });
        addShortcut("test://unshifted-" + std::to_string(i), { pairs[i].second });
    }

    //! [WHEN] The shortcuts are loaded for the US layout
    loadShortcuts(US_LAYOUT);

    //! [THEN] Each sequence activates its own shortcut
    for (const auto& [shifted, unshifted] : pairs) {
        EXPECT_EQ(activate(QString::fromStdString(unshifted)), unshifted);
        EXPECT_EQ(activate(QString::fromStdString(shifted)), shifted);
    }
}

TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, German_ColonDoesNotOverrideOtherShortcuts)
{
    //! [GIVEN] `:` is `Shift+.` and `;` is `Shift+,` on the German layout
    addShortcut("test://a", { ":" });
    addShortcut("test://b", { ";" });
    addShortcut("test://c", { "." });
    addShortcut("test://d", { "," });

    //! [WHEN] The shortcuts are loaded for the German layout
    loadShortcuts(GERMAN_LAYOUT);

    //! [THEN] Each sequence activates its own shortcut
    EXPECT_EQ(activate(":"), ":");
    EXPECT_EQ(activate(";"), ";");
    EXPECT_EQ(activate("."), ".");
    EXPECT_EQ(activate(","), ",");
}

/**
 * @brief Shortcuts with Latin letters are translated to the letters of a non-Latin layout typed by the same keys
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, Russian_LatinLettersAreTranslated)
{
    //! [GIVEN] Shortcuts with Latin letters
    addShortcut("test://a", { "Ctrl+A" });
    addShortcut("test://b", { "Ctrl+Z" });

    //! [WHEN] The shortcuts are loaded for the Russian layout
    loadShortcuts(RUSSIAN_LAYOUT);

    //! [THEN] The sequences are registered for the Cyrillic letters on the same keys
    EXPECT_TRUE(registeredSequences().contains("Ctrl+Ф"));
    EXPECT_TRUE(registeredSequences().contains("Ctrl+Я"));

    //! [THEN] Pressing the keys activates the original shortcuts
    EXPECT_EQ(activate("Ctrl+Ф"), "Ctrl+A");
    EXPECT_EQ(activate("Ctrl+Я"), "Ctrl+Z");
}

TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, Russian_ColonDoesNotOverrideSemicolon)
{
    //! [GIVEN] `;` is on the `Ж` key, while `:` is typed with Shift on the Russian layout
    addShortcut("test://a", { ":" });
    addShortcut("test://b", { ";" });

    //! [WHEN] The shortcuts are loaded for the Russian layout
    loadShortcuts(RUSSIAN_LAYOUT);

    //! [THEN] `;` is translated to the `Ж` key, and `:` is kept as is
    EXPECT_EQ(activate("Ж"), ";");
    EXPECT_EQ(activate(":"), ":");
}
