cmake_minimum_required(VERSION 3.12)
project(tar)

include_directories(src)

add_subdirectory(src)

add_executable(tar main.c)

target_link_libraries(tar
        gamux-tar)
