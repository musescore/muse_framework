/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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
#include "extensionsmodule.h"

#include "modularity/ioc.h"

#include "interactive/iinteractiveuriregister.h"

#include "internal/extensionsprovider.h"
#include "internal/extensionsconfiguration.h"
#include "internal/extensionsactioncontroller.h"
#include "internal/extensioninstaller.h"
#include "internal/extensionsuiengine.h"
#include "internal/extensionscommandsregister.h"
#include "internal/extensionscommandsstate.h"
#include "internal/extensionsregister.h"

#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"

#include "api/v1/extapiv1.h"

#include "muse_framework_config.h"
#ifdef MUSE_MODULE_DIAGNOSTICS
#include "diagnostics/idiagnosticspathsregister.h"
#endif

using namespace muse::extensions;
using namespace muse::modularity;

static const std::string mname("extensions");

std::string ExtensionsModule::moduleName() const
{
    return mname;
}

void ExtensionsModule::registerExports()
{
    m_configuration = std::make_shared<ExtensionsConfiguration>(globalCtx());
    m_extensionsRegister = std::make_shared<ExtensionsRegister>();
    m_extensionsCommandsRegister = std::make_shared<ExtensionsCommandsRegister>();

    globalIoc()->registerExport<IExtensionsConfiguration>(mname, m_configuration);
    globalIoc()->registerExport<IExtensionsRegister>(mname, m_extensionsRegister);
}

void ExtensionsModule::resolveImports()
{
    auto ir = globalIoc()->resolve<interactive::IInteractiveUriRegister>(mname);
    if (ir) {
        ir->registerQmlUri(Uri("muse://extensions/viewer"), "Muse.Extensions", "ExtensionViewerDialog");
        ir->registerQmlUri(Uri("muse://extensions/apidump"), "Muse.Extensions", "ExtensionsApiDumpDialog");
    }

    auto cr = globalIoc()->resolve<muse::rcommand::ICommandsRegister>(mname);
    if (cr) {
        cr->reg(m_extensionsCommandsRegister);

        m_extensionsCommandsRegister->init();
        m_extensionsCommandsRegister->commandListChanged().onNotify(this, [this, cr]() {
            cr->unreg(m_extensionsCommandsRegister);
            cr->reg(m_extensionsCommandsRegister);
        });
    }
}

void ExtensionsModule::registerApi()
{
    apiv1::ExtApiV1::registerQmlTypes();
}

void ExtensionsModule::onInit(const IApplication::RunMode&)
{
    m_configuration->init();
    m_extensionsRegister->reload();

#ifdef MUSE_MODULE_DIAGNOSTICS
    auto pr = globalIoc()->resolve<muse::diagnostics::IDiagnosticsPathsRegister>(mname);
    if (pr) {
        pr->reg("extensions: defaultPath", m_configuration->defaultPath());
        pr->reg("extensions: userPath", m_configuration->userPath());
        pr->reg("plugins (legacy): defaultPath", m_configuration->pluginsDefaultPath());
        pr->reg("plugins (legacy): userPath", m_configuration->pluginsUserPath());
    }
#endif
}

// Context
IContextSetup* ExtensionsModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new ExtensionsContext(ctx);
}

void ExtensionsContext::registerExports()
{
    m_actionController = std::make_shared<ExtensionsActionController>(iocContext());
    m_provider = std::make_shared<ExtensionsProvider>(iocContext());

    ioc()->registerExport<IExtensionsUiEngine>(mname, new ExtensionsUiEngine(iocContext()));
    ioc()->registerExport<IExtensionsProvider>(mname, m_provider);
    ioc()->registerExport<IExtensionInstaller>(mname, new ExtensionInstaller(iocContext()));
}

void ExtensionsContext::resolveImports()
{
    auto cs = ioc()->resolve<muse::rcommand::ICommandsState>(mname);
    if (cs) {
        cs->reg(std::make_shared<ExtensionsCommandsState>(iocContext()));
    }
}

void ExtensionsContext::onInit(const IApplication::RunMode&)
{
    m_actionController->init();
    m_provider->reloadExtensions();
}
