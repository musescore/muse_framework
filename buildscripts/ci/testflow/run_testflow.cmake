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
# The test cases are run by ctest, the script runs itself (--run_one) for each test case, --jobs of them at once.
#
# On Linux without a display run it with a virtual one: xvfb-run -a cmake -P run_testflow.cmake ...
#
# Usage:
#   cmake -P run_testflow.cmake --app <path> [--test_case <name>]... [--test_cases_file <file>]
#                               [--scripts_dir <dir>] [--output_dir <dir>] [--timeout <sec>] [--speed <mode>]
#                               [--jobs <count>]
#
#   --test_case         a test case name or file, can be repeated
#   --test_cases_file   a file with a test case per line, empty lines and lines starting with # are skipped
#   --scripts_dir       the test case scripts directory instead of the installed one
#   --output_dir        default: testflow_results
#   --timeout           the test case timeout in seconds, default: 300
#   --speed             the test case speed mode: Default|Fast|Normal|Slow
#   --jobs              the number of test cases running at once, default: 1

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
set(JOBS 1)
set(RUN_ONE "") # internal: run one test case and write its result

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
    elseif(arg STREQUAL "--jobs")
        set(JOBS "${value}")
    elseif(arg STREQUAL "--run_one")
        set(RUN_ONE "${value}")
    else()
        message(FATAL_ERROR "Unknown parameter passed: ${arg}")
    endif()

    math(EXPR i "${i} + 1")
endwhile()

if(NOT APP)
    message(FATAL_ERROR "Usage: cmake -P run_testflow.cmake --app <path> [--test_case <name>]... [--test_cases_file <file>]")
endif()

# Absolute, the test case runs are in another working directory
get_filename_component(APP "${APP}" ABSOLUTE)
get_filename_component(OUTPUT_DIR "${OUTPUT_DIR}" ABSOLUTE)

if(NOT EXISTS "${APP}")
    message(FATAL_ERROR "Application not found: ${APP}")
endif()
math(EXPR APP_TIMEOUT "${TIMEOUT} + ${EXIT_TIMEOUT}")

###########################################
# Run one test case
###########################################

function(case_dir_of test_case out)
    get_filename_component(name "${test_case}" NAME)
    string(REGEX REPLACE "\\.js$" "" name "${name}")
    set(${out} "${OUTPUT_DIR}/${name}" PARENT_SCOPE)
endfunction()

# Runs the test case and writes its result to <case dir>/result.cmake: status, duration and error if it is not in the log
if(RUN_ONE)
    case_dir_of("${RUN_ONE}" case_dir)
    file(REMOVE_RECURSE "${case_dir}")
    file(MAKE_DIRECTORY "${case_dir}")

    set(args --user-data-dir "${case_dir}/userdata" --test-case "${RUN_ONE}" --test-case-timeout ${TIMEOUT})
    if(SPEED)
        list(APPEND args --test-case-speed ${SPEED})
    endif()

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

    set(error "")
    if(result STREQUAL "0")
        set(status "PASSED")
    elseif(result STREQUAL "1")
        set(status "FAILED")
    elseif(result MATCHES "timeout")
        set(status "HUNG")
        set(error "killed after ${APP_TIMEOUT} sec")
    else()
        set(status "CRASHED")
        set(error "${result}")
    endif()

    # The error is in a bracket argument, it must not close it
    string(REPLACE "]==]" "] ==]" error "${error}")
    file(WRITE "${case_dir}/result.cmake"
        "set(status ${status})\nset(duration ${duration})\nset(error [==[${error}]==])\n")

    # So ctest shows the result too
    if(NOT status STREQUAL "PASSED")
        message(FATAL_ERROR "${RUN_ONE}: ${status}")
    endif()
    return()
endif()

###########################################
# Test cases
###########################################

if(NOT TEST_CASES)
    message(FATAL_ERROR "No test cases, use --test_case or --test_cases_file")
endif()

if(NOT JOBS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "Bad --jobs value: ${JOBS}, expected a positive number")
endif()

