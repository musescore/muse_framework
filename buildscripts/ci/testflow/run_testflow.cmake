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

# Runs testflow test cases, each in a separate application run with a clean user data directory.
#
#   1. For each test case runs: <app> --user-data-dir <dir> --test-case <name> --test-case-timeout <sec>
#      The application exits with 0 if the test case passed, otherwise with 1.
#      If the application hangs (e.g. its own timeout does not work), it is killed after the timeout plus a margin.
#   2. Writes to the output directory, for each test case:
#        <name>/output.log    the application output
#        <name>/userdata/     the user data of the run: AppData (settings, logs, session)
#                             and UserData (TestflowData: reports and saved files)
#   3. Prints the results. On GitHub Actions also adds them to the job summary
#   4. Fails if any test case failed
#
# On Linux without a display run it with a virtual one: xvfb-run -a cmake -P run_testflow.cmake ...
#
# Usage:
#   cmake -P run_testflow.cmake --app <path> [--test_case <name>]... [--test_cases_file <file>]
#                               [--scripts_dir <dir>] [--output_dir <dir>] [--timeout <sec>] [--speed <mode>]
#
#   --test_case         a test case name or file, can be repeated
#   --test_cases_file   a file with a test case per line, empty lines and lines starting with # are skipped
#   --scripts_dir       the test case scripts directory instead of the installed one
#   --output_dir        default: testflow_results
#   --timeout           the test case timeout in seconds, default: 300
#   --speed             the test case speed mode: Default|Fast|Normal|Slow

if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

cmake_minimum_required(VERSION 3.21)

###########################################
# Arguments
###########################################

set(APP "")
set(TEST_CASES "")
set(SCRIPTS_DIR "")
set(OUTPUT_DIR "testflow_results")
set(TIMEOUT 300)
set(SPEED "")

# The test case timeout includes the application startup, the margin is for the application exit
set(EXIT_TIMEOUT 60)

# CMAKE_ARGV0 is cmake, 1 is -P, 2 is this script
set(i 3)
while(i LESS CMAKE_ARGC)
    set(arg "${CMAKE_ARGV${i}}")
    math(EXPR i "${i} + 1")
    set(value "${CMAKE_ARGV${i}}")

    if(arg STREQUAL "--app")
        set(APP "${value}")
    elseif(arg STREQUAL "--test_case")
        list(APPEND TEST_CASES "${value}")
    elseif(arg STREQUAL "--test_cases_file")
        file(STRINGS "${value}" lines)
        foreach(line ${lines})
            string(STRIP "${line}" line)
            if(line AND NOT line MATCHES "^#")
                list(APPEND TEST_CASES "${line}")
            endif()
        endforeach()
    elseif(arg STREQUAL "--scripts_dir")
        get_filename_component(SCRIPTS_DIR "${value}" ABSOLUTE)
    elseif(arg STREQUAL "--output_dir")
        set(OUTPUT_DIR "${value}")
    elseif(arg STREQUAL "--timeout")
        set(TIMEOUT "${value}")
    elseif(arg STREQUAL "--speed")
        set(SPEED "${value}")
    else()
        message(FATAL_ERROR "Unknown parameter passed: ${arg}")
    endif()

    math(EXPR i "${i} + 1")
endwhile()

if(NOT APP)
    message(FATAL_ERROR "Usage: cmake -P run_testflow.cmake --app <path> [--test_case <name>]... [--test_cases_file <file>]")
endif()

if(NOT EXISTS "${APP}")
    message(FATAL_ERROR "Application not found: ${APP}")
endif()

if(NOT TEST_CASES)
    message(FATAL_ERROR "No test cases, use --test_case or --test_cases_file")
endif()

get_filename_component(OUTPUT_DIR "${OUTPUT_DIR}" ABSOLUTE)

if(SCRIPTS_DIR)
    set(ENV{MUSE_TESTFLOW_SCRIPTS_PATH} "${SCRIPTS_DIR}")
endif()

###########################################
# Run
###########################################

string(ASCII 27 ESC)

function(log_value log_text pattern out)
    string(REGEX MATCH "${pattern}([^\n]*)" _ "${log_text}")
    string(REGEX REPLACE "${ESC}\\[[0-9;]*m" "" value "${CMAKE_MATCH_1}")
    string(STRIP "${value}" value)
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

math(EXPR APP_TIMEOUT "${TIMEOUT} + ${EXIT_TIMEOUT}")

set(PASSED_COUNT 0)
set(FAILED_COUNT 0)
set(SUMMARY "| Test case | Result | Time | Failed step | Error |\n|---|---|---|---|---|\n")

foreach(test_case ${TEST_CASES})
    get_filename_component(case_dir_name "${test_case}" NAME)
    string(REGEX REPLACE "\\.js$" "" case_dir_name "${case_dir_name}")
    set(case_dir "${OUTPUT_DIR}/${case_dir_name}")
    file(REMOVE_RECURSE "${case_dir}")
    file(MAKE_DIRECTORY "${case_dir}")

    set(args --user-data-dir "${case_dir}/userdata" --test-case "${test_case}" --test-case-timeout ${TIMEOUT})
    if(SPEED)
        list(APPEND args --test-case-speed ${SPEED})
    endif()

    message(STATUS "=== Run ${test_case} ===")

    string(TIMESTAMP start "%s")
    execute_process(
        COMMAND "${APP}" ${args}
        RESULT_VARIABLE result
        OUTPUT_FILE "${case_dir}/output.log"
        ERROR_FILE "${case_dir}/output.log"
        TIMEOUT ${APP_TIMEOUT}
    )
    string(TIMESTAMP end "%s")
    math(EXPR duration "${end} - ${start}")

    file(READ "${case_dir}/output.log" log_text)
    log_value("${log_text}" "\\[testflow\\] failed step: " failed_step)
    log_value("${log_text}" "\\[testflow\\] error: " error)

    if(result STREQUAL "0")
        set(status "PASSED")
    elseif(result STREQUAL "1")
        set(status "FAILED")
        if(NOT error)
            log_value("${log_text}" "\\[testflow\\] FAILED: " error)
        endif()
    elseif(result MATCHES "timeout")
        set(status "HUNG")
        set(error "killed after ${APP_TIMEOUT} sec")
    else()
        set(status "CRASHED")
        set(error "${result}")
    endif()

    if(status STREQUAL "PASSED")
        math(EXPR PASSED_COUNT "${PASSED_COUNT} + 1")
        message(STATUS "${test_case}: ${status} (${duration} sec)")
    else()
        math(EXPR FAILED_COUNT "${FAILED_COUNT} + 1")
        message(STATUS "${test_case}: ${status} (${duration} sec), step: ${failed_step}, error: ${error}")
    endif()

    string(REPLACE "|" "\\|" failed_step "${failed_step}")
    string(REPLACE "|" "\\|" error "${error}")
    string(APPEND SUMMARY "| ${test_case} | ${status} | ${duration} s | ${failed_step} | ${error} |\n")
endforeach()

###########################################
# Results
###########################################

set(RESULT "Testflow: ${PASSED_COUNT} passed, ${FAILED_COUNT} failed")
message(STATUS "${RESULT}")
message(STATUS "Results: ${OUTPUT_DIR}")

if(DEFINED ENV{GITHUB_STEP_SUMMARY} AND NOT "$ENV{GITHUB_STEP_SUMMARY}" STREQUAL "")
    file(APPEND "$ENV{GITHUB_STEP_SUMMARY}" "### ${RESULT}\n\n${SUMMARY}\n")
endif()

if(FAILED_COUNT GREATER 0)
    message(FATAL_ERROR "${RESULT}")
endif()
