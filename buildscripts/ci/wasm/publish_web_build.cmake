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

# Publishes a web (wasm) build of an app to a git repository that serves it as a site
# (e.g. with GitHub Pages). The site itself (index.html etc.) lives in that repository;
# this script only updates the build in it and pushes one commit:
#
#   history.csv                a row is appended: date,commit,branch,version
#                              (commit is the SHA of the source code the build was made from)
#   <target_dir>/              replaced with the given build
#   <target_dir>/build.json    information about the build: version, commit, branch, date
#
# A build weighs about a hundred megabytes, so by default the history of the repository is not kept:
# the pushed commit has no parent and replaces the branch (force push), the repository holds only
# the latest build. Pass --keep_history ON to push a regular commit on top of the branch.
#
# The script does not authenticate by itself: pass a repository URL the runner can push to
# (e.g. an SSH URL with GIT_SSH_COMMAND set to a deploy key).
#
# Requires git.
#
# Usage:
#   cmake -P publish_web_build.cmake --repo_url <url> --build_dir <dir> [--target_dir app]
#         [--exclude <name>]... [--build_version <version>] [--branch main] [--keep_history OFF]
#         [--source_repo <owner/name>] [--source_sha <sha>] [--source_branch <branch>]
#
#   --exclude        a file or directory name in the build to skip, can be repeated
#   --source_repo, --source_sha and --source_branch default to GITHUB_REPOSITORY, GITHUB_SHA and GITHUB_REF_NAME
#
# Example:
#   cmake -P publish_web_build.cmake \
#     --repo_url git@github.com:<org>/<web_builds_repo>.git \
#     --build_dir build.artifacts \
#     --exclude env \
#     --build_version 4.7.0.123

if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

cmake_minimum_required(VERSION 3.21)

###########################################
# Arguments
###########################################

set(REPO_URL "")
set(BRANCH "main")
set(BUILD_DIR "")
set(TARGET_DIR "app")
set(EXCLUDES "")
set(VERSION "")
set(KEEP_HISTORY OFF)
set(SOURCE_REPO "$ENV{GITHUB_REPOSITORY}")
set(SOURCE_SHA "$ENV{GITHUB_SHA}")
set(SOURCE_BRANCH "$ENV{GITHUB_REF_NAME}")

# GitHub rejects files larger than this
set(MAX_FILE_SIZE 104857600)

# CMAKE_ARGV0 is cmake, 1 is -P, 2 is this script
set(i 3)
while(i LESS CMAKE_ARGC)
    set(arg "${CMAKE_ARGV${i}}")
    math(EXPR i "${i} + 1")
    if(NOT i LESS CMAKE_ARGC)
        message(FATAL_ERROR "No value for parameter: ${arg}")
    endif()
    set(value "${CMAKE_ARGV${i}}")

    if(arg STREQUAL "--repo_url")
        set(REPO_URL "${value}")
    elseif(arg STREQUAL "--branch")
        set(BRANCH "${value}")
    elseif(arg STREQUAL "--build_dir")
        set(BUILD_DIR "${value}")
    elseif(arg STREQUAL "--target_dir")
        set(TARGET_DIR "${value}")
    elseif(arg STREQUAL "--exclude")
        list(APPEND EXCLUDES "${value}")
    elseif(arg STREQUAL "--build_version")
        set(VERSION "${value}")
    elseif(arg STREQUAL "--keep_history")
        set(KEEP_HISTORY "${value}")
    elseif(arg STREQUAL "--source_repo")
        set(SOURCE_REPO "${value}")
    elseif(arg STREQUAL "--source_sha")
        set(SOURCE_SHA "${value}")
    elseif(arg STREQUAL "--source_branch")
        set(SOURCE_BRANCH "${value}")
    else()
        message(FATAL_ERROR "Unknown parameter passed: ${arg}")
    endif()

    math(EXPR i "${i} + 1")
endwhile()

if(NOT REPO_URL OR NOT BUILD_DIR)
    message(FATAL_ERROR
        "Usage: cmake -P publish_web_build.cmake --repo_url <url> --build_dir <dir> [--target_dir app]\n"
        "       [--exclude <name>]... [--build_version <version>] [--branch main] [--keep_history OFF]\n"
        "       [--source_repo <owner/name>] [--source_sha <sha>] [--source_branch <branch>]\n"
        "--source_repo, --source_sha and --source_branch default to GITHUB_REPOSITORY, GITHUB_SHA and GITHUB_REF_NAME")
