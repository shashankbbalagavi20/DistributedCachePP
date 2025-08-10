cmake_minimum_required(VERSION 3.15)
project(DistributedCachePP VERSION 1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include headers
include_directories(include)

# Source files
file(GLOB SOURCES src/*.cpp)

# Create executable
add_executable(DistributedCachePP ${SOURCES})

# Link libraries (future: httplib, json, prometheus, etc.)
# target_link_libraries(DistributedCachePP ...)