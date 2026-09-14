# © 2026 Massachusetts Institute of Technology
# MIT License

# util: a shared library for common utility functions for libgit2 projects

add_library(util OBJECT)

set(UTIL_INCLUDES
	"${PROJECT_BINARY_DIR}/src/util"
	"${PROJECT_BINARY_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/util"
	"${PROJECT_SOURCE_DIR}/include")

set(UTIL_SRC
	alloc.c
	alloc.h
	allocators/failalloc.c
	allocators/failalloc.h
	allocators/stdalloc.c
	allocators/stdalloc.h
	array.h
	assert_safe.h
	bitvec.h
	cc-compat.h
	ctype_compat.h
	date.c
	date.h
	errors.c
	errors.h
	filebuf.c
	filebuf.h
	fs_path.c
	fs_path.h
	futils.c
	futils.h
	git2_util.h
	hash.c
	hash.h
	hashmap.h
	hashmap_str.h
	integer.h
	map.h
	posix.c
	posix.h
	pqueue.c
	pqueue.h
	rand.c
	rand.h
	regexp.c
	regexp.h
	runtime.c
	runtime.h
	sortedcache.c
	sortedcache.h
	staticstr.h
	str.c
	str.h
	strlist.c
	strlist.h
	strnlen.h
	tsort.c
	utf8.c
	utf8.h
	util.c
	util.h
	varint.c
	varint.h
	vector.c
	vector.h
	wildmatch.c
	wildmatch.h
	zstream.c
	zstream.h)

#
# Platform specific sources
#
set(UTIL_SRC_OS
	unix/map.c
	unix/posix.h
	unix/realpath.c)

#
# Hash backend selection
#

set(UTIL_SRC_SHA1
	hash/collisiondetect.c
	hash/collisiondetect.h
	hash/sha1dc/sha1.c
	hash/sha1dc/sha1.h
	hash/sha1dc/ubc_check.c
	hash/sha1dc/ubc_check.h)
target_compile_definitions(util PRIVATE SHA1DC_NO_STANDARD_INCLUDES=1)
target_compile_definitions(util PRIVATE SHA1DC_CUSTOM_INCLUDE_SHA1_C=\"git2_util.h\")
target_compile_definitions(util PRIVATE SHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C=\"git2_util.h\")

set(UTIL_SRC_SHA256
	hash/builtin.c
	hash/builtin.h
	hash/rfc6234/sha.h
	hash/rfc6234/sha224-256.c)

#
# Build the library
#

target_sources(util PRIVATE ${UTIL_SRC} ${UTIL_SRC_OS} ${UTIL_SRC_SHA1} ${UTIL_SRC_SHA256})

target_include_directories(util PRIVATE ${UTIL_INCLUDES} ${LIBGIT2_DEPENDENCY_INCLUDES} PUBLIC ${libgit2_SOURCE_DIR}/include)
target_include_directories(util SYSTEM PRIVATE ${LIBGIT2_SYSTEM_INCLUDES})
