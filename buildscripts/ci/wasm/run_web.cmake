# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Builds the web (wasm) version of an app based on the Muse framework,
# optionally its audio engine, and serves it locally.
#
#   cmake -DAPP=<name> [-D<option>=<value> ...] -P run_web.cmake
#
# Requirements: cmake, ninja, python3, Emscripten SDK, Qt for WebAssembly (single-threaded)

cmake_minimum_required(VERSION 3.24)

###########################################
# Arguments
###########################################
set(_HELP [=[
Usage: cmake -DAPP=<name> [-D<option>=<value> ...] -P run_web.cmake

Project:
  ROOT            project root [default: current directory]
  APP             app target name, <APP>.wasm is expected in the artifacts (required)
  CMAKE_ARGS      configure arguments of the app, a list: "-DA=1;-DB=2"
  AUDIO_ENGINE    CMake project of the audio engine, relative to the root [default: none]
  BUILD_DIR       build directory [default: <ROOT>/build.web],
                  the audio engine is built in <BUILD_DIR>/audioengine
  ARTIFACTS_DIR   directory with the build result [default: <ROOT>/build.artifacts]
  PAGE            app page in the artifacts [default: viewer.html]

Environment:
  QT_DIR          Qt for WebAssembly [default: $QT_ROOT_DIR, or <home>/Qt/<version>/wasm_singlethread]
  QT_VERSION      preferred Qt version, used to find Qt [default: the latest installed]
  EMSDK_DIR       Emscripten SDK [default: $EMSDK, or <home>/emsdk]
  EMSDK_VERSION   expected Emscripten version, a warning if it differs [default: no check]

Run:
  JOBS            parallel compilation jobs [default: 8]
  PORT            http server port [default: 8081]
  NO_BUILD=ON     only run the server
  NO_RUN=ON       only build
  NO_OPEN=ON      don't open the browser
  HELP=ON         show this help
]=])

if(HELP)
    message("${_HELP}")
    return()
endif()

if(NOT APP)
    message("${_HELP}")
    message(FATAL_ERROR "APP is required")
endif()

if(NOT ROOT)
    set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}") # the current directory in the script mode
endif()
file(REAL_PATH "${ROOT}" ROOT)

if(NOT BUILD_DIR)
    set(BUILD_DIR "${ROOT}/build.web")
endif()
file(REAL_PATH "${BUILD_DIR}" BUILD_DIR)
set(AUDIO_BUILD_DIR "${BUILD_DIR}/audioengine")

if(NOT ARTIFACTS_DIR)
    set(ARTIFACTS_DIR "${ROOT}/build.artifacts")
endif()
file(REAL_PATH "${ARTIFACTS_DIR}" ARTIFACTS_DIR)

if(NOT PAGE)
    set(PAGE "viewer.html")
endif()
if(NOT JOBS)
    set(JOBS 8)
endif()
if(NOT PORT)
    set(PORT 8081)
endif()

if(DEFINED ENV{HOME})
    set(_HOME "$ENV{HOME}")
else()
    set(_HOME "$ENV{USERPROFILE}")
endif()

###########################################
# Helpers
###########################################
function(require_program var)
    find_program(${var} NAMES ${ARGN})
    if(NOT ${var})
        message(FATAL_ERROR "'${ARGV1}' not found in PATH")
    endif()
endfunction()

