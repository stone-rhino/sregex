cmake_minimum_required(VERSION 3.14)

project(sregex
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(SREGEX_BUILD_BENCHES "Build benchmark programs" ON)
option(SREGEX_BUILD_EXAMPLES "Build example programs" ON)
option(SREGEX_BUILD_TESTS "Build test programs" ON)

# sregex library
add_subdirectory(src)

# Benchmarks
if(SREGEX_BUILD_BENCHES)
    add_subdirectory(benches)
endif()

# Examples
if(SREGEX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(SREGEX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

