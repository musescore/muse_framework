include(CMakeDependentOption)

# === Enviropment ===
option(MUSE_QT_SUPPORT "Build with Qt support" ON)
option(MUSE_THREADS_SUPPORT "Build with threads support" ON)
option(MUSE_CONFIGURATION_IS_WEB "Configuration is web" OFF)

# === Build options ===
option(MUSE_COMPILE_ASAN "Enable Address Sanitizer" OFF)

# === Debug options ===
option(MUSE_COMPILE_STRING_DEBUG_HACK "Enable string debug hack (only clang)" ON)
option(MUSE_LOAD_QML_FROM_SOURCE "Load QML from source files instead of compiled resources (for development and debugging)" OFF)

# === Tests ===
option(MUSE_ENABLE_UNIT_TESTS_CODE_COVERAGE "Enable code coverage for unit tests" OFF)
option(MUSE_ENABLE_UNIT_TESTS "Build framework unit tests" ON)

# === Tools ===
option(MUSE_ENABLE_CUSTOM_ALLOCATOR "Enable custom allocator" OFF)

macro(declare_muse_module_opt name def)
    option(MUSE_MODULE_${name} "Build ${name} module" ${def})

    # 1. if we disable the module, the submodule values are irrelevant, as they will be ignored
    # 2. if we enable the module, all its submodules are enabled by default
    # 3. we can disable some submodules manually
    option(MUSE_MODULE_${name}_API "Build ${name} api" ON)
    option(MUSE_MODULE_${name}_QML "Build ${name} QML" ON)
    option(MUSE_MODULE_${name}_TESTS "Build ${name} tests" ${MUSE_ENABLE_UNIT_TESTS})
endmacro()

# Modules framework (alphabetical order please)
declare_muse_module_opt(ACCESSIBILITY ON)
option(MUSE_MODULE_ACCESSIBILITY_TRACE "Enable accessibility logging" OFF)

declare_muse_module_opt(ACTIONS ON)
option(MUSE_MODULE_ACTIONS_SUPPORT "Enable actions support" ON)

declare_muse_module_opt(AUDIO ON)
option(MUSE_MODULE_AUDIO_JACK "Enable jack support" OFF)

if (OS_IS_WIN)
    option(MUSE_MODULE_AUDIO_ASIO "Enable asio support" ON)
endif()

if (OS_IS_LIN OR OS_IS_FBSD)
    option(MUSE_MODULE_AUDIO_PIPEWIRE "Use PipeWire audio driver" OFF) # Turns ON on CI
endif()

option(MUSE_MODULE_AUDIO_EXPORT "Enable audio export" ON)

# 1 - worker - NOT SUPPORTED NOW
# 2 - driver callback
# 3 - hybrid mode - worker - RPC, driver callback - process
set(MUSE_MODULE_AUDIO_WORKMODE 3 CACHE STRING "Audio subsystem work mode")

declare_muse_module_opt(AUDIOPLUGINS ON)
option(MUSE_MODULE_AUDIOPLUGINS_SCAN_TRACE "Enable audio plugin scan logging" OFF)

declare_muse_module_opt(AUTOMATION ON)

declare_muse_module_opt(CLOUD ON)
option(MUSE_MODULE_CLOUD_MUSESCORECOM "Enable MuseScore.com account" ON)

declare_muse_module_opt(DIAGNOSTICS ON)
option(MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT "Enable crashpad client" OFF) # enable on CI
set(MUSE_MODULE_DIAGNOSTICS_CRASHPAD_HANDLER_PATH "" CACHE FILEPATH "Path to custom crashpad_handler executable (optional)")
set(MUSE_MODULE_DIAGNOSTICS_CRASHREPORT_URL "" CACHE STRING "URL where to send crash reports")

declare_muse_module_opt(DOCKWINDOW ON)
option(MUSE_MODULE_DOCKWINDOW_KDDOCKWIDGETS_V2 "Use KDDockWidgets v2" OFF)

declare_muse_module_opt(DRAW ON)
option(MUSE_MODULE_DRAW_TRACE "Trace draw objects" OFF)
option(MUSE_MODULE_DRAW_USE_QTFONTMETRICS "Use Qt font metrics (for some metrics)" ON)
option(MUSE_MODULE_DRAW_USE_QTTEXTDRAW "Use Qt text drawing path" ON)
option(MUSE_MODULE_DRAW_USE_STATIC_DEPS "Use static draw dependencies" OFF)
option(MUSE_MODULE_DRAW_USE_FONTFACE_FT "Use FreeType font face backend" ON)
option(MUSE_MODULE_DRAW_USE_FONTFACE_XT "Use XT font face backend" OFF)

declare_muse_module_opt(EXTENSIONS ON)

declare_muse_module_opt(GLOBAL ON)
option(MUSE_MODULE_GLOBAL_LOGGER_DEBUGLEVEL "Enable logging debug level" ON)
option(MUSE_MODULE_GLOBAL_MULTI_IOC "Enable multi ioc (multi windows)" OFF)

declare_muse_module_opt(INTERACTIVE ON)
option(MUSE_MODULE_INTERACTIVE_SYNC_SUPPORTED "Sync interactive supported" ON)

declare_muse_module_opt(LANGUAGES ON)
declare_muse_module_opt(LEARN ON)
declare_muse_module_opt(MEDIA ON)
declare_muse_module_opt(MIDI ON)
declare_muse_module_opt(MIDIREMOTE ON)
declare_muse_module_opt(MPE ON)

declare_muse_module_opt(MULTIWINDOWS ON)
option(MUSE_MODULE_MULTIWINDOWS_SINGLEPROC_MODE "Use single process mode for multi windows" OFF)

declare_muse_module_opt(MUSESAMPLER ON)
option(MUSE_MODULE_MUSESAMPLER_LOAD_IN_DEBUG "Load MuseSampler module in debug builds" OFF)

declare_muse_module_opt(NETWORK ON)
option(MUSE_MODULE_NETWORK_WEBSOCKET "Enable websocket support" OFF)

declare_muse_module_opt(RCOMMAND ON)
declare_muse_module_opt(RCONTROL ON)

declare_muse_module_opt(SHORTCUTS ON)
option(MUSE_MODULE_SHORTCUTS_V2 "Use shortcuts v2" OFF)

declare_muse_module_opt(TESTFLOW ON)
declare_muse_module_opt(TOAST ON)
declare_muse_module_opt(TOURS ON)

declare_muse_module_opt(UI ON)
option(MUSE_MODULE_UI_DISABLE_MODALITY "Disable dialogs modality for testing purpose" OFF)
option(MUSE_MODULE_UI_SYSTEMDRAG_SUPPORTED "System drag supported" ON)
option(MUSE_MODULE_UI_NAVIGATION_EXCLUDEPROJECT "Exclude the main project view from navigation" ON)

declare_muse_module_opt(UPDATE ON)

declare_muse_module_opt(VST OFF)

declare_muse_module_opt(WORKSPACE ON)
