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

# Makes the code coverage report of a build with coverage enabled, after its tests have run.
#
#   1. Captures coverage with lcov: a zero baseline from .gcno files (so compiled code that no test
#      runs is counted as uncovered) merged with the data of the test run (.gcda files)
#   2. Removes code that should not be counted: system and Qt headers, tests, third-party code,
#      generated files in the build directory, test infrastructure, dev tools, and --exclude patterns
#   3. Writes to the output directory:
#        filtered_coverage.info   lcov tracefile
#        coverage_badge.svg       badge with the lines coverage (template: coverage_badge.svg.in)
#        coverage_report/         HTML report (genhtml)
#   4. Prints lines and functions coverage. On GitHub Actions also adds them to the job summary
#      and exports COVERAGE and FUNCTIONS_COVERAGE to the next steps (GITHUB_ENV)
#
# Requires lcov and genhtml.
#
# Usage:
#   cmake -P make_code_coverage.cmake --build_dir <dir> [--output_dir <dir>] [--exclude <pattern>]...
#   cmake -P make_code_coverage.cmake --tracefile <lcov.info> [--output_dir <dir>] [--exclude <pattern>]...
#
#   --tracefile skips capturing and makes the report from an existing tracefile

if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

cmake_minimum_required(VERSION 3.21)

###########################################
# Arguments
###########################################

set(BUILD_DIR "")
set(OUTPUT_DIR ".")
set(TRACEFILE "")
set(EXTRA_EXCLUDES "")

# CMAKE_ARGV0 is cmake, 1 is -P, 2 is this script
set(i 3)
while(i LESS CMAKE_ARGC)
    set(arg "${CMAKE_ARGV${i}}")
    math(EXPR i "${i} + 1")
    set(value "${CMAKE_ARGV${i}}")

    if(arg STREQUAL "--build_dir")
        set(BUILD_DIR "${value}")
    elseif(arg STREQUAL "--output_dir")
        set(OUTPUT_DIR "${value}")
    elseif(arg STREQUAL "--tracefile")
        set(TRACEFILE "${value}")
    elseif(arg STREQUAL "--exclude")
        list(APPEND EXTRA_EXCLUDES "${value}")
    else()
        message(FATAL_ERROR "Unknown parameter passed: ${arg}")
    endif()

    math(EXPR i "${i} + 1")
endwhile()

if(NOT BUILD_DIR AND NOT TRACEFILE)
    message(FATAL_ERROR
        "Usage: cmake -P make_code_coverage.cmake --build_dir <dir> [--output_dir <dir>] [--exclude <pattern>]...\n"
        "       cmake -P make_code_coverage.cmake --tracefile <lcov.info> [--output_dir <dir>] [--exclude <pattern>]...")
endif()

get_filename_component(OUTPUT_DIR "${OUTPUT_DIR}" ABSOLUTE)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

set(WORK_DIR "${OUTPUT_DIR}/.code_coverage_tmp")
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

set(INFO_FILE "${OUTPUT_DIR}/filtered_coverage.info")
set(BADGE_FILE "${OUTPUT_DIR}/coverage_badge.svg")
set(REPORT_DIR "${OUTPUT_DIR}/coverage_report")

# lcov 2.x reports problems of gcc's data as errors; these do not affect the result
set(CAPTURE_IGNORE "mismatch,inconsistent,gcov,negative")
set(SUMMARY_IGNORE "inconsistent,corrupt,unsupported,empty,negative,mismatch,gcov")
set(GENHTML_IGNORE "inconsistent,corrupt,unsupported,empty,negative,mismatch,source")

function(fail msg)
    file(REMOVE_RECURSE "${WORK_DIR}")
    message(FATAL_ERROR "${msg}")
endfunction()

function(run name)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        fail("${name} failed (${result})")
    endif()
endfunction()

###########################################
# 1. Capture
###########################################

if(TRACEFILE)
    message(STATUS "=== Using tracefile ${TRACEFILE} ===")
    file(COPY_FILE "${TRACEFILE}" "${WORK_DIR}/coverage.info")
else()
    get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)
    if(NOT IS_DIRECTORY "${BUILD_DIR}")
        fail("Build directory not found: ${BUILD_DIR}")
    endif()

    message(STATUS "=== Capture coverage from ${BUILD_DIR} ===")
    run("Capture of the zero baseline"
        lcov --capture --initial --directory "${BUILD_DIR}" --output-file "${WORK_DIR}/coverage_base.info" --ignore-errors "${CAPTURE_IGNORE}")
    run("Capture of the test run"
        lcov --capture --directory "${BUILD_DIR}" --output-file "${WORK_DIR}/coverage_tests.info" --ignore-errors "${CAPTURE_IGNORE}")
    run("Merge of the baseline and the test run"
        lcov --add-tracefile "${WORK_DIR}/coverage_base.info" --add-tracefile "${WORK_DIR}/coverage_tests.info"
             --output-file "${WORK_DIR}/coverage.info" --ignore-errors "mismatch,inconsistent,negative")
