/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) 2026 MuseScore/Audacity and others
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
#pragma once

#include "../icommanddispatcher.h"

namespace muse::rcommand {
class CommandDispatcher : public ICommandDispatcher
{
public:
    CommandDispatcher() = default;
    ~CommandDispatcher() override;

    async::Promise<Response> dispatch(const Request& request) override;
    void onRequest(Commandable* client, const Command& command, const CallBack& callback) override;
    void onRequest(Commandable* client, const Command& command, const AsyncCallBack& callback) override;
    void unreg(Commandable* client) override;

    // for utests
    Response dispatch(const Command& command, const Params& params = {});
    Response dispatch(const CommandQuery& query);

private:

    struct Client
    {
        Commandable* client = nullptr;
        CallBack callback = nullptr;
        AsyncCallBack asyncCallback = nullptr;
    };

    void reg(Commandable* client, const Command& command, const Client& c);

    std::map<Command, Client> m_clients;
};
}
