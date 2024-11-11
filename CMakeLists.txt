cmake_minimum_required(VERSION 3.27)

project(gsmapp
        LANGUAGES C)

set(CMAKE_C_STANDARD 11)

option(BUILD_TESTING "Build without tests" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(AddGitSubmodule)
include(GitVersion)
add_git_submodule(3rdparty/libuv)

include_directories(3rdparty/libuv/include)

include(FindPkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})

get_version_from_git()

configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)
message(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")

add_executable(gsmapp main.c
        gsm.h
        gsm.c
        serial.h
        serial.c
        buffer.c
        buffer.h
        smartpointer.c
        smartpointer.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries(${PROJECT_NAME} uv_a)
target_link_libraries(${PROJECT_NAME} ${GLIB2_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
