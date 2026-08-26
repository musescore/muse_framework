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

#include "vstactionscontroller.h"

#include "rcommand/actiontocommand.h"

#include "../vstcommands.h"
#include "../widgets/vstviewdialog_qwidget.h"

#include "ivstplugininstance.h"

#include "log.h"

using namespace muse;
using namespace muse::vst;

static const char16_t* VST_EDITOR_URI = u"muse://vst/editor?instanceId=%1&modal=false&floating=true";

void VstActionsController::init()
{
    auto cd = commandDispatcher();
    cd->onRequest(this, VST_USE_OLDVIEW_COMMAND, [this]() { useView(false); return muse::make_ok(); });
    cd->onRequest(this, VST_USE_NEWVIEW_COMMAND, [this]() { useView(true); return muse::make_ok(); });
    cd->onRequest(this, VST_OPEN_FX_EDITOR_COMMAND, [this](const rcommand::Params& params) { return fxEditor(params); });
    cd->onRequest(this, VST_OPEN_INSTRUMENT_EDITOR_COMMAND, [this](const rcommand::Params& params) { return instEditor(params); });

    // compat
    {
        using namespace muse::rcommand;
        static const std::vector<ActionToCommand> actionToCommands = {
            { "vst-use-oldview", VST_USE_OLDVIEW_COMMAND, {} },
            { "vst-use-newview", VST_USE_NEWVIEW_COMMAND, {} }
        };

        rcommand::registerActionToCommand(this, actionToCommands, commandDispatcher(), dispatcher());

        static const std::vector<std::pair<actions::ActionQuery, Command> > actionQueryToCommands = {
            { actions::ActionQuery("action://vst/fx_editor"), VST_OPEN_FX_EDITOR_COMMAND },
            { actions::ActionQuery("action://vst/instrument_editor"), VST_OPEN_INSTRUMENT_EDITOR_COMMAND }
        };

        for (const auto& [actionQuery, command] : actionQueryToCommands) {
            dispatcher()->reg(this, actionQuery, [this, command](const actions::ActionQuery& aquery) {
                CommandQuery query(command);
                query.setParams(aquery.params());
                commandDispatcher()->dispatch(query);
            });
        }
    }
}

muse::Ret VstActionsController::fxEditor(const rcommand::Params& params)
{
#ifdef Q_OS_LINUX
    if (!isUsedNewView()) {
        LOGW() << "Old (QWidget) VST View not support Linux";
        return make_ret(Ret::Code::NotSupported);
    }
#endif

    std::string resourceId = params.at("resourceId").toString();
    IF_ASSERT_FAILED(!resourceId.empty()) {
        LOGE() << "not set resourceId";
        return make_ret(Ret::Code::BadArgs);
    }

    int trackId = params.at("trackId", Val(-1)).toInt();
    int chainOrder = params.at("chainOrder", Val(0)).toInt();
    std::string operation = params.at("operation", Val("open")).toString();
    bool sync = params.at("sync", Val(false)).toBool();

    auto instance = instancesRegister()->fxPlugin(resourceId, trackId, chainOrder);

    if (operation == "close" && !instance) {
        return make_ret(Ret::Code::BadArgs);
    }

    IF_ASSERT_FAILED(instance) {
        LOGE() << "not found instance, resourceId: " << resourceId
               << ", trackId: " << trackId << ", chainOrder: " << chainOrder;
        return make_ret(Ret::Code::BadArgs);
    }

    editorOperation(operation, instance->id(), sync);
    return make_ok();
}

muse::Ret VstActionsController::instEditor(const rcommand::Params& params)
{
#ifdef Q_OS_LINUX
    if (!isUsedNewView()) {
        LOGW() << "Old (QWidget) VST View not support Linux";
        return make_ret(Ret::Code::NotSupported);
    }
#endif

    std::string resourceId = params.at("resourceId").toString();
    IF_ASSERT_FAILED(!resourceId.empty()) {
        LOGE() << "not set resourceId";
        return make_ret(Ret::Code::BadArgs);
    }

    int trackId = params.at("trackId", Val(-1)).toInt();

    auto instance = instancesRegister()->instrumentPlugin(resourceId, trackId);

    std::string operation = params.at("operation", Val("open")).toString();
    bool sync = params.at("sync", Val(false)).toBool();

    if (operation == "close" && !instance) {
        return make_ret(Ret::Code::BadArgs);
    }

    IF_ASSERT_FAILED(instance) {
        LOGE() << "not found instance, resourceId: " << resourceId
               << ", trackId: " << trackId;
        return make_ret(Ret::Code::BadArgs);
    }

    editorOperation(operation, instance->id(), sync);
    return make_ok();
}

void VstActionsController::editorOperation(const std::string& operation, int instanceId, bool sync)
{
    UriQuery editorUri = UriQuery(String(VST_EDITOR_URI).arg(instanceId));

    if (operation == "close") {
        if (sync) {
            interactive()->closeSync(editorUri);
        } else {
            interactive()->close(editorUri);
        }
        return;
    }

    if (interactive()->isOpened(editorUri).val) {
        interactive()->raise(editorUri);
    } else if (sync) {
        interactive()->openSync(editorUri);
    } else {
        interactive()->open(editorUri);
    }
}

void VstActionsController::setupUsedView()
{
    useView(configuration()->usedVstView() == "newview");
}

void VstActionsController::useView(bool isNew)
{
    interactiveUriRegister()->unregisterUri(Uri("muse://vst/editor"));

    if (isNew) {
        configuration()->setUsedVstView("newview");
        interactiveUriRegister()->registerQmlUri(Uri("muse://vst/editor"), "Muse.Vst", "VstEditorDialog");
    } else {
        configuration()->setUsedVstView("oldview");
        interactiveUriRegister()->registerWidgetUri<VstViewDialog>(Uri("muse://vst/editor"));
    }

    m_actionCheckedChanged.send({ "vst-use-oldview", "vst-use-newview" });
}

bool VstActionsController::isUsedNewView() const
{
    return configuration()->usedVstView() == "newview";
}

bool VstActionsController::actionChecked(const actions::ActionCode& act) const
{
    if (act == "vst-use-oldview") {
        return !isUsedNewView();
    } else if (act == "vst-use-newview") {
        return isUsedNewView();
    }

    return false;
}

async::Channel<actions::ActionCodeList> VstActionsController::actionCheckedChanged() const
{
    return m_actionCheckedChanged;
}
