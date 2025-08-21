cmake_minimum_required(VERSION 3.15)
project(DistributedCachePP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force consistent MSVC runtime library across project + GoogleTest
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Enable all warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

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
# Force GoogleTest to use same runtime library
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# httplib (HTTP sever)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# JSON (header-only, skip its CMakeLists.txt to avoid warnings)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    # only add its headers, don’t add_subdirectory
    set(JSON_INCLUDE_DIR ${json_SOURCE_DIR}/single_include CACHE INTERNAL "")
endif()

# ------------- Library -----------------
add_library(cache
src/cache.cpp
src/api.cpp
 # add src/metrics.cpp when ready
)
target_include_directories(cache PRIVATE ${JSON_INCLUDE_DIR} include)
target_link_libraries(cache PRIVATE cache httplib::httplib)

# Add pthread only on UNIX
if(UNIX)
    target_link_libraries(DistributedCachePP PRIVATE pthread)
endif()

# ---------- Main Server ----------
add_executable(DistributedCachePP src/main.cpp)
target_link_libraries(DistributedCachePP PRIVATE cache)

# ---------- Tests ----------
enable_testing()
file(GLOB TEST_SOURCES tests/*.cpp)   # automatically pick up all test files
add_executable(UnitTests ${TEST_SOURCES})
target_link_libraries(UnitTests PRIVATE cache gtest_main)
add_test(NAME UnitTests COMMAND UnitTests)