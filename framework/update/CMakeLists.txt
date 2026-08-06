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

muse_create_module(muse_update ALIAS muse::update)

target_sources(muse_update PRIVATE
    updatemodule.cpp
    updatemodule.h
    updatetypes.h
    updateerrors.h
    iupdateconfiguration.h
    iupdaterequestparamsprovider.h
    iappupdatescenario.h
    iappupdateservice.h
    iupdateinstaller.h
    updatecommands.h

    internal/updateconfiguration.cpp
    internal/updateconfiguration.h
    internal/updateuiactions.cpp
    internal/updateuiactions.h
    internal/updateactioncontroller.cpp
    internal/updateactioncontroller.h
    internal/updatecommandsregister.cpp
    internal/updatecommandsregister.h
    internal/updatecommandsstate.cpp
    internal/updatecommandsstate.h
    internal/appupdatescenario.cpp
    internal/appupdatescenario.h
    internal/appupdateservice.cpp
    internal/appupdateservice.h
    internal/downloadfiledevice.cpp
    internal/downloadfiledevice.h
)

if (OS_IS_MAC)
    target_sources(muse_update PRIVATE
        internal/platform/mac/macupdateinstaller.cpp
        internal/platform/mac/macupdateinstaller.h
    )
elseif (OS_IS_WIN)
    target_sources(muse_update PRIVATE
        internal/platform/win/winupdateinstaller.cpp
        internal/platform/win/winupdateinstaller.h
        internal/platform/win/winupdateshared.h
    )

    target_link_libraries(muse_update PRIVATE taskschd ole32 oleaut32)
else()
    target_sources(muse_update PRIVATE
        internal/platform/stub/updateinstallerstub.cpp
        internal/platform/stub/updateinstallerstub.h
    )
endif()

if (MUSE_QT_SUPPORT)
    target_link_libraries(muse_update PRIVATE Qt::Concurrent)
endif()

if (NOT CC_IS_EMCC)
    add_subdirectory(helper)
endif()

if (MUSE_MODULE_UPDATE_TESTS)
    add_subdirectory(tests)
endif()

if (MUSE_MODULE_UPDATE_QML)
    add_subdirectory(qml/Muse/Update)
endif()