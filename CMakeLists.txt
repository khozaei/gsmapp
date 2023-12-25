cmake_minimum_required(VERSION 3.27)
project(gsmapp C)

set(CMAKE_C_STANDARD 11)

include(AddGitSubmodule.cmake)
option(BUILD_TESTING "Build without tests" OFF)
add_git_submodule(3rdparty/libuv)

include_directories(3rdparty/CLIST 3rdparty/libuv/include)

add_executable(gsmapp main.c
        gsm.h
        gsm.c
        serial.h
        serial.c
        3rdparty/CLIST/clist.c
        3rdparty/CLIST/clist.h)

target_link_libraries(${PROJECT_NAME} uv_a)
