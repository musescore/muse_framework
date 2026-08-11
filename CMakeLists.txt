if (APPLE)
    cmake_minimum_required(VERSION 3.26...3.31) # Min. required for Swift-C++ interop
else()
    cmake_minimum_required(VERSION 3.22...3.31) # Min. required by Qt 6.10
endif()

project(muse_framework LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MUSE_FRAMEWORK_PATH ${CMAKE_CURRENT_LIST_DIR})
set(MUSE_FRAMEWORK_SRC_PATH ${MUSE_FRAMEWORK_PATH}/framework)

set(FETCHCONTENT_BASE_DIR ${PROJECT_BINARY_DIR}/_deps)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${MUSE_FRAMEWORK_PATH}/buildscripts
    ${MUSE_FRAMEWORK_PATH}/buildscripts/cmake
    ${CMAKE_MODULE_PATH}
    )

###########################################
# Check compiler version
###########################################
message(STATUS "C:   ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")    
message(STATUS "CXX: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

include(CheckCompilerVersion)
check_compile_version("Clang" 20 "https://github.com/musescore/MuseScore/wiki/Set-up-developer-environment")
check_compile_version("AppleClang" 21 "https://github.com/musescore/MuseScore/wiki/Set-up-developer-environment")
check_compile_version("GCC" 14 "https://github.com/musescore/MuseScore/wiki/Set-up-developer-environment")
check_compile_version("MSVC" 19.40 "https://github.com/musescore/MuseScore/wiki/Set-up-developer-environment")

###########################################
# Setup option and build settings
###########################################
include(GetPaths)

set(MUSE_APP_BUILD_MODE "dev" CACHE STRING "Build mode")
# Possible MUSE_APP_BUILD_MODE values:
# - dev     - for development/nightly builds
# - testing - for testing versions (alpha, beta, RC)
# - release - for stable release builds

include(${MUSE_FRAMEWORK_SRC_PATH}/cmake/MuseDeclareOptions.cmake)

###########################################
# Setup Configure
###########################################

# default
set(QT_ADD_LINGUISTTOOLS ON)
set(QT_ADD_CONCURRENT ON)
set(QT_ADD_WEBSOCKET OFF)
set(QT_QPROCESS_SUPPORTED ON)
set(QT_CONCURRENT_SUPPORTED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)
set(MUSE_MODULE_MULTIWINDOWS_SINGLEPROC_MODE ON)

set(MUSE_APP_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(MUSE_APP_INSTALL_RESOURCES_LOCATION ".")

add_compile_definitions(KORS_PROFILER_ENABLED)

include(${MUSE_FRAMEWORK_SRC_PATH}/cmake/MuseSetupConfiguration.cmake)

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

###########################################
# Setup compiler and build environment
###########################################

include(SetupBuildEnvironment)
include(GetPlatformInfo)

if (MUSE_COMPILE_USE_COMPILER_CACHE)
    include(SetupCompilerCache)
endif(MUSE_COMPILE_USE_COMPILER_CACHE)

###########################################
# Setup external dependencies
###########################################

include(SetupQt6)

include(SetupDependencies)

###########################################
# Add source tree
###########################################
if (MUSE_ENABLE_UNIT_TESTS)
    enable_testing()
    message(STATUS "Enabled testing")
endif()

add_subdirectory(${MUSE_FRAMEWORK_SRC_PATH})
