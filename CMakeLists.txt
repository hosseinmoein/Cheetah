cmake_minimum_required(VERSION 3.8)

if(POLICY CMP0090)
    cmake_policy(SET CMP0090 NEW)
endif()

project(TimerAlarm VERSION 1.0.0 LANGUAGES CXX)

option(HMTA_TESTING "Enable testing" OFF)
## option(HMTA_EXAMPLES "Build Examples" OFF)
## option(HMTA_BENCHMARKS "Build Benchmarks" OFF)

if(HMTA_TESTING)
    enable_testing()
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include(CheckSymbolExists)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

include(HMTATargetDataFiles)

# Build TimerAlarm library
check_symbol_exists(clock_gettime "time.h" HMTA_HAVE_CLOCK_GETTIME)

find_package(Threads REQUIRED)

# Installation

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/TimerAlarm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT dev
)

## pkgconfig file
if(UNIX AND NOT APPLE)
    set(TAADPOOL_PKG_EXTRA_LIBS " -lpthread -lrt")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TimerAlarm.pc.in
    TimerAlarm.pc
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/TimerAlarm.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

## CMake config files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TimerAlarmConfig.cmake.in
    TimerAlarmConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TimerAlarm
)

write_basic_package_version_file(
    TimerAlarmConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    EXPORT TimerAlarmExport
    NAMESPACE TimerAlarm::
    FILE TimerAlarmTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TimerAlarm
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/TimerAlarmConfig.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/TimerAlarmConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TimerAlarm
)

export(PACKAGE TimerAlarm)

# Tests
if(HMTA_TESTING)
    add_subdirectory(test)
endif()

