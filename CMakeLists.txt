cmake_minimum_required(VERSION 3.27)
project(gsmapp C)

set(CMAKE_C_STANDARD 11)

add_executable(gsmapp main.c
        stb_ds.h
        gsm.h
        gsm.c
        serial.h
        serial.c)
