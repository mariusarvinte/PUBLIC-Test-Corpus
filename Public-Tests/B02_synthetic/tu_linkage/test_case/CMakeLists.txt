# © 2026 Massachusetts Institute of Technology
# MIT License

cmake_minimum_required(VERSION 3.10)
project(driver)
add_executable(driver src/main.c src/engine.c src/a.c src/b.c src/util.c src/lib.c)