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

#include "async/processevents.h"
#include "modularity/ioc.h"

#include "interactive/internal/interactive.h"

#include "mocks/interactiveuriregistermock.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using namespace muse;
using namespace muse::interactive;

namespace muse::interactive {
class Interactive_InteractiveTests : public ::testing::Test, public async::Asyncable
{
protected:
    void SetUp() override
    {
        m_uriRegister = std::make_shared<NiceMock<InteractiveUriRegisterMock> >();
        modularity::globalIoc()->unregister<IInteractiveUriRegister>("utests");
        modularity::globalIoc()->registerExport<IInteractiveUriRegister>("utests", m_uriRegister);

        m_interactive = std::make_shared<Interactive>(modularity::globalCtx());
    }

    void TearDown() override
    {
        m_interactive.reset();
        modularity::globalIoc()->unregister<IInteractiveUriRegister>("utests");
        m_uriRegister.reset();
    }

    //! Nothing is registered, so nothing can be opened
    void givenNothingIsRegistered()
    {
        ON_CALL(*m_uriRegister, meta(_)).WillByDefault(Return(ContainerMeta()));
    }

    static void drainDeferredCalls()
    {
        for (int i = 0; i < 10; ++i) {
            async::processMessages();
        }
    }

    std::shared_ptr<InteractiveUriRegisterMock> m_uriRegister;
    std::shared_ptr<IInteractive> m_interactive;
};

TEST_F(Interactive_InteractiveTests, Question_CannotBeOpened_IsAnsweredWithCancel)
{
    //! [GIVEN] The question cannot be opened (its dialog is not registered)
    givenNothingIsRegistered();

    //! [WHEN] Asking...
    bool resolved = false;
    bool rejected = false;
    IInteractive::Result result;

    m_interactive->question("Title", "Text", { IInteractive::Button::Yes, IInteractive::Button::No })
    .onResolve(this, [&resolved, &result](const IInteractive::Result& res) {
        resolved = true;
        result = res;
    })
    .onReject(this, [&rejected](int, const std::string&) {
        rejected = true;
    });

    drainDeferredCalls();

    //! [THEN] ...the answer is Cancel, and the failure is reported, the caller does not wait forever
    EXPECT_TRUE(resolved);
    EXPECT_EQ(result.standardButton(), IInteractive::Button::Cancel);
    EXPECT_TRUE(rejected);
}

TEST_F(Interactive_InteractiveTests, Open_CannotBeOpened_ReportsTheFailure)
{
    //! [GIVEN] The dialog cannot be opened (it is not registered)
    givenNothingIsRegistered();

    //! [WHEN] Opening it...
    bool resolved = false;
    bool rejected = false;

    m_interactive->open(UriQuery("test://a"))
    .onResolve(this, [&resolved](const Val&) {
        resolved = true;
    })
    .onReject(this, [&rejected](int, const std::string&) {
        rejected = true;
    });

    drainDeferredCalls();

    //! [THEN] ...the failure is reported, the caller does not wait forever
    EXPECT_FALSE(resolved);
    EXPECT_TRUE(rejected);
}
}
