# Uploads native debug information files to Sentry: a dSYM on macOS, a PDB on
# Windows, an unstripped ELF on Linux, each together with the shipped binary.
# Sentry reads these formats directly, no intermediate symbol files needed.
# sentry-cli from extdeps.
#
# Usage:
#   cmake -DFILES="<file>;<file>" -P upload_debug_files.cmake

set(HERE ${CMAKE_CURRENT_LIST_DIR})

set(FILES "" CACHE STRING "Debug files and binaries to upload")

set(SENTRY_URL "$ENV{SENTRY_URL}" CACHE STRING "Sentry URL")
set(SENTRY_AUTH_TOKEN "$ENV{SENTRY_AUTH_TOKEN}" CACHE STRING "Sentry Auth Token")
set(SENTRY_ORG "$ENV{SENTRY_ORG}" CACHE STRING "Sentry Organization")
set(SENTRY_PROJECT "$ENV{SENTRY_PROJECT}" CACHE STRING "Sentry Project")

# Check
if(NOT FILES)
    message(FATAL_ERROR "error: not set FILES")
endif()
if(NOT SENTRY_URL)
    message(FATAL_ERROR "error: not set SENTRY_URL")
endif()
if(NOT SENTRY_AUTH_TOKEN)
    message(FATAL_ERROR "error: not set SENTRY_AUTH_TOKEN")
endif()
if(NOT SENTRY_ORG)
    message(FATAL_ERROR "error: not set SENTRY_ORG")
endif()
if(NOT SENTRY_PROJECT)
    message(FATAL_ERROR "error: not set SENTRY_PROJECT")
endif()

message(STATUS "FILES: ${FILES}")
message(STATUS "SENTRY_URL: ${SENTRY_URL}")
message(STATUS "SENTRY_ORG: ${SENTRY_ORG}")
message(STATUS "SENTRY_PROJECT: ${SENTRY_PROJECT}")

set(LOCAL_ROOT_PATH "${HERE}/_deps")
set(EXTDEPS_DIR "${CMAKE_SOURCE_DIR}/muse_deps" CACHE PATH "muse_deps checkout")
include("${EXTDEPS_DIR}/buildtools/manifest.cmake")
require_tool(sentry-cli)
get_property(_bin_dir GLOBAL PROPERTY sentry-cli_BIN_DIR)
if(WIN32)
    set(SENTRY_CLI "${_bin_dir}/sentry-cli.exe")
else()
    set(SENTRY_CLI "${_bin_dir}/sentry-cli")
endif()

execute_process(COMMAND ${SENTRY_CLI} --version)

set(ENV{SENTRY_URL} ${SENTRY_URL})
set(ENV{SENTRY_AUTH_TOKEN} ${SENTRY_AUTH_TOKEN})

# Check what we are about to upload
#
# `debug-files check` fails only on files it cannot parse at all: one that
# simply carries no debug information is still reported as usable. So look at
# the features instead, and require the set as a whole to provide both halves
# of a readable crash report: `debug` for function names, files and lines,
# `unwind` for walking the stack.

set(HAS_DEBUG FALSE)
set(HAS_UNWIND FALSE)

foreach(FILE IN LISTS FILES)
    if(NOT EXISTS "${FILE}")
        message(FATAL_ERROR "error: ${FILE} not found")
    endif()

    message(STATUS "----- ${FILE}")

    execute_process(
        COMMAND ${SENTRY_CLI} debug-files check ${FILE}
        OUTPUT_VARIABLE output
        RESULT_VARIABLE result
    )

    message("${output}")

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to check ${FILE}, code: ${result}")
    endif()

    # Only the '> symtab, debug, unwind' lines, not the heading above them,
    # which contains the word 'debug' itself
    string(REGEX MATCHALL "Contained debug information:[ \t\r\n]*>[^\r\n]*" blocks "${output}")
    foreach(block IN LISTS blocks)
        string(REGEX REPLACE "^.*>" "" features "${block}")
        if(features MATCHES "debug")
            set(HAS_DEBUG TRUE)
        endif()
        if(features MATCHES "unwind")
            set(HAS_UNWIND TRUE)
        endif()
    endforeach()
endforeach()

if(NOT HAS_DEBUG)
    message(FATAL_ERROR "error: none of the files carries debug information\n"
                        "       the build has no debug info, or the files are stripped")
endif()

if(NOT HAS_UNWIND)
    message(FATAL_ERROR "error: none of the files carries unwind information")
endif()

# Upload
execute_process(
    COMMAND ${SENTRY_CLI} debug-files upload --wait -o ${SENTRY_ORG} -p ${SENTRY_PROJECT} ${FILES}
    RESULT_VARIABLE result
)

if(result EQUAL 0)
    message(STATUS "Success debug files uploaded")
else()
    message(FATAL_ERROR "Failed debug files uploaded, code: ${result}")
endif()
