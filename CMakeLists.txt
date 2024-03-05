cmake_minimum_required(VERSION 3.22)
project(pshell C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(pshell
        shell.c
        shell.h
        cmd_parser.c)

target_compile_options(pshell
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

set(SANITIZE_FLAGS "-fsanitize=address,undefined")

include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("${SANITIZE_FLAGS}" COMPILER_SUPPORTS_SANITIZERS)
if(COMPILER_SUPPORTS_SANITIZERS)
    message(STATUS "Compiler supports sanitizers")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
else()
    message(WARNING "Compiler does not support sanitizers")
endif()