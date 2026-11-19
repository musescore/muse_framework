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

#include "uicomponents/qml/Muse/UiComponents/filepickermodel.h"

#include "interactive/tests/mocks/interactivemock.h"

using namespace ::testing;
using namespace muse;
using namespace muse::uicomponents;

namespace {
const QString TITLE = "Choose a file";
const QString DIR = "folder";
const QStringList FILTER = { "Text (*.txt)", "All (*)" };
const std::vector<std::string> EXPECTED_FILTER = { "Text (*.txt)", "All (*)" };

//! NOTE: keep the body queued, don't run it eagerly
//! otherwise resolve() can fire before onResolve is attached
async::Promise<io::path_t> chosenFile(const io::path_t& path)
{
    return async::make_promise<io::path_t>([path](auto resolve, auto) {
        return resolve(path);
    });
}

async::Promise<io::path_t> cancelled()
{
    return async::make_promise<io::path_t>([](auto, auto reject) {
        return reject(int(Ret::Code::Cancel), "");
    });
}
}

namespace muse::uicomponents {
class UiComponents_FilePickerModelTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_interactive = std::make_shared<NiceMock<InteractiveMock> >();
        m_model.interactive.set(m_interactive);

        m_model.setTitle(TITLE);
        m_model.setDir(DIR);
        m_model.setFilter(FILTER);

        QObject::connect(&m_model, &FilePickerModel::fileSelected, [this](const QString& path) {
            m_selectedFiles << path;
        });
    }

    FilePickerModel m_model;
    std::shared_ptr<InteractiveMock> m_interactive;
    QStringList m_selectedFiles;
};
}

TEST_F(UiComponents_FilePickerModelTests, SelectFile_UserChoosesAFile_SelectsItAndRemembersItsDir)
{
    //! [GIVEN] The user chooses a file in another folder
    EXPECT_CALL(*m_interactive, selectOpeningFile(TITLE.toStdString(), io::path_t(DIR), EXPECTED_FILTER))
    .WillOnce(Return(chosenFile("other/file.txt")));

    //! [WHEN] Selecting a file
    m_model.selectFile();
    async::processMessages();

    //! [THEN] The file is selected, and the next selection starts from its folder
    EXPECT_EQ(m_selectedFiles, QStringList { "other/file.txt" });
    EXPECT_EQ(m_model.dir(), QString("other"));
}

TEST_F(UiComponents_FilePickerModelTests, SelectFile_UserCancels_SelectsNothing)
{
    //! [GIVEN] The user cancels
    EXPECT_CALL(*m_interactive, selectOpeningFile(_, _, _)).WillOnce(Return(cancelled()));

    //! [WHEN] Selecting a file
    m_model.selectFile();
    async::processMessages();

    //! [THEN] Nothing is selected, and the folder stays the same
    EXPECT_TRUE(m_selectedFiles.isEmpty());
    EXPECT_EQ(m_model.dir(), DIR);
}

TEST_F(UiComponents_FilePickerModelTests, SelectAny_UserChoosesAFile_SelectsItAndRemembersItsDir)
{
    //! [GIVEN] The user chooses a file to save in another folder
    EXPECT_CALL(*m_interactive, selectSavingFile(TITLE.toStdString(), io::path_t(DIR), EXPECTED_FILTER, _))
    .WillOnce(Return(chosenFile("other/file.txt")));

    //! [WHEN] Selecting any file
    m_model.selectAny();
    async::processMessages();

    //! [THEN] The file is selected, and the next selection starts from its folder
    EXPECT_EQ(m_selectedFiles, QStringList { "other/file.txt" });
    EXPECT_EQ(m_model.dir(), QString("other"));
}

TEST_F(UiComponents_FilePickerModelTests, SelectAny_UserCancels_SelectsNothing)
{
    //! [GIVEN] The user cancels
    EXPECT_CALL(*m_interactive, selectSavingFile(_, _, _, _)).WillOnce(Return(cancelled()));

    //! [WHEN] Selecting any file
    m_model.selectAny();
    async::processMessages();

    //! [THEN] Nothing is selected, and the folder stays the same
    EXPECT_TRUE(m_selectedFiles.isEmpty());
    EXPECT_EQ(m_model.dir(), DIR);
}
