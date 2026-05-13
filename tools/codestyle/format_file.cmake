if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

set(FILE_PATH ${CMAKE_ARGV3})

include(${CMAKE_CURRENT_LIST_DIR}/uncrustify.cmake)

execute_process(
    COMMAND ${UNCRUSTIFY_BIN} -c ${CMAKE_CURRENT_LIST_DIR}/uncrustify_muse.cfg --no-backup -l CPP ${FILE_PATH}
    RESULT_VARIABLE UNCRUSTIFY_OUT
)

if (UNCRUSTIFY_OUT)
    message(FATAL_ERROR"Failed formatting file: ${FILE_PATH}, error code: ${UNCRUSTIFY_OUT}")
endif()
