# © 2026 Massachusetts Institute of Technology
# MIT License

cmake_minimum_required(VERSION 3.10)
project(driver)
set(OP "add" CACHE STRING "operation leveraged")
set(REPEAT "5" CACHE STRING "iterations tested")
set(CMAKE_C_FLAGS "-DOP=${OP} -DREPEAT=${REPEAT}")
add_executable(driver
    src/mdcore.c
    src/mdmain.c)
