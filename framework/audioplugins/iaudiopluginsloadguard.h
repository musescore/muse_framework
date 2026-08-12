/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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
#pragma once

#include "modularity/imoduleinterface.h"

#include "global/types/ret.h"

#include "audiopluginstypes.h"

namespace muse::audioplugins {
/**
 * @brief Crash guard for in-process plugin loading.
 *
 * @details A plugin that passed validation may still bring the whole application down
 * when loaded later (e.g. after a licensing state change). Callers bracket
 * each in-process load with beginLoad()/endLoad(): beginLoad() persists the
 * plugin id to disk and endLoad() removes it once the load call has returned.
 * If the application dies in between, the id is still on disk at the next
 * launch and is reported by danglingLoads(), so the plugin can be marked as
 * broken instead of crashing the application again.
 */
class IAudioPluginsLoadGuard : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IAudioPluginsLoadGuard)

public:
    virtual ~IAudioPluginsLoadGuard() = default;

    virtual Ret beginLoad(const PluginResourceId& resourceId) = 0;
    virtual void endLoad(const PluginResourceId& resourceId) = 0;

    //! Ids whose load never completed in a previous run
    virtual PluginResourceIdList danglingLoads() const = 0;
    virtual Ret clearDanglingLoads() = 0;
};
}
