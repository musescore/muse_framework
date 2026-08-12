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
#include "diagnosticsactionscontroller.h"

#include "rcommand/actiontocommand.h"

#include "../diagnosticscommands.h"

#include "types/uri.h"

#include "qml/Muse/Diagnostics/diagnosticaccessiblemodel.h"

#include "log.h"

using namespace muse::diagnostics;
using namespace muse;
using namespace muse::rcommand;

static const muse::UriQuery SYSTEM_PATHS_URI("muse://diagnostics/system/paths?modal=false&floating=true");
static const muse::UriQuery GRAPHICSINFO_URI("muse://diagnostics/system/graphicsinfo?modal=false&floating=true");
static const muse::UriQuery PROFILER_URI("muse://diagnostics/system/profiler?modal=false&floating=true");
static const muse::UriQuery NAVIGATION_TREE_URI("muse://diagnostics/navigation/tree?modal=false&floating=true");
static const muse::UriQuery ACCESSIBLE_TREE_URI("muse://diagnostics/accessible/tree?modal=false&floating=true");
static const muse::UriQuery ENGRAVING_ELEMENTS_URI("musescore://diagnostics/engraving/elements?modal=false&floating=true");
static const muse::UriQuery ENGRAVING_UNDOSTACK_URI("musescore://diagnostics/engraving/undostack?modal=false&floating=true");
static const muse::UriQuery ENGRAVING_STYLE_URI("musescore://diagnostics/engraving/style?modal=false&floating=true");
static const muse::UriQuery ACTIONS_LIST_URI("muse://diagnostics/actions/list?modal=false&floating=true");
static const muse::UriQuery RCOMMAND_LIST_URI("muse://diagnostics/rcommand/list?modal=false&floating=true");

void DiagnosticsActionsController::init()
{
    auto cd = commandDispatcher();
    cd->onRequest(this, DIAGNOSTICS_SAVE_FILES_COMMAND, [this]() { saveDiagnosticFiles(); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_PATHS_COMMAND, [this]() { openUri(SYSTEM_PATHS_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_GRAPHICSINFO_COMMAND, [this]() { openUri(GRAPHICSINFO_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_PROFILER_COMMAND, [this]() { openUri(PROFILER_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_NAVIGATION_TREE_COMMAND, [this]() { openUri(NAVIGATION_TREE_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_ACCESSIBLE_TREE_COMMAND, [this]() { openUri(ACCESSIBLE_TREE_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_DUMP_ACCESSIBLE_TREE_COMMAND, []() { DiagnosticAccessibleModel().dumpTree(); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_ENGRAVING_ELEMENTS_COMMAND, [this]() {
        openUri(ENGRAVING_ELEMENTS_URI, false);
        return muse::make_ok();
    });
    cd->onRequest(this, DIAGNOSTICS_SHOW_ENGRAVING_UNDOSTACK_COMMAND, [this]() {
        openUri(ENGRAVING_UNDOSTACK_URI, false);
        return muse::make_ok();
    });
    cd->onRequest(this, DIAGNOSTICS_SHOW_ENGRAVING_STYLE_COMMAND, [this]() {
        openUri(ENGRAVING_STYLE_URI, false);
        return muse::make_ok();
    });
    cd->onRequest(this, DIAGNOSTICS_SHOW_ACTIONS_COMMAND, [this]() { openUri(ACTIONS_LIST_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_SHOW_RCOMMANDS_COMMAND, [this]() { openUri(RCOMMAND_LIST_URI); return muse::make_ok(); });
    cd->onRequest(this, DIAGNOSTICS_ACTIONS_QUERY_COMMAND, [this](const CommandQuery& q) { return onCommandQuery(q); });
    cd->onRequest(this, DIAGNOSTICS_ACTIONS_QUERY_PARAMS1_COMMAND, [this](const CommandQuery& q) { return onCommandQuery(q); });
    cd->onRequest(this, DIAGNOSTICS_ACTIONS_QUERY_PARAMS2_COMMAND, [this](const CommandQuery& q) { return onCommandQuery(q); });

    // compat
    {
        static const std::vector<ActionToCommand> actionToCommands = {
            { "diagnostic-save-diagnostic-files", DIAGNOSTICS_SAVE_FILES_COMMAND, {} },
            { "diagnostic-show-paths", DIAGNOSTICS_SHOW_PATHS_COMMAND, {} },
            { "diagnostic-show-graphicsinfo", DIAGNOSTICS_SHOW_GRAPHICSINFO_COMMAND, {} },
            { "diagnostic-show-profiler", DIAGNOSTICS_SHOW_PROFILER_COMMAND, {} },
            { "diagnostic-show-navigation-tree", DIAGNOSTICS_SHOW_NAVIGATION_TREE_COMMAND, {} },
            { "diagnostic-show-accessible-tree", DIAGNOSTICS_SHOW_ACCESSIBLE_TREE_COMMAND, {} },
            { "diagnostic-accessible-tree-dump", DIAGNOSTICS_DUMP_ACCESSIBLE_TREE_COMMAND, {} },
            { "diagnostic-show-engraving-elements", DIAGNOSTICS_SHOW_ENGRAVING_ELEMENTS_COMMAND, {} },
            { "diagnostic-show-engraving-undostack", DIAGNOSTICS_SHOW_ENGRAVING_UNDOSTACK_COMMAND, {} },
            { "diagnostic-show-engraving-style", DIAGNOSTICS_SHOW_ENGRAVING_STYLE_COMMAND, {} },
            { "diagnostic-show-actions", DIAGNOSTICS_SHOW_ACTIONS_COMMAND, {} },
            { "diagnostic-show-rcommands", DIAGNOSTICS_SHOW_RCOMMANDS_COMMAND, {} },
        };

        rcommand::registerActionToCommand(this, actionToCommands, commandDispatcher(), dispatcher());

        static const std::vector<std::pair<actions::ActionQuery, Command> > actionQueryToCommands = {
            { actions::ActionQuery("action://diagnostic/actions/query"), DIAGNOSTICS_ACTIONS_QUERY_COMMAND },
            { actions::ActionQuery("action://diagnostic/actions/query_params1"), DIAGNOSTICS_ACTIONS_QUERY_PARAMS1_COMMAND },
            { actions::ActionQuery("action://diagnostic/actions/query_params2"), DIAGNOSTICS_ACTIONS_QUERY_PARAMS2_COMMAND },
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

void DiagnosticsActionsController::openUri(const UriQuery& uri, bool isSingle)
{
    if (isSingle && interactive()->isOpened(uri.uri()).val) {
        return;
    }

    interactive()->open(uri);
}

void DiagnosticsActionsController::saveDiagnosticFiles()
{
    Ret ret = saveDiagnosticsScenario()->saveDiagnosticFiles();
    if (!ret) {
        LOGE() << ret.toString();
    }
}

muse::Ret DiagnosticsActionsController::onCommandQuery(const muse::rcommand::CommandQuery& query)
{
    interactive()->info("Test query action", query.toString());
    return muse::make_ok();
}
