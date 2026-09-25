# Collects the debug information of a macOS binary into a dSYM bundle.
#
# On macOS the linker leaves the DWARF in the object files and puts only a
# debug map into the executable, so this has to run right after the build,
# while both are still around: packaging usually strips the binary and drops
# every dSYM found inside the app bundle. For the same reason the output
# should be written outside the bundle.
#
# The dSYM keeps matching the shipped binary afterwards, because LC_UUID
# survives strip, install_name_tool and codesign.
#
# Usage:
#   cmake -DBIN=<binary> [-DDSYM=<output.dSYM>] -P generate_dsym.cmake

set(BIN "" CACHE STRING "Path to the binary")
set(DSYM "" CACHE STRING "Path to the dSYM bundle to write, default '<binary>.dSYM'")

# Check
if(NOT BIN)
    message(FATAL_ERROR "error: not set BIN")
endif()
if(NOT EXISTS "${BIN}")
    message(FATAL_ERROR "error: ${BIN} not found")
endif()
if(NOT DSYM)
    set(DSYM "${BIN}.dSYM")
endif()

message(STATUS "BIN: ${BIN}")
message(STATUS "DSYM: ${DSYM}")

find_program(DSYMUTIL dsymutil REQUIRED)
find_program(DWARFDUMP dwarfdump REQUIRED)

file(REMOVE_RECURSE "${DSYM}")

# dsymutil warns and still exits 0 when the binary carries no debug map
execute_process(
    COMMAND ${DSYMUTIL} ${BIN} -o ${DSYM}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    RESULT_VARIABLE result
)

message("${output}${error}")

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate dSYM, code: ${result}")
endif()

if("${output}${error}" MATCHES "no debug symbols in executable")
    message(FATAL_ERROR "error: no debug map in ${BIN}\n"
                        "       the build has no debug info, or the binary is already stripped")
endif()

execute_process(COMMAND du -sh ${BIN} ${DSYM})
execute_process(COMMAND ${DWARFDUMP} --uuid ${BIN})
execute_process(COMMAND ${DWARFDUMP} --uuid ${DSYM})
