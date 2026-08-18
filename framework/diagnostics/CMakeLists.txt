# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2026 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

muse_create_module(muse_diagnostics ALIAS muse::diagnostics NO_COVERAGE)

target_sources(muse_diagnostics PRIVATE
    diagnosticsmodule.cpp
    diagnosticsmodule.h
    diagnosticutils.h
    idiagnosticspathsregister.h
    idiagnosticsconfiguration.h
    diagnosticscommands.h

    internal/diagnosticsconfiguration.cpp
    internal/diagnosticsconfiguration.h
    internal/diagnosticsactions.cpp
    internal/diagnosticsactions.h
    internal/diagnosticsactionscontroller.cpp
    internal/diagnosticsactionscontroller.h
    internal/diagnosticscommandsregister.cpp
    internal/diagnosticscommandsregister.h
    internal/diagnosticscommandsstate.cpp
    internal/diagnosticscommandsstate.h
    internal/diagnosticspathsregister.cpp
    internal/diagnosticspathsregister.h

    internal/isavediagnosticfilesscenario.h
    internal/savediagnosticfilesscenario.cpp
    internal/savediagnosticfilesscenario.h
    internal/diagnosticfileswriter.cpp
    internal/diagnosticfileswriter.h
)

# --- Crashpad ---
# Not building with MinGW, so turned off for MinGW
include(GetCompilerInfo)
include(GetPaths)
if (CC_IS_MINGW)
    set(BUILD_CRASHPAD_CLIENT OFF)
endif(CC_IS_MINGW)

if (MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT)
    target_sources(muse_diagnostics PRIVATE
        internal/crashhandler/crashhandler.cpp
        internal/crashhandler/crashhandler.h
    )

    set_source_files_properties(
        internal/crashhandler/crashhandler.cpp
        PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
    )

    target_compile_definitions(muse_diagnostics PRIVATE
        MUSE_MODULE_DIAGNOSTICS_CRASHREPORT_URL="${MUSE_MODULE_DIAGNOSTICS_CRASHREPORT_URL}"
    )

    # gcrashpad is built from the muse_deps crashpad_client source dep;
    # its PUBLIC includes carry the crashpad and mini_chromium headers.
    crashpad_client_add_to_build()
    target_link_libraries(muse_diagnostics PRIVATE gcrashpad)

    # Install crashpad handler
    if (NOT MUSE_MODULE_DIAGNOSTICS_CRASHPAD_HANDLER_PATH)
        message(FATAL_ERROR "Crashpad client is enabled, but MUSE_MODULE_DIAGNOSTICS_CRASHPAD_HANDLER_PATH is not set")
    endif()
    if (NOT EXISTS "${MUSE_MODULE_DIAGNOSTICS_CRASHPAD_HANDLER_PATH}")
        message(FATAL_ERROR "Crashpad handler not found: ${MUSE_MODULE_DIAGNOSTICS_CRASHPAD_HANDLER_PATH}")
    endif()
    if (OS_IS_LIN OR OS_IS_WIN OR OS_IS_MAC)
        install(PROGRAMS ${MUSE_MODULE_DIAGNOSTICS_CRASHPAD_HANDLER_PATH} DESTINATION ${INSTALL_SUBDIR})
    endif()
endif() # MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT
# ----------------

if (MUSE_QT_SUPPORT)
    target_link_libraries(muse_diagnostics PRIVATE Qt::Gui Qt::Quick)
endif()

if (MUSE_MODULE_DIAGNOSTICS_TESTS)
    add_subdirectory(tests)
endif()

if (MUSE_MODULE_DIAGNOSTICS_QML)
    add_subdirectory(qml/Muse/Diagnostics)
endif()
