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

muse_create_module(muse_audioplugins ALIAS muse::audioplugins)

target_sources(muse_audioplugins PRIVATE
    audiopluginsmodule.cpp
    audiopluginsmodule.h
    audiopluginstypes.h
    audiopluginserrors.h
    iaudiopluginsconfiguration.h
    iknownaudiopluginsregister.h
    iknownaudiopluginsmigrationregister.h
    iaudiopluginsscanner.h
    iaudiopluginsscannerregister.h
    iaudiopluginmetareader.h
    iaudiopluginmetareaderregister.h
    iaudiopluginsloadguard.h
    iregisteraudiopluginsscenario.h

    internal/audiopluginsconfiguration.cpp
    internal/audiopluginsconfiguration.h
    internal/audiopluginsloadguard.cpp
    internal/audiopluginsloadguard.h
    internal/knownaudiopluginsregister.cpp
    internal/knownaudiopluginsregister.h
    internal/knownaudiopluginsmigrationregister.cpp
    internal/knownaudiopluginsmigrationregister.h
    internal/audiopluginsscannerregister.cpp
    internal/audiopluginsscannerregister.h
    internal/audiopluginmetareaderregister.cpp
    internal/audiopluginmetareaderregister.h
    internal/registeraudiopluginsscenario.cpp
    internal/registeraudiopluginsscenario.h
)

if (MUSE_MODULE_AUDIOPLUGINS_SCAN_TRACE)
    target_compile_definitions(muse_audioplugins PRIVATE MUSE_MODULE_AUDIOPLUGINS_SCAN_TRACE)
endif()

if (MUSE_MODULE_AUDIOPLUGINS_TESTS)
    add_subdirectory(tests)
endif()
