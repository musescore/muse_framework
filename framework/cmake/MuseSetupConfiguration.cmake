
# hard dependencies
if (NOT MUSE_MODULE_AUDIO)
    set(MUSE_MODULE_MUSESAMPLER OFF)
endif()

if (NOT MUSE_MODULE_UI)
    set(MUSE_MODULE_TOAST OFF) # Hard dependency
endif()

include(${CMAKE_CURRENT_LIST_DIR}/MuseModules.cmake)

# Disable sub-options of disabled modules
foreach(NAME ${MUSE_FRAMEWORK_MODULES})
    if (NOT MUSE_MODULE_${NAME})
        set(MUSE_MODULE_${NAME}_API OFF)
        # set(MUSE_MODULE_${NAME}_QML OFF) -- Most stubs have QML
        set(MUSE_MODULE_${NAME}_TESTS OFF)
    endif()
endforeach()

if (NOT MUSE_ENABLE_UNIT_TESTS)
    # disable unit tests
    foreach(NAME ${MUSE_FRAMEWORK_MODULES})
        set(MUSE_MODULE_${NAME}_TESTS OFF)
    endforeach()
endif()

if (MUSE_MODULE_DRAW_TESTS)
    set(MUSE_MODULE_DRAW_USE_FONTFACE_FT ON)
    set(MUSE_MODULE_DRAW_USE_FONTFACE_XT ON)
endif()

if (NOT MUSE_MODULE_DIAGNOSTICS)
    set(MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT OFF)
endif()

if (NOT MUSE_MODULE_UI)
    set(MUSE_MODULE_UI_QML OFF) # Does not have stub that has QML
    set(MUSE_MODULE_INTERACTIVE OFF) # Hard dependency
endif()

if (NOT MUSE_MODULE_UI_QML)
    # Disable QML modules
    foreach(NAME ${MUSE_FRAMEWORK_MODULES})
        set(MUSE_MODULE_${NAME}_QML OFF)
    endforeach()
endif()

if (NOT MUSE_MODULE_TESTFLOW)
    set(MUSE_MODULE_TESTFLOW_QML OFF) # Does not have stub that has QML
endif()

if (NOT MUSE_MODULE_DIAGNOSTICS)
    set(MUSE_MODULE_DIAGNOSTICS_QML OFF) # Does not have stub that has QML
endif()

if (NOT MUSE_MODULE_MULTIWINDOWS)
    set(MUSE_MODULE_MULTIWINDOWS_QML OFF) # Stub does not have QML
endif()

if (NOT MUSE_MODULE_VST)
    set(MUSE_MODULE_VST_QML OFF) # Stub does not have QML
endif()

if (MUSE_QT_SUPPORT)
    add_compile_definitions(KORS_LOGGER_QT_SUPPORT)
else()
    set(GLOBAL_NO_INTERNAL ON)
    set(MUSE_MODULE_DRAW_USE_QTFONTMETRICS OFF)
    set(MUSE_MODULE_DRAW_USE_QTTEXTDRAW OFF)
    add_compile_definitions(NO_QT_SUPPORT)
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/muse_framework_config.h.in muse_framework_config.h )

include(${CMAKE_CURRENT_LIST_DIR}/MuseCreateModule.cmake)
