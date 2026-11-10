cmake_minimum_required(VERSION 3.14)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_BENCHES "Build benchmark programs" OFF)
option(BUILD_EXAMPLES "Build example programs" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

# Append the "benches" feature to VCPKG_MANIFEST_FEATURES if building benchmarks
if(BUILD_BENCHES)
    list(APPEND VCPKG_MANIFEST_FEATURES "benches")
endif()

project(sregex
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# sregex library
add_subdirectory(src)

# Benchmarks
if(BUILD_BENCHES)
    add_subdirectory(benches)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(
    TARGETS sregex
    EXPORT sregexTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  FILES "src/sregex.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sregex
)

install(
    EXPORT sregexTargets
    FILE sregexTargets.cmake
    NAMESPACE sregex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sregex
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/sregexConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sregexConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/sregex"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sregexConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/sregexConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/sregexConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sregex
)