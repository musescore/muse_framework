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

set(UPDATE_HELPER_TARGET museupdater)

set(UPDATE_HELPER_SRC
    main.cpp
    platform.h
)

if (OS_IS_WIN)
    list(APPEND UPDATE_HELPER_SRC
        platform_win.cpp
        updatetask_win.cpp
        updatetask_win.h
        ../internal/platform/win/winupdateshared.h
    )
else()
    list(APPEND UPDATE_HELPER_SRC
        swap.cpp
        swap.h
    )

    if (OS_IS_MAC)
        list(APPEND UPDATE_HELPER_SRC platform_mac.cpp)
    else()
        list(APPEND UPDATE_HELPER_SRC platform_unix.cpp)
    endif()
endif()

add_executable(${UPDATE_HELPER_TARGET} ${UPDATE_HELPER_SRC})

set_target_properties(${UPDATE_HELPER_TARGET} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (OS_IS_LIN)
    # The helper is copied out of the AppImage and runs after it has been
    # unmounted, so it cannot rely on the libraries bundled inside it. Those are
    # generally newer than what the host provides, and linking against them
    # dynamically would make the helper fail to start on exactly the systems
    # where it is needed.
    if (CC_IS_GCC OR CC_IS_CLANG)
        target_link_options(${UPDATE_HELPER_TARGET} PRIVATE -static-libstdc++ -static-libgcc)
    endif()
endif()

if (OS_IS_WIN)
    target_link_libraries(${UPDATE_HELPER_TARGET} PRIVATE
        taskschd  # CLSID_TaskScheduler, IID_ITaskService, IID_IExecAction
        ole32     # CoInitializeEx, CoCreateInstance
        oleaut32  # SysAllocString, VariantInit
        advapi32  # registry, security descriptors, DuplicateTokenEx, CreateProcessAsUserW
        shell32   # CommandLineToArgvW
        wintrust  # WinVerifyTrust and the WTHelper* signer accessors
        crypt32   # CertGetNameStringW
        userenv   # CreateEnvironmentBlock
        wtsapi32  # WTSQueryUserToken
    )
endif()

# On macOS the helper is embedded into the app bundle by the app target.
if (NOT OS_IS_MAC)
    set(UPDATE_HELPER_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
    if (NOT UPDATE_HELPER_INSTALL_DIR)
        # GNUInstallDirs is not necessarily in use; the application itself is
        # installed into "bin" and the helper has to sit next to it.
        set(UPDATE_HELPER_INSTALL_DIR "bin")
    endif()

    install(TARGETS ${UPDATE_HELPER_TARGET}
        RUNTIME DESTINATION ${UPDATE_HELPER_INSTALL_DIR}
    )
endif()
