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

#include <QKeySequence>
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

    static QString normalized(const QString& sequence)
    {
        return QKeySequence::fromString(sequence, QKeySequence::PortableText).toString(QKeySequence::PortableText);
    }

    bool isRegistered(const QString& sequence) const
    {
        return m_model->shortcuts().contains(normalized(sequence));
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

        m_model->activate(normalized(sequence));

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
 * @brief Shortcuts with a character that requires Shift are translated to Shift and the key
 * @details `:` is `Shift+;` on the US layout. It used to be translated to `;`, so the shortcut with `:` took over `;`
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, US_ColonDoesNotOverrideSemicolon)
{
    //! [GIVEN] `:` comes before `;`, so its wrong translation to `;` would be recorded first
    addShortcut("test://a", { ":" });
    addShortcut("test://b", { ";" });

    //! [WHEN] The shortcuts are loaded for the US layout
    loadShortcuts(US_LAYOUT);

    //! [THEN] `:` is registered as the keys that type it
    EXPECT_TRUE(isRegistered("Shift+;"));
    EXPECT_TRUE(isRegistered(";"));
    EXPECT_FALSE(isRegistered(":"));

    //! [THEN] Each one activates its own shortcut
    EXPECT_EQ(activate(";"), ";");
    EXPECT_EQ(activate("Shift+;"), ":");
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
    EXPECT_EQ(activate("Shift+;"), ":");
}

/**
 * @brief The other default shortcuts with a character that requires Shift on the US layout
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, US_ShiftedCharactersDoNotOverrideUnshifted)
{
    struct Case {
        std::string shifted;
        QString shiftedKeys;
        std::string unshifted;
    };

    //! [GIVEN] Pairs of shortcuts with a character and the character of the same key without Shift
    const std::vector<Case> cases = {
        { "_", "Shift+-", "-" },
        { "(", "Shift+9", "9" },
        // `+` is also on the keypad, so it is kept as is
        { "+", "+", "=" },
        { "Alt++", "Alt++", "Alt+=" },
        { "{", "Shift+[", "[" },
        { "}", "Shift+]", "]" },
    };

    for (size_t i = 0; i < cases.size(); ++i) {
        addShortcut("test://shifted-" + std::to_string(i), { cases[i].shifted });
        addShortcut("test://unshifted-" + std::to_string(i), { cases[i].unshifted });
    }

    //! [WHEN] The shortcuts are loaded for the US layout
    loadShortcuts(US_LAYOUT);

    //! [THEN] Each sequence activates its own shortcut
    for (const Case& c : cases) {
        EXPECT_EQ(activate(QString::fromStdString(c.unshifted)), c.unshifted);
        EXPECT_EQ(activate(c.shiftedKeys), c.shifted);
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
    EXPECT_EQ(activate("Shift+."), ":");
    EXPECT_EQ(activate("Shift+,"), ";");
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
    EXPECT_TRUE(isRegistered("Ctrl+Ф"));
    EXPECT_TRUE(isRegistered("Ctrl+Я"));

    //! [THEN] Pressing the keys activates the original shortcuts
    EXPECT_EQ(activate("Ctrl+Ф"), "Ctrl+A");
    EXPECT_EQ(activate("Ctrl+Я"), "Ctrl+Z");
}

TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, Russian_ColonDoesNotOverrideSemicolon)
{
    //! [GIVEN] `;` is on the `Ж` key, while `:` is `Shift+5` on the Russian layout
    addShortcut("test://a", { ":" });
    addShortcut("test://b", { ";" });

    //! [WHEN] The shortcuts are loaded for the Russian layout
    loadShortcuts(RUSSIAN_LAYOUT);

    //! [THEN] Both are translated to the keys that type them
    EXPECT_EQ(activate("Ж"), ";");
    EXPECT_EQ(activate("Shift+5"), ":");
}

/**
 * @brief Option shortcuts don't become ambiguous with the shortcuts for the characters typed with Option
 * @details `Option+1` types `!` and `Option+2` types `@` on the Russian layout. Qt matches such a key press against
 * both `Alt+1` and `!`, and if there are shortcuts for both, it activates neither of them
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, Russian_OptionShortcutsAreNotAmbiguous)
{
    //! [GIVEN] Shortcuts with Option and the characters typed by the same keys
    addShortcut("test://a", { "!" });
    addShortcut("test://b", { "@" });
    addShortcut("test://c", { "Alt+1" });
    addShortcut("test://d", { "Alt+2" });

    //! [WHEN] The shortcuts are loaded for the Russian layout
    loadShortcuts(RUSSIAN_LAYOUT);

    //! [THEN] The characters are not registered as is
    EXPECT_FALSE(isRegistered("!"));
    EXPECT_FALSE(isRegistered("@"));

    //! [THEN] The Option shortcuts take precedence over the characters typed by the same keys
    EXPECT_EQ(activate("Alt+1"), "Alt+1");
    EXPECT_EQ(activate("Alt+2"), "Alt+2");

    //! [THEN] `!` is still available with Shift
    EXPECT_EQ(activate("Shift+1"), "!");
}

/**
 * @brief A character that is typed with Option activates its shortcut, if there is no Option shortcut for that key
 */
TEST_F(Shortcuts_MacOSShortcutsInstanceModelTests, Russian_CharacterTypedWithOption)
{
    //! [GIVEN] `@` is `Option+2` on the Russian layout
    addShortcut("test://a", { "@" });

    //! [WHEN] The shortcuts are loaded for the Russian layout
    loadShortcuts(RUSSIAN_LAYOUT);

    //! [THEN] Pressing `Option+2` activates the shortcut
    EXPECT_EQ(activate("Alt+2"), "@");
}
