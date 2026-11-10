cmake_minimum_required(VERSION 3.16)
project(ArenaTactics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/raylib)

add_executable(ArenaTactics src/main.cpp)
target_link_libraries(ArenaTactics PRIVATE raylib)
