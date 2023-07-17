cmake_minimum_required(VERSION 3.1)
project(CUT C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_C_FLAGS "-Weverything -Wno-padded -DNDEBUG")
#set(CMAKE_C_COMPILER "/usr/bin/gcc")
#set(CMAKE_C_FLAGS "-Wall -Wextra -DNDEBUG")

add_executable(CUT main.c)