# Runs a command, stops on failure
function(run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Command failed (${_result}): ${ARGN}")
    endif()
endfunction()

# The given Qt version if installed, else the latest one
function(find_qt out)
    foreach(_base "${_HOME}/Qt" "C:/Qt")
        if(QT_VERSION AND EXISTS "${_base}/${QT_VERSION}/wasm_singlethread")
            set(${out} "${_base}/${QT_VERSION}/wasm_singlethread" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    file(GLOB _found LIST_DIRECTORIES true "${_HOME}/Qt/*/wasm_singlethread" "C:/Qt/*/wasm_singlethread")
    if(_found)
        list(SORT _found COMPARE NATURAL)
        list(GET _found -1 _latest)
        set(${out} "${_latest}" PARENT_SCOPE)
    else()
        set(${out} "" PARENT_SCOPE)
    endif()
endfunction()

# Sets up the Emscripten environment, the same as emsdk_env does
function(setup_emsdk_env emsdk_dir)
    if(CMAKE_HOST_WIN32)
        # On Windows emsdk sets the environment in the calling cmd (via a temporary .bat),
        # so it's set up and printed in the same child shell. Lines: NAME=VALUE
        file(TO_NATIVE_PATH "${emsdk_dir}/emsdk_env.bat" _emsdk_env)
        set(_command cmd /c call "${_emsdk_env}" >nul && set)
        set(_line_regex "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$")
    else()
        # Lines: export NAME="VALUE";
        set(_command "${emsdk_dir}/emsdk" construct_env)
        set(_line_regex "^export ([A-Za-z_][A-Za-z0-9_]*)=\"(.*)\";?$")
    endif()

    execute_process(COMMAND ${_command} OUTPUT_VARIABLE _env ERROR_QUIET RESULT_VARIABLE _result)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Failed to get the Emscripten environment: ${_command}")
    endif()

    # NOTE Not a CMake list, the values may contain ';' (PATH on Windows)
    string(REPLACE "\r" "" _env "${_env}")
    while(NOT _env STREQUAL "")
        string(FIND "${_env}" "\n" _pos)
        if(_pos EQUAL -1)
            set(_line "${_env}")
            set(_env "")
        else()
            string(SUBSTRING "${_env}" 0 ${_pos} _line)
            math(EXPR _pos "${_pos} + 1")
            string(SUBSTRING "${_env}" ${_pos} -1 _env)
        endif()

        if(_line MATCHES "${_line_regex}")
            set(ENV{${CMAKE_MATCH_1}} "${CMAKE_MATCH_2}")
        endif()
    endwhile()
endfunction()

# Checks that an existing build directory is a wasm build with the same configure arguments,
# so that e.g. a desktop build in the same directory is not reused
# Sets <out_var> to TRUE if some of the -D arguments are not in the cache yet (needs a reconfigure)
function(check_build_dir dir out_var)
    file(STRINGS "${dir}/CMakeCache.txt" _cache)
    set(_reconfigure FALSE)

    if(NOT "EMSCRIPTEN:INTERNAL=1" IN_LIST _cache)
        message(FATAL_ERROR "${dir} is not a wasm build, remove it or set another BUILD_DIR")
    endif()

    # The toolchain (Qt) can't be changed in an existing build directory
    file(REAL_PATH "$ENV{CMAKE_TOOLCHAIN_FILE}" _toolchain)
    set(_cached_toolchain "")
    foreach(_line ${_cache})
        if(_line MATCHES "^CMAKE_TOOLCHAIN_FILE:[A-Z]+=(.*)$")
            file(REAL_PATH "${CMAKE_MATCH_1}" _cached_toolchain)
            break()
        endif()
    endforeach()
    if(NOT _cached_toolchain STREQUAL _toolchain)
        message(FATAL_ERROR "${dir} is configured with another toolchain (Qt): ${_cached_toolchain}\n"
                            "expected: ${_toolchain}, remove it or set another BUILD_DIR")
    endif()

    foreach(_arg ${ARGN})
        if(NOT _arg MATCHES "^-D([^=:]+)(:[A-Z]+)?=(.*)$")
            continue()
        endif()
        set(_key "${CMAKE_MATCH_1}")
        set(_value "${CMAKE_MATCH_3}")
        set(_cached FALSE)
        foreach(_line ${_cache})
            if(_line MATCHES "^${_key}:[A-Z]+=(.*)$")
                set(_cached TRUE)
                if(NOT CMAKE_MATCH_1 STREQUAL _value)
                    message(FATAL_ERROR "${dir} is configured with another ${_key} (expected: ${_value}),\n"
                                        "remove it or set another BUILD_DIR")
                endif()
                break()
            endif()
        endforeach()
        if(NOT _cached)
            set(_reconfigure TRUE)
        endif()
    endforeach()
    set(${out_var} ${_reconfigure} PARENT_SCOPE)
endfunction()

# Configures the build directory if needed and builds it, with a low priority if possible
function(build src dir)
    if(EXISTS "${dir}/build.ninja")
        check_build_dir("${dir}" _reconfigure ${ARGN})
        if(_reconfigure)
            run(${CMAKE_COMMAND} -S "${src}" -B "${dir}" ${ARGN})
        endif()
    else()
        run(${CMAKE_COMMAND} -S "${src}" -B "${dir}" -G Ninja
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_TOOLCHAIN_FILE=$ENV{CMAKE_TOOLCHAIN_FILE}
            ${ARGN})
    endif()

    find_program(NICE nice)
    if(NICE)
        run(${NICE} -n 15 ${NINJA} -C "${dir}" -j ${JOBS})
    else()
        run(${NINJA} -C "${dir}" -j ${JOBS})
    endif()
endfunction()

###########################################
# Build
###########################################
if(NOT NO_BUILD)
    require_program(NINJA ninja)

    # Emscripten
    if(NOT EMSDK_DIR)
        if(DEFINED ENV{EMSDK})
            set(EMSDK_DIR "$ENV{EMSDK}")
        else()
            set(EMSDK_DIR "${_HOME}/emsdk")
        endif()
    endif()
    if(NOT EXISTS "${EMSDK_DIR}/emsdk.py")
        message(FATAL_ERROR "Emscripten SDK not found: ${EMSDK_DIR} (set EMSDK_DIR or $EMSDK)")
    endif()
    setup_emsdk_env("${EMSDK_DIR}")
    require_program(EMCC emcc)

    execute_process(COMMAND ${EMCC} --version OUTPUT_VARIABLE _emcc_version)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" _active_emsdk_version "${_emcc_version}")
    if(EMSDK_VERSION AND NOT _active_emsdk_version STREQUAL EMSDK_VERSION)
        message(WARNING "Emscripten ${_active_emsdk_version} is active, but ${EMSDK_VERSION} is expected\n"
                        "(emsdk install ${EMSDK_VERSION} && emsdk activate ${EMSDK_VERSION})")
    endif()

    # Qt
    if(NOT QT_DIR)
        if(DEFINED ENV{QT_ROOT_DIR})
            set(QT_DIR "$ENV{QT_ROOT_DIR}")
        else()
            find_qt(QT_DIR)
        endif()
    endif()
    if(NOT QT_DIR OR NOT EXISTS "${QT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake")
        message(FATAL_ERROR "Qt for WebAssembly (wasm_singlethread) not found (set QT_DIR or $QT_ROOT_DIR)")
    endif()
    if(QT_VERSION AND NOT QT_DIR MATCHES "/${QT_VERSION}/")
        message(WARNING "Qt ${QT_VERSION} is expected, using ${QT_DIR}")
    endif()
    set(ENV{QT_ROOT_DIR} "${QT_DIR}")
    set(ENV{CMAKE_TOOLCHAIN_FILE} "${QT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake")

    message(STATUS "=== Build ${APP} (wasm) in ${BUILD_DIR} ===")
    message(STATUS "    Qt: ${QT_DIR}")
    message(STATUS "    Emscripten: ${_active_emsdk_version}")
    build("${ROOT}" "${BUILD_DIR}" ${CMAKE_ARGS} "-DMUSE_ARTIFACTS_DIR=${ARTIFACTS_DIR}")

    if(AUDIO_ENGINE)
        message(STATUS "=== Build the audio engine in ${AUDIO_BUILD_DIR} ===")
        build("${ROOT}/${AUDIO_ENGINE}" "${AUDIO_BUILD_DIR}" "-DMUSE_ARTIFACTS_DIR=${ARTIFACTS_DIR}")
    endif()
endif()

###########################################
# Run
###########################################
if(NOT NO_RUN)
    require_program(PYTHON python3 python)

    if(NOT EXISTS "${ARTIFACTS_DIR}/${APP}.wasm" OR NOT EXISTS "${ARTIFACTS_DIR}/${PAGE}")
        message(FATAL_ERROR "No build in ${ARTIFACTS_DIR}, run without NO_BUILD first")
    endif()

    set(_open_arg "--open")
    if(NO_OPEN)
        set(_open_arg "")
    endif()

    # A static server without caching, it also prints the app logs (the browser console).
    # NOTE The single-threaded build doesn't need the cross-origin isolation (COOP/COEP),
    # with these headers the main module doesn't load
    run(${PYTHON} -u "${CMAKE_CURRENT_LIST_DIR}/web_server.py"
        --dir "${ARTIFACTS_DIR}" --port ${PORT} --page "${PAGE}" ${_open_arg})
endif()
