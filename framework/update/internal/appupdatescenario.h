/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "update/iappupdatescenario.h"
#include "async/asyncable.h"
#include "modularity/ioc.h"

#include "interactive/iinteractive.h"
#include "actions/iactionsdispatcher.h"
#include "multiwindows/imultiwindowsprovider.h"
#include "update/iupdateconfiguration.h"
#include "update/iappupdateservice.h"
#include "global/iapplication.h"

namespace muse::update {
class AppUpdateScenario : public IAppUpdateScenario, public Contextable, public async::Asyncable
{
    GlobalInject<IApplication> application;
    GlobalInject<mi::IMultiWindowsProvider> multiwindowsProvider;
    GlobalInject<IUpdateConfiguration> configuration;
    ContextInject<IInteractive> interactive = { this };
    ContextInject<actions::IActionsDispatcher> dispatcher = { this };
    ContextInject<IAppUpdateService> service = { this };

public:
    AppUpdateScenario(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    bool needCheckForUpdate() const override;
    void checkForUpdate(bool manual) override;

    bool hasUpdate() const override;

    bool hasReadyUpdate() const override;
    async::Notification hasReadyUpdateChanged() const override;
    std::string readyUpdateVersion() const override;

    void installReadyUpdate() override;

private:
    muse::async::Promise<Ret> processUpdateError(int errorCode);

    async::Promise<IInteractive::Result> showNoUpdateMsg();
    muse::async::Promise<Ret> showReleaseInfo(const ReleaseInfo& info);
    async::Promise<IInteractive::Result> showServerErrorMsg();

    void downloadUpdateInBackground();

    muse::async::Promise<Ret> downloadRelease();
    muse::async::Promise<Ret> askToCloseAppAndCompleteInstall(const io::path_t& installerPath);
    muse::async::Promise<Ret> prepareAndInstall(const io::path_t& packagePath);
    muse::async::Promise<Ret> askToRestartAndInstall(const io::path_t& packagePath, const io::path_t& preparedPath);

    bool shouldIgnoreUpdate(const ReleaseInfo& info) const;

    bool m_checkInProgress = false;

    bool m_bgDownloadInProgress = false;
    io::path_t m_readyPackagePath;
    std::string m_readyUpdateVersion;
    async::Notification m_hasReadyUpdateChanged;
};
}