endif()

# The target directory is removed before the build is copied, so it must be a subdirectory
if(TARGET_DIR STREQUAL "" OR TARGET_DIR MATCHES "^[/\\.]" OR TARGET_DIR MATCHES "(^|[/\\])\\.\\.?([/\\]|$)" OR TARGET_DIR MATCHES ":")
    message(FATAL_ERROR "The target directory must be a relative subdirectory of the repository: ${TARGET_DIR}")
endif()

get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)
if(NOT IS_DIRECTORY "${BUILD_DIR}")
    message(FATAL_ERROR "Build not found: ${BUILD_DIR}")
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

# The values go to a csv row and a json string
foreach(var VERSION SOURCE_BRANCH)
    string(REGEX REPLACE "[,\"\\\r\n]" "_" ${var} "${${var}}")
endforeach()

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
set(WORK_DIR "${TMP_DIR}/web_build_publish_${RANDOM_SUFFIX}")
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

message(STATUS "Publishing the web build ${VERSION} of ${SOURCE_NAME} to ${REPO_URL} (${BRANCH}/${TARGET_DIR})")

execute_process(
    COMMAND git clone -q --depth 1 --branch "${BRANCH}" "${REPO_URL}" "${REPO_DIR}"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    fail("git clone failed (${result})")
endif()

# The cloned tip of the branch: the force push replaces only it, a concurrent update fails the push
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY "${REPO_DIR}"
    OUTPUT_VARIABLE CLONED_TIP
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0 OR NOT CLONED_TIP)
    fail("git rev-parse failed (${result})")
endif()

###########################################
# Update
###########################################

string(TIMESTAMP DATE "%Y-%m-%dT%H:%M:%SZ" UTC)

if(NOT EXISTS "${REPO_DIR}/history.csv")
    file(WRITE "${REPO_DIR}/history.csv" "date,commit,branch,version\n")
endif()
file(APPEND "${REPO_DIR}/history.csv" "${DATE},${SOURCE_SHA},${SOURCE_BRANCH},${VERSION}\n")

set(copy_excludes "")
foreach(name ${EXCLUDES})
    list(APPEND copy_excludes PATTERN "${name}" EXCLUDE)
endforeach()

file(REMOVE_RECURSE "${REPO_DIR}/${TARGET_DIR}")
file(MAKE_DIRECTORY "${REPO_DIR}/${TARGET_DIR}")
file(COPY "${BUILD_DIR}/" DESTINATION "${REPO_DIR}/${TARGET_DIR}" ${copy_excludes})

file(WRITE "${REPO_DIR}/${TARGET_DIR}/build.json"
"{
  \"version\": \"${VERSION}\",
  \"repository\": \"${SOURCE_REPO}\",
  \"commit\": \"${SOURCE_SHA}\",
  \"branch\": \"${SOURCE_BRANCH}\",
  \"date\": \"${DATE}\"
}
")

file(GLOB_RECURSE published_files LIST_DIRECTORIES false "${REPO_DIR}/${TARGET_DIR}/*")
foreach(f ${published_files})
    file(SIZE "${f}" size)
    if(size GREATER MAX_FILE_SIZE)
        fail("The file is too large for GitHub (${size} bytes): ${f}")
    endif()
endforeach()

###########################################
# Commit and push
###########################################

set(COMMIT_MESSAGE "Web build ${VERSION} of ${SOURCE_NAME}")
if(SOURCE_BRANCH)
    string(APPEND COMMIT_MESSAGE " (${SOURCE_BRANCH})")
endif()

git("config" config user.name "github-actions[bot]")
git("config" config user.email "41898282+github-actions[bot]@users.noreply.github.com")

if(KEEP_HISTORY)
    git("add" add -A)
    git("commit" commit -q -m "${COMMIT_MESSAGE}")
    git("push" push -q origin "${BRANCH}")
else()
    git("checkout" checkout -q --orphan web_build_publish)
    git("add" add -A)
    git("commit" commit -q -m "${COMMIT_MESSAGE}")
    git("push" push -q "--force-with-lease=refs/heads/${BRANCH}:${CLONED_TIP}" origin "web_build_publish:refs/heads/${BRANCH}")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")

message(STATUS "Published")
