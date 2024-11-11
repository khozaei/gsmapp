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

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(PROJECT_VERSION ${GIT_VERSION})
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)
message(${PROJECT_VERSION})

add_executable(gsmapp main.c
        gsm.h
        gsm.c
        serial.h
        serial.c
        buffer.c
        buffer.h
        smartpointer.c
        smartpointer.h
        version.h
)

target_link_libraries(${PROJECT_NAME} uv_a)
target_link_libraries(gsmapp ${GLIB2_LIBRARIES})
