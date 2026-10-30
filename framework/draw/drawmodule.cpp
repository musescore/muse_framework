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
#include "drawmodule.h"

#include "global/modularity/ioc.h"

#ifndef DRAW_NO_INTERNAL
#ifndef NO_QT_SUPPORT
#include "internal/qfontprovider.h"
#include "internal/qimageprovider.h"
#else
#include "internal/fontprovider.h"
#include "internal/fontsengine.h"
#endif

#include "internal/fontsdatabase.h"
#endif

#include "muse_framework_config.h"

using namespace muse::draw;
using namespace muse::modularity;

static const std::string mname("draw");

std::string DrawModule::moduleName() const
{
    return mname;
}

void DrawModule::registerExports()
{
#ifndef DRAW_NO_INTERNAL
#ifndef NO_QT_SUPPORT
    globalIoc()->registerExport<draw::IImageProvider>(mname, std::make_shared<QImageProvider>());
#endif
#endif // DRAW_NO_INTERNAL
}

IContextSetup* DrawModule::newContext(const ContextPtr& ctx) const
{
    return new DrawContext(ctx);
}

void DrawContext::registerExports()
{
#ifndef DRAW_NO_INTERNAL
    ioc()->registerExport<draw::IFontsDatabase>(mname, std::make_shared<FontsDatabase>());

#ifndef NO_QT_SUPPORT
    ioc()->registerExport<draw::IFontProvider>(mname, std::make_shared<QFontProvider>(iocContext()));
#else
    m_fontsEngine = std::make_shared<FontsEngine>(iocContext());
    ioc()->registerExport<draw::IFontsEngine>(mname, m_fontsEngine);
    ioc()->registerExport<draw::IFontProvider>(mname, std::make_shared<FontProvider>(iocContext()));
#endif
#endif // DRAW_NO_INTERNAL
}

void DrawContext::onInit(const IApplication::RunMode&)
{
#if !defined(DRAW_NO_INTERNAL) && defined(NO_QT_SUPPORT)
    m_fontsEngine->init();
#endif // DRAW_NO_INTERNAL
}
