# © 2026 Massachusetts Institute of Technology
# MIT License

cmake_minimum_required(VERSION 3.10)
project(fileops)
add_library(fileops SHARED
    src/fileops.c)

target_include_directories(fileops PUBLIC include)
target_include_directories(fileops PRIVATE src)

install(TARGETS fileops
    DESTINATION lib)
install(DIRECTORY include/
    DESTINATION include)
