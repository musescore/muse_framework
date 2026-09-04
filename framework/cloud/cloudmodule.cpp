/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include "cloudmodule.h"

#include "modularity/ioc.h"
#include "interactive/iinteractiveuriregister.h"

#ifdef MUSE_MODULE_CLOUD_MUSESCORECOM
#include "musescorecom/musescorecomservice.h"
#endif
#include "audiocom/audiocomservice.h"
#include "internal/cloudconfiguration.h"

using namespace muse;
using namespace muse::cloud;
using namespace muse::modularity;

static const std::string mname("cloud");

std::string CloudModule::moduleName() const
{
    return "cloud";
}

void CloudModule::registerExports()
{
    m_cloudConfiguration = std::make_shared<CloudConfiguration>(globalCtx());
    globalIoc()->registerExport<ICloudConfiguration>(moduleName(), m_cloudConfiguration);
}

void CloudModule::resolveImports()
{
    auto ir = globalIoc()->resolve<interactive::IInteractiveUriRegister>(moduleName());
    if (ir) {
        ir->registerQmlUri(Uri("muse://cloud/requireauthorization"), "Muse.Cloud", "RequireAuthorizationDialog");
    }
}

void CloudModule::onInit(const IApplication::RunMode&)
{
    m_cloudConfiguration->init();
}

modularity::IContextSetup* CloudModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new CloudModuleContext(ctx);
}

void CloudModuleContext::registerExports()
{
#ifdef MUSE_MODULE_CLOUD_MUSESCORECOM
    m_museScoreComService = std::make_shared<MuseScoreComService>(iocContext());
    ioc()->registerExport<IMuseScoreComService>(mname, m_museScoreComService);
#endif
    m_audioComService = std::make_shared<AudioComService>(iocContext());
    ioc()->registerExport<IAudioComService>(mname, m_audioComService);
}

void CloudModuleContext::onInit(const IApplication::RunMode&)
{
#ifdef MUSE_MODULE_CLOUD_MUSESCORECOM
    m_museScoreComService->init();
#endif
    m_audioComService->init();
}
