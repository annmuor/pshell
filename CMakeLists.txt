cmake_minimum_required(VERSION 3.25)
project(pshell C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(pshell
        shell.c
        shell.h
        cmd_parser.c)
