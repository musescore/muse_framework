

function(check_compile_version name min_version hint_message)
    if (CMAKE_CXX_COMPILER_ID STREQUAL ${name})
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${min_version})
            message(FATAL_ERROR
                "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} is too old; "
                "requires ${min_version} or newer. ${hint_message}")
        endif()
    endif()
endfunction()