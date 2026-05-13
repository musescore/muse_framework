
if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

set(SCAN_DIR ${CMAKE_ARGV3})

include(${CMAKE_CURRENT_LIST_DIR}/uncrustify.cmake)

set(CONFIG ${CMAKE_CURRENT_LIST_DIR}/uncrustify_muse.cfg)
set(UNTIDY_FILE ".untidy")

execute_process(
    COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/run_scan.sh ${SCAN_BIN} ${UNCRUSTIFY_BIN} ${CONFIG} ${UNTIDY_FILE} ${SCAN_DIR}
    RESULT_VARIABLE SCAN_RESULT
)

if (NOT SCAN_RESULT EQUAL 0)
    message(FATAL_ERROR "Scan failed, please check log for details")
endif()