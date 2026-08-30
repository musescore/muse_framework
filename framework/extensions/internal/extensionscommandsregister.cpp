/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) MuseScore Limited and others
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

#include "extensionscommandsregister.h"

#include "../extensionscommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::extensions;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        OPEN_APIDUMP_COMMAND,
        TranslatableString("extensions", "Show API dump"),
        TranslatableString("extensions", "Show API dump"),
        InputSchema(),
        Decoration()
    },
};

std::string ExtensionsCommandsRegister::moduleName() const
{
    return "extensions";
}

void ExtensionsCommandsRegister::init()
{
    extensionsRegister()->manifestListChanged().onNotify(this, [this]() {
        reload();
    });

    reload();
}

void ExtensionsCommandsRegister::reload()
{
    m_commandInfos.clear();
    m_commands.clear();

    const auto manifests = extensionsRegister()->manifestList();

    m_commandInfos.reserve(manifests.size() + s_commandInfos.size());
    m_commandInfos.insert(m_commandInfos.end(), s_commandInfos.begin(), s_commandInfos.end());

    for (const auto& manifest : manifests) {
        for (const auto& action : manifest.actions) {
            CommandInfo info = {
                makeCommand(manifest.uri, action.code),
                TranslatableString::untranslatable(action.title.empty() ? manifest.title : action.title),
                TranslatableString::untranslatable(manifest.description),
                InputSchema(),
                Decoration(action.icon)
            };
            m_commandInfos.push_back(std::move(info));
        }
    }

    m_commandListChanged.notify();
}

const std::vector<muse::rcommand::Command>& ExtensionsCommandsRegister::commandList() const
{
    if (m_commands.empty()) {
        m_commands.reserve(m_commandInfos.size());
        for (const auto& info : m_commandInfos) {
            m_commands.push_back(info.command);
        }
    }
    return m_commands;
}

const std::vector<CommandInfo>& ExtensionsCommandsRegister::commandInfoList() const
{
    return m_commandInfos;
}

async::Notification ExtensionsCommandsRegister::commandListChanged() const
{
    return m_commandListChanged;
}
