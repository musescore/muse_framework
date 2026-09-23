/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
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

 #include "singleprocessprovider.h"

 #include <QCoreApplication>

 #include "../../iprojectprovider.h"
 #include "ui/imainwindow.h"
 #include "actions/iactionsdispatcher.h"
 #include "rcommand/icommanddispatcher.h"

 #include "log.h"

using namespace muse;
using namespace muse::mi;

static const std::string pname = "mi";

//! NOTE The command is implemented by the application
static const rcommand::Command APP_QUIT_COMMAND("command://app/quit");

size_t SingleProcessProvider::windowCount() const
{
    return std::max(static_cast<size_t>(1), application()->contextCount());
}

bool SingleProcessProvider::isFirstWindow() const
{
    return windowCount() <= 1;
}

std::shared_ptr<IProjectProvider> SingleProcessProvider::projectProvider(const modularity::ContextPtr& ctx) const
{
    return modularity::ioc(ctx)->resolve<IProjectProvider>(pname);
}

std::shared_ptr<ui::IMainWindow> SingleProcessProvider::mainWindow(const modularity::ContextPtr& ctx) const
{
    return modularity::ioc(ctx)->resolve<ui::IMainWindow>(pname);
}

std::shared_ptr<actions::IActionsDispatcher> SingleProcessProvider::dispatcher(const modularity::ContextPtr& ctx) const
{
    return modularity::ioc(ctx)->resolve<actions::IActionsDispatcher>(pname);
}

std::shared_ptr<rcommand::ICommandDispatcher> SingleProcessProvider::commandDispatcher(const modularity::ContextPtr& ctx) const
{
    return modularity::ioc(ctx)->resolve<rcommand::ICommandDispatcher>(pname);
}

bool SingleProcessProvider::isProjectAlreadyOpened(const muse::io::path_t& projectPath) const
{
    for (const auto& ctx : application()->contexts()) {
        std::shared_ptr<IProjectProvider> pp = projectProvider(ctx);
        if (!pp) {
            LOGW() << "Not found implementation of IProjectProvider for context: " << ctx->id;
            continue;
        }

        if (pp->isProjectOpened(projectPath)) {
            return true;
        }
    }
    return false;
}

void SingleProcessProvider::activateWindowWithProject(const muse::io::path_t& projectPath)
{
    for (const auto& ctx : application()->contexts()) {
        std::shared_ptr<IProjectProvider> pp = projectProvider(ctx);
        if (!pp) {
            LOGW() << "Not found implementation of IProjectProvider for context: " << ctx->id;
            continue;
        }

        if (!pp->isProjectOpened(projectPath)) {
            continue;
        }

        std::shared_ptr<ui::IMainWindow> w = mainWindow(ctx);
        IF_ASSERT_FAILED(w) {
            continue;
        }

        w->requestShowOnFront();
        break;
    }
}

bool SingleProcessProvider::isHasWindowWithoutProject() const
{
    for (const auto& ctx : application()->contexts()) {
        std::shared_ptr<IProjectProvider> pp = projectProvider(ctx);
        if (!pp) {
            LOGW() << "Not found implementation of IProjectProvider for context: " << ctx->id;
            continue;
        }

        if (!pp->isAnyProjectOpened()) {
            return true;
        }
    }
    return false;
}

void SingleProcessProvider::activateWindowWithoutProject(const QStringList& args)
{
    for (const auto& ctx : application()->contexts()) {
        std::shared_ptr<IProjectProvider> pp = projectProvider(ctx);
        if (!pp) {
            LOGW() << "Not found implementation of IProjectProvider for context: " << ctx->id;
            continue;
        }

        if (pp->isAnyProjectOpened()) {
            continue;
        }

        std::shared_ptr<ui::IMainWindow> w = mainWindow(ctx);
        IF_ASSERT_FAILED(w) {
            continue;
        }

        w->requestShowOnFront();
        if (args.count() > 0 && !args.at(0).isEmpty()) {
            std::shared_ptr<actions::IActionsDispatcher> d = dispatcher(ctx);
            IF_ASSERT_FAILED(d) {
                break;
            }
            d->dispatch(args.at(0).toStdString(), actions::ActionData::make_arg1<bool>(false));
        }

        break;
    }
}

bool SingleProcessProvider::openNewWindow(const QStringList& args)
{
    LOGDA() << args;

    application()->setupNewContext(args);

    return true;
}

async::Promise<Ret> SingleProcessProvider::quitForAll(const modularity::ContextPtr& ctx)
{
    const std::vector<modularity::ContextPtr> all = application()->contexts();

    std::vector<modularity::ContextPtr> others;
    for (auto it = all.crbegin(); it != all.crend(); ++it) {
        const modularity::ContextPtr& c = *it;
        if (ctx && c->id == ctx->id) {
            continue;
        }

        others.push_back(c);
    }

    //! NOTE The current window quits first, then the others one by one
    quitWindow(ctx);

    return quitWindows(others);
}

async::Promise<Ret> SingleProcessProvider::quitWindows(const std::vector<modularity::ContextPtr>& ctxs)
{
    return async::make_promise<Ret>([this, ctxs](auto resolve) {
        if (ctxs.empty()) {
            return resolve(make_ok());
        }

        std::shared_ptr<rcommand::ICommandDispatcher> cd = commandDispatcher(ctxs.front());
        IF_ASSERT_FAILED(cd) {
            return resolve(make_ret(Ret::Code::InternalError));
        }

        std::shared_ptr<ui::IMainWindow> w = mainWindow(ctxs.front());
        IF_ASSERT_FAILED(w) {
            return resolve(make_ret(Ret::Code::InternalError));
        }

        //! NOTE Bring the window to the front, so that the user is asked about the window they see
        w->requestShowOnFront();

        std::vector<modularity::ContextPtr> rest(ctxs.cbegin() + 1, ctxs.cend());

        cd->dispatch(APP_QUIT_COMMAND, { { "all_instances", Val(false) } })
        .onResolve(this, [this, rest, resolve](const rcommand::Response& response) {
            if (!response.ret) {
                LOGD() << "quit for all canceled: " << response.ret.toString();
                (void)resolve(response.ret);
                return;
            }

            quitWindows(rest).onResolve(this, [resolve](const Ret& ret) {
                (void)resolve(ret);
            });
        });

        return async::Promise<Ret>::dummy_result();
    });
}

void SingleProcessProvider::quitWindow(const modularity::ContextPtr& ctx)
{
    if (application()->contextCount() <= 1) {
        QCoreApplication::exit();
    } else {
        application()->destroyContext(ctx);
    }
}
