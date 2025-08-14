cmake_minimum_required(VERSION 3.14)
project(DistributedCachePP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source and include directories
include_directories(include)

# Library directories
add_library(cache src/cache.cpp)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Tests
enable_testing()
add_executable(CacheTests tests/cache_test.cpp)
target_link_libraries(CacheTests PRIVATE cache gtest_main)

add_test(NAME CacheTests COMMAND CacheTests)