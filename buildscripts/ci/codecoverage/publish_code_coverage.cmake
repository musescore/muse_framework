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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Publishes code coverage results to a git repository that serves them as a site
# (e.g. with GitHub Pages). The site itself (index.html etc.) lives in that repository;
# this script only updates the data in it and pushes one commit:
#
#   history.csv                a row is appended: date,commit,lines,functions
#                              (commit is the SHA of the source code the coverage was measured on)
#   report/                    replaced with the given HTML report (e.g. genhtml output)
#   badge/coverage_badge.svg   replaced with the given badge
#
# The script does not authenticate by itself: pass a repository URL the runner can push to
# (e.g. an SSH URL with GIT_SSH_COMMAND set to a deploy key).
#
# Requires git.
#
# Usage:
#   cmake -P publish_code_coverage.cmake --repo_url <url> --report_dir <dir> --badge <svg> --lines <percent>
#         [--functions <percent>] [--branch main] [--source_repo <owner/name>] [--source_sha <sha>]
#
#   --source_repo and --source_sha default to GITHUB_REPOSITORY and GITHUB_SHA
#
# Example:
#   cmake -P publish_code_coverage.cmake \
#     --repo_url git@github.com:<org>/<coverage_repo>.git \
#     --report_dir coverage_report \
#     --badge coverage_badge.svg \
#     --lines 45.0 \
#     --functions 44.3

if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

cmake_minimum_required(VERSION 3.21)

###########################################
# Arguments
###########################################

set(REPO_URL "")
set(BRANCH "main")
set(REPORT_DIR "")
set(BADGE_FILE "")
set(LINES "")
set(FUNCTIONS "")
set(SOURCE_REPO "$ENV{GITHUB_REPOSITORY}")
set(SOURCE_SHA "$ENV{GITHUB_SHA}")

# CMAKE_ARGV0 is cmake, 1 is -P, 2 is this script
set(i 3)
while(i LESS CMAKE_ARGC)
    set(arg "${CMAKE_ARGV${i}}")
    math(EXPR i "${i} + 1")
    set(value "${CMAKE_ARGV${i}}")

    if(arg STREQUAL "--repo_url")
        set(REPO_URL "${value}")
    elseif(arg STREQUAL "--branch")
        set(BRANCH "${value}")
    elseif(arg STREQUAL "--report_dir")
        set(REPORT_DIR "${value}")
    elseif(arg STREQUAL "--badge")
        set(BADGE_FILE "${value}")
    elseif(arg STREQUAL "--lines")
        set(LINES "${value}")
    elseif(arg STREQUAL "--functions")
        set(FUNCTIONS "${value}")
    elseif(arg STREQUAL "--source_repo")
        set(SOURCE_REPO "${value}")
    elseif(arg STREQUAL "--source_sha")
        set(SOURCE_SHA "${value}")
    else()
        message(FATAL_ERROR "Unknown parameter passed: ${arg}")
    endif()

    math(EXPR i "${i} + 1")
endwhile()

if(NOT REPO_URL OR NOT REPORT_DIR OR NOT BADGE_FILE OR "${LINES}" STREQUAL "")
    message(FATAL_ERROR
        "Usage: cmake -P publish_code_coverage.cmake --repo_url <url> --report_dir <dir> --badge <svg> --lines <percent>\n"
        "       [--functions <percent>] [--branch main] [--source_repo <owner/name>] [--source_sha <sha>]\n"
        "--source_repo and --source_sha default to GITHUB_REPOSITORY and GITHUB_SHA")
endif()

get_filename_component(REPORT_DIR "${REPORT_DIR}" ABSOLUTE)
get_filename_component(BADGE_FILE "${BADGE_FILE}" ABSOLUTE)

if(NOT EXISTS "${REPORT_DIR}/index.html")
    message(FATAL_ERROR "Report not found: ${REPORT_DIR}/index.html")
endif()
if(NOT EXISTS "${BADGE_FILE}")
    message(FATAL_ERROR "Badge not found: ${BADGE_FILE}")
endif()

if(NOT LINES MATCHES "^[0-9.]+$")
    message(FATAL_ERROR "Lines coverage is not a number: ${LINES}")
endif()
# Non-numeric values (e.g. "?" when lcov did not print them) are stored as empty
if(NOT FUNCTIONS MATCHES "^[0-9.]+$")
    set(FUNCTIONS "")
endif()

if(NOT SOURCE_SHA)
    execute_process(
        COMMAND git rev-parse HEAD
        OUTPUT_VARIABLE SOURCE_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT SOURCE_SHA)
        set(SOURCE_SHA "unknown")
    endif()
endif()
string(SUBSTRING "${SOURCE_SHA}" 0 7 SOURCE_SHORT_SHA)

if(SOURCE_REPO)
    set(SOURCE_NAME "${SOURCE_REPO}@${SOURCE_SHORT_SHA}")
else()
    set(SOURCE_NAME "${SOURCE_SHORT_SHA}")
endif()

###########################################
# Clone
###########################################

foreach(tmp "$ENV{RUNNER_TEMP}" "$ENV{TMPDIR}" "$ENV{TEMP}" "/tmp")
    if(tmp AND IS_DIRECTORY "${tmp}")
        set(TMP_DIR "${tmp}")
        break()
    endif()
endforeach()
string(RANDOM LENGTH 8 ALPHABET "abcdefghijklmnopqrstuvwxyz0123456789" RANDOM_SUFFIX)
set(WORK_DIR "${TMP_DIR}/code_coverage_publish_${RANDOM_SUFFIX}")
set(REPO_DIR "${WORK_DIR}/repo")
file(MAKE_DIRECTORY "${WORK_DIR}")

function(fail msg)
    file(REMOVE_RECURSE "${WORK_DIR}")
    message(FATAL_ERROR "${msg}")
endfunction()

function(git name)
    execute_process(COMMAND git ${ARGN} WORKING_DIRECTORY "${REPO_DIR}" RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        fail("git ${name} failed (${result})")
    endif()
endfunction()

message(STATUS "Publishing coverage ${LINES}% of ${SOURCE_NAME} to ${REPO_URL} (${BRANCH})")

execute_process(
    COMMAND git clone -q --depth 1 --branch "${BRANCH}" "${REPO_URL}" "${REPO_DIR}"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    fail("git clone failed (${result})")
endif()

###########################################
# Update
###########################################

if(NOT EXISTS "${REPO_DIR}/history.csv")
    file(WRITE "${REPO_DIR}/history.csv" "date,commit,lines,functions\n")
endif()
string(TIMESTAMP DATE "%Y-%m-%dT%H:%M:%SZ" UTC)
file(APPEND "${REPO_DIR}/history.csv" "${DATE},${SOURCE_SHA},${LINES},${FUNCTIONS}\n")

file(REMOVE_RECURSE "${REPO_DIR}/report")
file(COPY "${REPORT_DIR}/" DESTINATION "${REPO_DIR}/report")
file(MAKE_DIRECTORY "${REPO_DIR}/badge")
file(COPY_FILE "${BADGE_FILE}" "${REPO_DIR}/badge/coverage_badge.svg")

###########################################
# Commit and push
###########################################

git("config" config user.name "github-actions[bot]")
git("config" config user.email "41898282+github-actions[bot]@users.noreply.github.com")
git("add" add -A)
git("commit" commit -q -m "Code coverage ${LINES}% for ${SOURCE_NAME}")
git("push" push -q origin "${BRANCH}")

file(REMOVE_RECURSE "${WORK_DIR}")

message(STATUS "Published")
