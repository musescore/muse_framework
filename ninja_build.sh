#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore Limited and others
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

trap 'echo Build failed; exit 1' ERR

if [ $(which nproc) ]; then
    JOBS=$(nproc --all)
else
    JOBS=4
fi

TARGET=release

CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES:-""}

MUSE_INSTALL_DIR=${MUSE_INSTALL_DIR:-"build/install"}
MUSE_BUILD_UNIT_TESTS=${MUSE_BUILD_UNIT_TESTS:-"OFF"}
MUSE_ENABLE_CODE_COVERAGE=${MUSE_ENABLE_CODE_COVERAGE:-"OFF"}
MUSE_CRASHREPORT_URL=${MUSE_CRASHREPORT_URL:-""}
MUSE_BUILD_CRASHPAD_CLIENT=${MUSE_BUILD_CRASHPAD_CLIENT:-"OFF"}
MUSE_NO_RPATH=${MUSE_NO_RPATH:-"OFF"}
MUSE_COMPILE_USE_UNITY=${MUSE_COMPILE_USE_UNITY:-"ON"}

MUSE_APP_INSTALL_SUFFIX=${MUSE_APP_INSTALL_SUFFIX:-""}
MUSE_APP_BUILD_MODE=${MUSE_APP_BUILD_MODE:-"dev"}
MUSE_APP_BUILD_NUMBER=${MUSE_APP_BUILD_NUMBER:-"12345678"}
MUSE_APP_REVISION=${MUSE_APP_REVISION:-"abc123456"}
MUSE_APP_DEBUGLEVEL_ENABLED="OFF"

SHOW_HELP=0
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -t|--target) TARGET="$2"; shift;;
        -j|--jobs) JOBS="$2"; shift;;
        -h|--help) SHOW_HELP=1;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

if [ $SHOW_HELP -eq 1 ]; then
    echo -e "Usage: ${0}\n" \
        "\t-t, --target <string> [default: ${TARGET}]\n" \
        "\t\tProvided targets: \n" \
        "\t\trelease, debug, relwithdebinfo, install, installrelwithdebinfo, \n" \
        "\t\tinstalldebug, clean, compile_commands, revision, appimage\n" \
        "\t-j, --jobs <number> [default: ${JOBS}]\n" \
        "\t\t Number of parallel compilations jobs\n" \
        "\t-h, --help\n" \
        "\t\t Show this help"
    exit 0
fi

cmake --version
echo "ninja version $(ninja --version)"

function do_build() {
    BUILD_TYPE=$1

    cmake ../.. -GNinja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_OSX_ARCHITECTURES="${CMAKE_OSX_ARCHITECTURES}" \
        -DCMAKE_INSTALL_PREFIX="${MUSE_INSTALL_DIR}" \
        -DCMAKE_TOOLCHAIN_FILE="${QT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake" \
        -DCMAKE_PREFIX_PATH="${QT_DIR}/lib/cmake" \
        -DMUSE_APP_INSTALL_SUFFIX="${MUSE_APP_INSTALL_SUFFIX}" \
        -DMUSE_APP_BUILD_MODE="${MUSE_APP_BUILD_MODE}" \
        -DCMAKE_BUILD_NUMBER="${MUSE_APP_BUILD_NUMBER}" \
        -DMUSE_APP_REVISION="${MUSE_APP_REVISION}" \
        -DMUSE_ENABLE_UNIT_TESTS="${MUSE_BUILD_UNIT_TESTS}" \
        -DMUSE_ENABLE_UNIT_TESTS_CODE_COVERAGE="${MUSE_UNIT_TESTS_ENABLE_CODE_COVERAGE}" \
        -DMUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT="${MUSE_BUILD_CRASHPAD_CLIENT}" \
        -DMUSE_MODULE_DIAGNOSTICS_CRASHREPORT_URL="${MUSE_CRASHREPORT_URL}" \
        -DMUSE_MODULE_GLOBAL_LOGGER_DEBUGLEVEL="${MUSE_APP_DEBUGLEVEL_ENABLED}" \
        -DCMAKE_SKIP_RPATH="${MUSE_NO_RPATH}" \
        -DMUSE_COMPILE_USE_UNITY="${MUSE_COMPILE_USE_UNITY}"

    ninja -j $JOBS
}

case $TARGET in
    release)
        mkdir -p build/release
        cd build/release
        do_build Release
        ;;

    debug)
        mkdir -p build/debug
        cd build/debug
        do_build Debug
        ;;

    relwithdebinfo)
        mkdir -p build/release
        cd build/release
        do_build RelWithDebInfo
        ;;

    install)
        mkdir -p build/release
        cd build/release
        do_build Release
        ninja install
        ;;

    installrelwithdebinfo)
        mkdir -p build/release
        cd build/release
        do_build RelWithDebInfo
        ninja install
        ;;

    installdebug)
        mkdir -p build/debug
        cd build/debug
        do_build Debug
        ninja install
        ;;

    clean)
        rm -rf build/
        ;;

    revision)
        git rev-parse --short=7 HEAD | tr -d '\n' >local_build_revision.env
        ;;

    *)
        echo "Unknown target: $TARGET"
        exit 1
        ;;
esac
