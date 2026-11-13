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

#include "internal/shortcutsresolver.h"

using namespace muse;
using namespace muse::shortcuts;
using namespace muse::rcommand;

namespace muse::shortcuts {
class Shortcuts_ShortcutsResolverTests : public ::testing::Test
{
public:
    static Shortcut makeShortcut(const std::string& command, const std::string& scope)
    {
        Shortcut sc;
        sc.command = Command(command);
        sc.scope = scope;
        sc.sequences = { "Space" };
        return sc;
    }

    ShortcutsResolver m_resolver;
};

TEST_F(Shortcuts_ShortcutsResolverTests, SelectsTheScopeWithTheHighestPriority)
{
    //! [GIVEN] Two scopes with different priorities
    m_resolver.setScope({ "A", {}, 1 });
    m_resolver.setScope({ "B", {}, 2 });

    //! [WHEN] Both shortcuts are allowed
    Shortcut active = m_resolver.selectActive({ makeShortcut("test://a", "A"), makeShortcut("test://b", "B") });

    //! [THEN] The shortcut of the scope with the higher priority is selected
    EXPECT_EQ(active.command, Command("test://b"));
}

TEST_F(Shortcuts_ShortcutsResolverTests, ScopeThatIsNotSetHasZeroPriority)
{
    //! [GIVEN] Only one scope is set
    m_resolver.setScope({ "A", {}, 1 });

    //! [WHEN] The other scope is unknown to the resolver
    Shortcut active = m_resolver.selectActive({ makeShortcut("test://c", "C"), makeShortcut("test://a", "A") });

    //! [THEN] The set scope wins
    EXPECT_EQ(active.command, Command("test://a"));
}

TEST_F(Shortcuts_ShortcutsResolverTests, ActivityIsCheckedOnEachSelection)
{
    //! [GIVEN] The activity of a scope depends on a state
    bool bActive = false;
    m_resolver.setScope({ "A", {}, 1 });
    m_resolver.setScope({ "B", {}, 2, [&bActive]() { return bActive; } });

    const ShortcutList list = { makeShortcut("test://a", "A"), makeShortcut("test://b", "B") };

    //! [THEN] The state is taken into account when selecting
    EXPECT_EQ(m_resolver.selectActive(list).command, Command("test://a"));

    bActive = true;
    EXPECT_EQ(m_resolver.selectActive(list).command, Command("test://b"));
}

TEST_F(Shortcuts_ShortcutsResolverTests, InactiveScopeIsLowerThanUnknownScope)
{
    //! [GIVEN] A scope with a high priority that is not active
    m_resolver.setScope({ "A", {}, 10, []() { return false; } });

    //! [WHEN] Selecting among its shortcut and a shortcut of an unknown scope
    Shortcut active = m_resolver.selectActive({ makeShortcut("test://a", "A"), makeShortcut("test://c", "C") });

    //! [THEN] The shortcut of the unknown scope is selected
    EXPECT_EQ(active.command, Command("test://c"));
}

TEST_F(Shortcuts_ShortcutsResolverTests, ActiveScopeWithNegativePriority)
{
    //! [GIVEN] An active scope with a negative priority and an inactive scope
    m_resolver.setScope({ "A", {}, -5 });
    m_resolver.setScope({ "B", {}, 1, []() { return false; } });

    //! [THEN] The active scope is selected, it is not rejected because of its priority
    EXPECT_EQ(m_resolver.selectActive({ makeShortcut("test://a", "A") }).command, Command("test://a"));
    EXPECT_EQ(m_resolver.selectActive({ makeShortcut("test://b", "B"), makeShortcut("test://a", "A") }).command, Command("test://a"));

    //! [THEN] Among the active scopes the priority still decides
    EXPECT_EQ(m_resolver.selectActive({ makeShortcut("test://a", "A"), makeShortcut("test://c", "C") }).command, Command("test://c"));
}

TEST_F(Shortcuts_ShortcutsResolverTests, InactiveScopeGivesNoShortcut)
{
    //! [GIVEN] The only scope is not active
    m_resolver.setScope({ "A", {}, 1, []() { return false; } });

    //! [WHEN] Selecting among its shortcuts
    Shortcut active = m_resolver.selectActive({ makeShortcut("test://a", "A") });

    //! [THEN] There is no active shortcut
    EXPECT_FALSE(active.isValid());
}

TEST_F(Shortcuts_ShortcutsResolverTests, CommandWinsOverNoneInTheSameScope)
{
    //! [GIVEN] A scope that has a real command and the none command for the same sequence
    m_resolver.setScope({ "A", {}, 1 });

    //! [WHEN] Selecting
    Shortcut active = m_resolver.selectActive({ makeShortcut(NONE_COMMAND.toString(), "A"), makeShortcut("test://a", "A") });

    //! [THEN] The real command is selected
    EXPECT_EQ(active.command, Command("test://a"));
}

TEST_F(Shortcuts_ShortcutsResolverTests, ScopesWithEqualPriorityAndNoneCommand)
{
    //! [GIVEN] Two scopes with the same priority, one of them also has the none command
    m_resolver.setScope({ "A", {}, 1 });
    m_resolver.setScope({ "B", {}, 1 });

    const Shortcut aReal = makeShortcut("test://a", "A");
    const Shortcut bReal = makeShortcut("test://b", "B");
    const Shortcut aNone = makeShortcut(NONE_COMMAND.toString(), "A");

    //! [THEN] For any order the first real command of the list is selected
    EXPECT_EQ(m_resolver.selectActive({ aReal, bReal, aNone }).command, aReal.command);
    EXPECT_EQ(m_resolver.selectActive({ aReal, aNone, bReal }).command, aReal.command);
    EXPECT_EQ(m_resolver.selectActive({ bReal, aReal, aNone }).command, bReal.command);
    EXPECT_EQ(m_resolver.selectActive({ bReal, aNone, aReal }).command, bReal.command);
    EXPECT_EQ(m_resolver.selectActive({ aNone, aReal, bReal }).command, aReal.command);
    EXPECT_EQ(m_resolver.selectActive({ aNone, bReal, aReal }).command, bReal.command);
}

TEST_F(Shortcuts_ShortcutsResolverTests, ScopeTitle)
{
    //! [GIVEN] A scope with a title
    m_resolver.setScope({ "a", TranslatableString::untranslatable("Title A") });

    //! [THEN] The title is found regardless of the case
    EXPECT_EQ(m_resolver.scopeTitle("A").str, String(u"Title A"));
    EXPECT_EQ(m_resolver.scopeTitle("a").str, String(u"Title A"));

    //! [THEN] The code is used as the title of an unknown scope
    EXPECT_EQ(m_resolver.scopeTitle("b").str, String(u"B"));
}
}
