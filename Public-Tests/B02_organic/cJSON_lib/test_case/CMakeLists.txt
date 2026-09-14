# © 2026 Massachusetts Institute of Technology
# MIT License

cmake_minimum_required(VERSION 3.10)
project(cJSON)
add_library(cJSON SHARED src/cJSON.c src/test.c)

target_include_directories(cJSON PUBLIC include)
target_include_directories(cJSON PRIVATE src)

target_link_libraries(cJSON PRIVATE m)

install(TARGETS cJSON DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

