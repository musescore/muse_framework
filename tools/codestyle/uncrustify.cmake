if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "This file is a script")
endif()

set(VERSION "074")

set(OS_PREFIX "")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(OS_PREFIX "windows")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(OS_PREFIX "linux")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")  # macOS
    set(OS_PREFIX "macos")
else()
    message(FATAL_ERROR "Unknown system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(UNCRUSTIFY_BIN ${CMAKE_CURRENT_LIST_DIR}/bin/${OS_PREFIX}/uncrustify_${VERSION})
if(NOT EXISTS ${UNCRUSTIFY_BIN})
    message(FATAL_ERROR "Failed to get uncrustify: ${UNCRUSTIFY_BIN}")
endif()

set(SCAN_BIN ${CMAKE_CURRENT_LIST_DIR}/bin/${OS_PREFIX}/scan_files)
if(NOT EXISTS ${SCAN_BIN})
    message(FATAL_ERROR "Failed to get scan_files: ${SCAN_BIN}")
endif()