endif()

###########################################
# 2. Filter
###########################################

set(EXCLUDES
    "/usr/*"                  # system headers
    "*/Qt/*"                  # Qt headers
    "*framework/Headers/*"    # macOS framework headers
    "*.framework/*"
    "*/tests/*"               # tests themselves
    "*/thirdparty/*"          # third-party code
    "*/hb-*"                  # HarfBuzz
    "*/moc_*"                 # Qt generated code
    "*/framework/testing/*"   # test infrastructure
    "*/devtools/*"            # developer tools
)
if(BUILD_DIR)
    list(APPEND EXCLUDES "${BUILD_DIR}/*") # generated code and dependencies fetched into the build directory
endif()
list(APPEND EXCLUDES ${EXTRA_EXCLUDES})

list(JOIN EXCLUDES " " EXCLUDES_STR)
message(STATUS "=== Remove: ${EXCLUDES_STR} ===")
run("Removing of excluded files"
    lcov --remove "${WORK_DIR}/coverage.info" ${EXCLUDES}
         --output-file "${INFO_FILE}" --ignore-errors "unused,inconsistent")

###########################################
# 3. Summary and badge
###########################################

execute_process(
    COMMAND lcov --summary "${INFO_FILE}" --ignore-errors "${SUMMARY_IGNORE}"
    OUTPUT_VARIABLE SUMMARY
    ERROR_VARIABLE SUMMARY
    RESULT_VARIABLE result
)

string(REGEX MATCH "lines\\.+: *([0-9.]+)%" _ "${SUMMARY}")
set(COVERAGE "${CMAKE_MATCH_1}")
string(REGEX MATCH "functions\\.+: *([0-9.]+)%" _ "${SUMMARY}")
set(FUNCTIONS_COVERAGE "${CMAKE_MATCH_1}")
if(NOT FUNCTIONS_COVERAGE)
    set(FUNCTIONS_COVERAGE "?")
endif()

if(NOT COVERAGE)
    fail("Failed to read the lines coverage from lcov summary:\n${SUMMARY}")
endif()

# Badge shows the coverage rounded to an integer
string(REGEX MATCH "^([0-9]+)(\\.([0-9]))?" _ "${COVERAGE}")
set(BADGE_PERCENT "${CMAKE_MATCH_1}")
if(CMAKE_MATCH_3 AND CMAKE_MATCH_3 GREATER_EQUAL 5)
    math(EXPR BADGE_PERCENT "${BADGE_PERCENT} + 1")
endif()

# Badge color by the shown coverage: red below 50%, yellow below 75%, green from 75%
if(BADGE_PERCENT LESS 50)
    set(BADGE_COLOR "#e05d44")
elseif(BADGE_PERCENT LESS 75)
    set(BADGE_COLOR "#dfb317")
else()
    set(BADGE_COLOR "#4c1")
endif()

configure_file("${CMAKE_CURRENT_LIST_DIR}/coverage_badge.svg.in" "${BADGE_FILE}" @ONLY)

###########################################
# 4. HTML report
###########################################

# Without per-file function pages and extra sorted index pages: they make up about a third of the report
message(STATUS "=== Make HTML report ===")
file(REMOVE_RECURSE "${REPORT_DIR}")
execute_process(
    COMMAND genhtml "${INFO_FILE}" --output-directory "${REPORT_DIR}"
            --no-function-coverage --no-sort --ignore-errors "${GENHTML_IGNORE}"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    file(REMOVE_RECURSE "${REPORT_DIR}")
    fail("genhtml failed (${result}), the HTML report is incomplete")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")

###########################################
# Results
###########################################

set(RESULT "Code coverage: ${COVERAGE}% of lines, ${FUNCTIONS_COVERAGE}% of functions")
message(STATUS "${RESULT}")

if(DEFINED ENV{GITHUB_STEP_SUMMARY} AND NOT "$ENV{GITHUB_STEP_SUMMARY}" STREQUAL "")
    file(APPEND "$ENV{GITHUB_STEP_SUMMARY}" "### ${RESULT}\n")
endif()
if(DEFINED ENV{GITHUB_ENV} AND NOT "$ENV{GITHUB_ENV}" STREQUAL "")
    file(APPEND "$ENV{GITHUB_ENV}" "COVERAGE=${COVERAGE}\nFUNCTIONS_COVERAGE=${FUNCTIONS_COVERAGE}\n")
endif()
