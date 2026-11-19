/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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

#include "extensions/api/v1/filedialog.h"

#include "interactive/tests/mocks/interactivemock.h"

using namespace ::testing;
using namespace muse;
using namespace muse::extensions::apiv1;

namespace muse::extensions::apiv1 {
class Extensions_FileDialogTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_interactive = std::make_shared<NiceMock<InteractiveMock> >();
        m_dialog.interactive.set(m_interactive);
        countSignals();
    }

    //! NOTE The answer is given later, as by the user
    async::Promise<io::path_t> pendingAnswer()
    {
        return async::make_promise<io::path_t>([this](auto resolve, auto reject) {
            m_resolve = resolve;
            m_reject = reject;
            return async::Promise<io::path_t>::dummy_result();
        }, async::PromiseType::AsyncByBody);
    }

    void countSignals()
    {
        QObject::connect(&m_dialog, &FileDialog::accepted, [this]() { ++m_acceptedCount; });
        QObject::connect(&m_dialog, &FileDialog::rejected, [this]() { ++m_rejectedCount; });
    }

    FileDialog m_dialog;
    int m_acceptedCount = 0;
    int m_rejectedCount = 0;
    std::shared_ptr<InteractiveMock> m_interactive;
    async::Promise<io::path_t>::Resolve m_resolve;
    async::Promise<io::path_t>::Reject m_reject;
};
}

TEST_F(Extensions_FileDialogTests, UserChoosesAFile_Accepted)
{
    //! [GIVEN] The dialog is open, and the user has not answered yet
    EXPECT_CALL(*m_interactive, selectOpeningFile(_, _, _)).WillOnce(Return(pendingAnswer()));

    m_dialog.open();
    async::processMessages();

    //! [THEN] The dialog stays visible until the answer
    EXPECT_TRUE(m_dialog.visible());
    EXPECT_EQ(m_acceptedCount, 0);

    //! [WHEN] The user chooses a file
    (void)m_resolve(io::path_t("file.txt"));
    async::processMessages();

    //! [THEN] The file is accepted
    EXPECT_FALSE(m_dialog.visible());
    EXPECT_EQ(m_dialog.filePath(), QString("file.txt"));
    EXPECT_EQ(m_acceptedCount, 1);
    EXPECT_EQ(m_rejectedCount, 0);
}

TEST_F(Extensions_FileDialogTests, UserCancels_Rejected)
{
    //! [GIVEN] The saving dialog is open
    m_dialog.setType(FileDialog::Save);
    EXPECT_CALL(*m_interactive, selectSavingFile(_, _, _, _)).WillOnce(Return(pendingAnswer()));

    m_dialog.open();
    async::processMessages();

    //! [WHEN] The user cancels
    (void)m_reject(int(Ret::Code::Cancel), "");
    async::processMessages();

    //! [THEN] The dialog is rejected
    EXPECT_FALSE(m_dialog.visible());
    EXPECT_TRUE(m_dialog.filePath().isEmpty());
    EXPECT_EQ(m_acceptedCount, 0);
    EXPECT_EQ(m_rejectedCount, 1);
}

TEST_F(Extensions_FileDialogTests, OpenedAgainBeforeTheAnswer_OpensOneDialog)
{
    //! [GIVEN] The dialog is open, and the user has not answered yet
    EXPECT_CALL(*m_interactive, selectOpeningFile(_, _, _)).WillOnce(Return(pendingAnswer()));

    m_dialog.open();
    async::processMessages();

    //! [WHEN] The plugin opens it again
    m_dialog.open();
    async::processMessages();

    //! [THEN] No other dialog is opened (checked by WillOnce)
}

TEST_F(Extensions_FileDialogTests, ClosedAndOpenedAgainBeforeTheAnswer_OnlyTheLastAnswerCounts)
{
    //! [GIVEN] The dialog was opened, closed and opened again before the user answered the first one
    EXPECT_CALL(*m_interactive, selectOpeningFile(_, _, _))
    .Times(2)
    .WillRepeatedly([this](auto&&...) { return pendingAnswer(); });

    m_dialog.open();
    async::processMessages();
    async::Promise<io::path_t>::Resolve firstResolve = m_resolve;

    m_dialog.close();
    m_dialog.open();
    async::processMessages();

    //! [WHEN] The first dialog is answered
    (void)firstResolve(io::path_t("first.txt"));
    async::processMessages();

    //! [THEN] It is ignored
    EXPECT_TRUE(m_dialog.visible());
    EXPECT_TRUE(m_dialog.filePath().isEmpty());
    EXPECT_EQ(m_acceptedCount, 0);
    EXPECT_EQ(m_rejectedCount, 0);

    //! [WHEN] The last dialog is answered
    (void)m_resolve(io::path_t("last.txt"));
    async::processMessages();

    //! [THEN] Its file is accepted
    EXPECT_FALSE(m_dialog.visible());
    EXPECT_EQ(m_dialog.filePath(), QString("last.txt"));
    EXPECT_EQ(m_acceptedCount, 1);
    EXPECT_EQ(m_rejectedCount, 0);
}

TEST_F(Extensions_FileDialogTests, ClosedBeforeTheAnswer_TheAnswerIsIgnored)
{
    //! [GIVEN] The dialog was closed before the user answered
    EXPECT_CALL(*m_interactive, selectOpeningFile(_, _, _)).WillOnce(Return(pendingAnswer()));

    m_dialog.open();
    async::processMessages();
    m_dialog.close();

    //! [WHEN] The user chooses a file
    (void)m_resolve(io::path_t("file.txt"));
    async::processMessages();

    //! [THEN] Nothing changes
    EXPECT_FALSE(m_dialog.visible());
    EXPECT_TRUE(m_dialog.filePath().isEmpty());
    EXPECT_EQ(m_acceptedCount, 0);
    EXPECT_EQ(m_rejectedCount, 0);
}
