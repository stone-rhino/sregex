cmake_minimum_required(VERSION 3.14)

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
  DESTINATION lib
)

install(
  DIRECTORY src/
  DESTINATION include/sregex
  FILES_MATCHING PATTERN "*.h"
)