cmake_minimum_required(VERSION 3.27)
project(zadanie1 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_compile_options(-Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC)

add_link_options(-Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup)

add_executable(zadanie1
        memory_tests.c
        memory_tests.h
        nand.c
        nand.h
        nand_example.c)
