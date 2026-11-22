cmake_minimum_required(VERSION 3.16)
project(ArenaTactics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/raylib)

add_executable(ArenaTactics src/main.cpp)
target_link_libraries(ArenaTactics PRIVATE raylib)

# copy assets (fonts etc) next to the exe after each build
add_custom_command(TARGET ArenaTactics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ArenaTactics>/assets)
