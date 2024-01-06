cmake_minimum_required(VERSION 3.27)
project(gsmapp C)

set(CMAKE_C_STANDARD 11)

include(AddGitSubmodule.cmake)
option(BUILD_TESTING "Build without tests" OFF)
add_git_submodule(3rdparty/libuv)

include_directories(3rdparty/libuv/include)

include(FindPkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})


add_executable(gsmapp main.c
        gsm.h
        gsm.c
        serial.h
        serial.c
        buffer.c
        buffer.h
)

target_link_libraries(${PROJECT_NAME} uv_a)
target_link_libraries(gsmapp ${GLIB2_LIBRARIES})
