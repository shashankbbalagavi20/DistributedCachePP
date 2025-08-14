cmake_minimum_required(VERSION 3.14)
project(DistributedCachePP)

set(CMAKE_CXX_STANDARD 17)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Library
add_library(cache src/cache.cpp include/cache.h)
target_include_directories(cache PUBLIC include)

# Tests
enable_testing()
add_executable(CacheTests tests/cache_test.cpp)
target_link_libraries(CacheTests PRIVATE cache gtest_main)

include(GoogleTest)
add_test(NAME CacheTests COMMAND CacheTests)