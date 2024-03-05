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

include(CheckCCompilerFlag)

check_c_compiler_flag("-fsanitize=address" COMPILER_SUPPORTS_ASAN)
if(COMPILER_SUPPORTS_ASAN)
    message(STATUS "Compiler supports AddressSanitizer")
    target_compile_options(pshell PRIVATE -fsanitize=address)
    target_link_libraries(pshell PRIVATE -fsanitize=address)
else()
    message(WARNING "Compiler does not support AddressSanitizer")
endif()

check_c_compiler_flag("-fsanitize=undefined" COMPILER_SUPPORTS_UBSAN)
if(COMPILER_SUPPORTS_UBSAN)
    message(STATUS "Compiler supports UndefinedBehaviorSanitizer")
    target_compile_options(pshell PRIVATE -fsanitize=undefined)
    target_link_libraries(pshell PRIVATE -fsanitize=undefined)
else()
    message(WARNING "Compiler does not support UndefinedBehaviorSanitizer")
endif()