# Each test case has its own results directory, by the file name.
# Cleared, so a result of a previous run cannot be taken for a case that does not run
set(case_dirs "")
foreach(test_case ${TEST_CASES})
    case_dir_of("${test_case}" case_dir)
    if(case_dir IN_LIST case_dirs)
        message(FATAL_ERROR "Duplicate test case: ${test_case}, the results directory is the same: ${case_dir}")
    endif()
    list(APPEND case_dirs "${case_dir}")
endforeach()
file(REMOVE_RECURSE ${case_dirs})

###########################################
# Run
###########################################

# Inherited by the test case runs
if(SCRIPTS_DIR)
    set(ENV{MUSE_TESTFLOW_SCRIPTS_PATH} "${SCRIPTS_DIR}")
endif()

# ctest runs the test cases in the given order, each by this script with --run_one
set(ctest_dir "${OUTPUT_DIR}/.ctest")
file(REMOVE_RECURSE "${ctest_dir}")

set(run_one_args --app "${APP}" --output_dir "${OUTPUT_DIR}" --timeout ${TIMEOUT})
if(SPEED)
    list(APPEND run_one_args --speed ${SPEED})
endif()

set(tests "")
foreach(test_case ${TEST_CASES})
    set(command "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_LIST_FILE}" ${run_one_args} --run_one "${test_case}")
    list(JOIN command "]==] [==[" command)
    string(APPEND tests "add_test([==[${test_case}]==] [==[${command}]==])\n")
endforeach()
file(WRITE "${ctest_dir}/CTestTestfile.cmake" "${tests}")

# Not 0 also if a test case failed
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${ctest_dir}" --parallel ${JOBS} --output-on-failure
    RESULT_VARIABLE ctest_result)

###########################################
# Results
###########################################

string(ASCII 27 ESC)

# The value of the first log line "[testflow] <name>: <value>" without colors.
# NOTE The log is not read as a list of lines: the color codes contain "[", which disables ";" as the list separator
function(log_value log_text name out)
    string(REGEX MATCH "\\[testflow\\] ${name}: ([^\n]*)" _ "${log_text}")
    string(REGEX REPLACE "${ESC}\\[[0-9;]*m" "" value "${CMAKE_MATCH_1}")
    string(STRIP "${value}" value)
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

set(PASSED_COUNT 0)
set(FAILED_COUNT 0)
set(SUMMARY "| Test case | Result | Time | Failed step | Error |\n|---|---|---|---|---|\n")

foreach(test_case ${TEST_CASES})
    case_dir_of("${test_case}" case_dir)

    set(status "CRASHED")
    set(duration "?")
    set(error "no result")
    if(EXISTS "${case_dir}/result.cmake")
        include("${case_dir}/result.cmake")
    endif()

    set(failed_step "")
    if(NOT status STREQUAL "PASSED" AND EXISTS "${case_dir}/output.log")
        file(READ "${case_dir}/output.log" log_text)
        log_value("${log_text}" "failed step" failed_step)
        if(NOT error)
            log_value("${log_text}" "error" error)
        endif()
        if(NOT error)
            log_value("${log_text}" "FAILED" error)
        endif()
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

set(RESULT "Testflow: ${PASSED_COUNT} passed, ${FAILED_COUNT} failed")
message(STATUS "${RESULT}")
message(STATUS "Results: ${OUTPUT_DIR}")

if(DEFINED ENV{GITHUB_STEP_SUMMARY} AND NOT "$ENV{GITHUB_STEP_SUMMARY}" STREQUAL "")
    file(APPEND "$ENV{GITHUB_STEP_SUMMARY}" "### ${RESULT}\n\n${SUMMARY}\n")
endif()

if(FAILED_COUNT GREATER 0)
    message(FATAL_ERROR "${RESULT}")
endif()

if(NOT ctest_result STREQUAL "0")
    message(FATAL_ERROR "Testflow: ctest failed: ${ctest_result}")
endif()
