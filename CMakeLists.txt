cmake_minimum_required(VERSION 3.10)
project(DistributedCachePP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

# Source for production executable
add_executable(DistributedCachePP
    src/cache.cpp
    src/main.cpp
)

# Debug/demo build
add_executable(DistributedCachePP_Demo
    src/cache.cpp
    tests/demo.cpp
)