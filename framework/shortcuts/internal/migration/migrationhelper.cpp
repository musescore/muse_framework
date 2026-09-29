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

#include "migrationhelper.h"

#include "io/buffer.h"
#include "io/iodevice.h"
#include "io/path.h"
#include "rcommand/commandtypes.h"
#include "serialization/textstream.h"
#include "shortcutstypes.h"
#include "thirdparty/kors_logger/src/log_base.h"
#include "types/bytearray.h"
#include "io/file.h"

#include "log.h"

using namespace muse::shortcuts;

void MigrationHelper::init()
{
    commandDispatcher()->onRequest(this, rcommand::Command("command://shortcuts/migrate"), [this]() {
        actionsToCommands();
        return muse::make_ok();
    });
}

void MigrationHelper::actionsToCommands()
{
    commandDispatcher()->preDispatch().onReceive(this, [this](const rcommand::Command& command, bool* allowDispatch) {
        if (allowDispatch) {
            *allowDispatch = false;
        }

        onCommand(command);
        dispatchNext();
    });

    actionsDispatcher()->notRegistered().onReceive(this, [this](const actions::ActionCode&) {
        dispatchNext();
    });

    m_jsonArray = JsonArray();
    m_shortcuts = shortcutsRegister()->shortcuts();
    IF_ASSERT_FAILED(m_shortcuts.size() > 0) {
        return;
    }
    m_currentIt = m_shortcuts.begin();

    actionsDispatcher()->dispatch(m_currentIt->action);
}

void MigrationHelper::dispatchNext()
{
    m_currentIt++;
    if (m_currentIt == m_shortcuts.end()) {
        onFinished();
        return;
    }
    actionsDispatcher()->dispatch(m_currentIt->action);
}

void MigrationHelper::onCommand(const rcommand::Command& command)
{
    const Shortcut& shortcut = *m_currentIt;

    auto it = std::find_if(m_resultInfos.begin(), m_resultInfos.end(), [command](const ScInfo& scInfo) {
        return scInfo.command == command;
    });

    if (it != m_resultInfos.end()) {
        LOGW() << "Command already processed: " << command.toString() << " adding sequences to existing command";
        it->sequences.insert(it->sequences.end(), shortcut.sequences.begin(), shortcut.sequences.end());
        return;
    }

    ScInfo scInfo;
    scInfo.command = command;
    scInfo.action = m_currentIt->action;
    scInfo.sequences = shortcut.sequences;

    m_resultInfos.push_back(std::move(scInfo));
}

void MigrationHelper::onFinished()
{
    //! NOTE Json doesn't format very nicely and adds unnecessary escape characters, so we serialize manually.

    io::Buffer buf;
    buf.open(io::IODevice::ReadWrite);
    TextStream s(&buf);

    s << "[\n";
    for (const auto& scInfo : m_resultInfos) {
        s << "  {\n";
        s << "    \"command\": \"" << scInfo.command.toString() << "\",\n";
        s << "    \"sequences\": [";
        for (size_t i = 0; i < scInfo.sequences.size(); ++i) {
            s << "\"" << scInfo.sequences.at(i) << "\"";
            if (i < scInfo.sequences.size() - 1) {
                s << ",";
            }
        }
        s << "]\n";
        s << "  },\n";
    }
    s << "]\n";

    s.flush();

    io::path_t filePath = io::dirpath(configuration()->shortcutsUserAppDataPath()) + "/migration.json";
    Ret ret =io::File::writeFile(filePath, buf.data());

    if (ret) {
        LOGI() << "Migration data saved to: " << filePath << " commands count: " << m_resultInfos.size();
    } else {
        LOGE() << "Failed to save migration data to: " << filePath;
    }
}
