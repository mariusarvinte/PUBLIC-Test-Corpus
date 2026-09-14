# © 2026 Massachusetts Institute of Technology
# MIT License

# This top-level CMakeLists.txt sets up configuration options and
# determines which subprojects to build.

cmake_minimum_required(VERSION 3.10)

project(libgit2 VERSION "1.9.0" LANGUAGES C)

#
# Build options
#

# Feature enablement and backend selection
set(USE_COMPRESSION         "" CACHE STRING "Selects compression backend. Either zlib or builtin (Defaults to builtin).")
option(USE_NSEC             "Enable nanosecond precision timestamps." ON)
option(EXPERIMENTAL_SHA256  "Enable experimental SHA256 support (for R&D/testing)"  OFF)
option(DEPRECATE_HARD       "Do not include deprecated functions in the library"    OFF)


# Add defines for feature selection
if(USE_COMPRESSION STREQUAL "zlib")
	add_definitions(-DGIT_COMPRESSION_ZLIB)
	list(APPEND LIBGIT2_SYSTEM_LIBS z)
	list(APPEND LIBGIT2_PC_REQUIRES "zlib")
else()
	add_definitions(-DGIT_COMPRESSION_BUILTIN)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib" "${CMAKE_CURRENT_BINARY_DIR}/deps/zlib")
	list(APPEND LIBGIT2_DEPENDENCY_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib")
	list(APPEND LIBGIT2_DEPENDENCY_OBJECTS $<TARGET_OBJECTS:zlib>)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/xdiff" "${CMAKE_CURRENT_BINARY_DIR}/deps/xdiff")
list(APPEND LIBGIT2_DEPENDENCY_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/deps/xdiff")
list(APPEND LIBGIT2_DEPENDENCY_OBJECTS "$<TARGET_OBJECTS:xdiff>")

add_definitions(-DGIT_REGEX_PCRE2)
list(APPEND LIBGIT2_SYSTEM_LIBS pcre2-8)
list(APPEND LIBGIT2_PC_REQUIRES "libpcre2-8")

add_definitions(-DGIT_SHA1_BUILTIN)
add_definitions(-DGIT_SHA256_BUILTIN)
if(EXPERIMENTAL_SHA256)
	add_definitions(-DGIT_EXPERIMENTAL_SHA256)
endif()
if (USE_NSEC)
	add_definitions(-DGIT_NSEC)
endif()
if(DEPRECATE_HARD)
        add_definitions(-DGIT_DEPRECATE_HARD)
endif()


# Compilation options
set(CMAKE_C_STANDARD        "90" CACHE STRING "The C standard to compile against")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -O0")
endif()

#
# Subdirectories
#

add_subdirectory(src)
