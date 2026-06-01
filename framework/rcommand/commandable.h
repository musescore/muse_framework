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

#pragma once

#include "icommanddispatcher.h"

namespace muse::rcommand {
class Commandable
{
public:
    virtual ~Commandable()
    {
        if (m_dispatcher) {
            m_dispatcher->unreg(this);
        }
    }

    inline void setDispatcher(ICommandDispatcher* dispatcher)
    {
        m_dispatcher = dispatcher;
    }

    inline bool isDispatcher(const ICommandDispatcher* dispatcher) const
    {
        return m_dispatcher == dispatcher;
    }

private:

    ICommandDispatcher* m_dispatcher = nullptr;
};
}
