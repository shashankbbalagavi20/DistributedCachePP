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

# httplib (HTTP server)
include(FetchContent)
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
    set(JSON_INCLUDE_DIR ${json_SOURCE_DIR}/single_include CACHE INTERNAL "")
endif()

# ---------------- Library ----------------
add_library(DistributedCacheLib src/cache.cpp src/replication.cpp)
target_include_directories(DistributedCacheLib
 PUBLIC
  include
  ${JSON_INCLUDE_DIR}   # in case replication uses JSON later
  ${httplib_SOURCE_DIR}
)
# ---------------- Main Server Binary ----------------
add_executable(DistributedCachePP src/main.cpp src/api.cpp)
target_include_directories(DistributedCachePP PRIVATE ${JSON_INCLUDE_DIR} include)
target_link_libraries(DistributedCachePP PUBLIC DistributedCacheLib httplib::httplib)

if(UNIX)
    target_link_libraries(DistributedCachePP PRIVATE pthread)
endif()

# ---------------- Tests (optional) ----------------
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    include(CTest)
    enable_testing()

    # Fetch GoogleTest only if tests are enabled
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Cache unit tests
    add_executable(CacheTests tests/cache_test.cpp)
    target_link_libraries(CacheTests PRIVATE DistributedCacheLib gtest_main)
    add_test(NAME CacheTests COMMAND CacheTests)

    # API integration tests
    add_executable(ApiTests tests/api_test.cpp src/api.cpp)
    target_include_directories(ApiTests PRIVATE ${JSON_INCLUDE_DIR} include)
    target_link_libraries(ApiTests PRIVATE DistributedCacheLib httplib::httplib gtest_main)
    if(UNIX)
        target_link_libraries(ApiTests PRIVATE pthread)
    endif()
    add_test(NAME ApiTests COMMAND ApiTests)
endif()