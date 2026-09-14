# © 2026 Massachusetts Institute of Technology
# MIT License

# tests: the unit and integration tests for libgit2

cmake_minimum_required(VERSION 3.10)
project(tests LANGUAGES C)

if(NOT "${CMAKE_VERSION}" VERSION_LESS 3.27)
	cmake_policy(SET CMP0148 OLD)
endif()

set(Python_ADDITIONAL_VERSIONS 3 2.7)
find_package(PythonInterp)

if(NOT PYTHONINTERP_FOUND)
	message(FATAL_ERROR "Could not find a python interpreter, which is needed to build the tests. "
	                     "Make sure python is available, or pass -DBUILD_TESTS=OFF to skip building the tests")
ENDIF()

# Clar specific setup
set(CLAR_PATH "${PROJECT_SOURCE_DIR}/clar")
set(CLAR_FIXTURES "${PROJECT_SOURCE_DIR}/resources/")
set(TEST_PATH "${PROJECT_SOURCE_DIR}/libgit2")
set(SRC_CLAR
	clar/clar.c
	clar/clar.h
	clar/clar/fixtures.h
	clar/clar/fs.h
	clar/clar/print.h
	clar/clar/sandbox.h
	clar/clar/summary.h
	clar/clar_libgit2.c
	clar/clar_libgit2.h
	clar/clar_libgit2_alloc.c
	clar/clar_libgit2_alloc.h
	clar/clar_libgit2_timer.c
	clar/clar_libgit2_timer.h
	clar/clar_libgit2_trace.c
	clar/clar_libgit2_trace.h
	clar/main.c)
add_definitions(-DCLAR_FIXTURE_PATH=\"${CLAR_FIXTURES}\")
add_definitions(-DCLAR_TMPDIR=\"tests\")
add_definitions(-DCLAR_HAS_REALPATH)
add_definitions(-D_FILE_OFFSET_BITS=64)

# Generic project specific setup
set(SRC_TEST
	libgit2/apply/apply_helpers.c
	libgit2/apply/apply_helpers.h
	libgit2/apply/both.c
	libgit2/apply/callbacks.c
	libgit2/apply/check.c
	libgit2/apply/fromdiff.c
	libgit2/apply/fromfile.c
	libgit2/apply/index.c
	libgit2/apply/partial.c
	libgit2/apply/tree.c
	libgit2/apply/workdir.c
	libgit2/attr/attr_expect.h
	libgit2/attr/file.c
	libgit2/attr/flags.c
	libgit2/attr/lookup.c
	libgit2/attr/macro.c
	libgit2/attr/repo.c
	libgit2/blame/blame_helpers.c
	libgit2/blame/blame_helpers.h
	libgit2/blame/buffer.c
	libgit2/blame/getters.c
	libgit2/blame/harder.c
	libgit2/blame/simple.c
	libgit2/checkout/binaryunicode.c
	libgit2/checkout/checkout_helpers.c
	libgit2/checkout/checkout_helpers.h
	libgit2/checkout/conflict.c
	libgit2/checkout/crlf.c
	libgit2/checkout/head.c
	libgit2/checkout/index.c
	libgit2/checkout/nasty.c
	libgit2/checkout/tree.c
	libgit2/checkout/typechange.c
	libgit2/cherrypick/bare.c
	libgit2/cherrypick/workdir.c
	libgit2/clone/empty.c
	libgit2/clone/local.c
	libgit2/clone/nonetwork.c
	libgit2/clone/transport.c
	libgit2/commit/commit.c
	libgit2/commit/create.c
	libgit2/commit/parent.c
	libgit2/commit/parse.c
	libgit2/commit/signature.c
	libgit2/commit/write.c
	libgit2/config/add.c
	libgit2/config/backend.c
	libgit2/config/conditionals.c
	libgit2/config/config_helpers.c
	libgit2/config/config_helpers.h
	libgit2/config/configlevel.c
	libgit2/config/find.c
	libgit2/config/global.c
	libgit2/config/include.c
	libgit2/config/memory.c
	libgit2/config/multivar.c
	libgit2/config/new.c
	libgit2/config/read.c
	libgit2/config/readonly.c
	libgit2/config/rename.c
	libgit2/config/snapshot.c
	libgit2/config/validkeyname.c
	libgit2/config/write.c
	libgit2/core/buf.c
	libgit2/core/env.c
	libgit2/core/features.c
	libgit2/core/hashsig.c
	libgit2/core/oid.c
	libgit2/core/oidarray.c
	libgit2/core/opts.c
	libgit2/core/structinit.c
	libgit2/core/version.c
	libgit2/date/date.c
	libgit2/date/rfc2822.c
	libgit2/delta/apply.c
	libgit2/describe/describe.c
	libgit2/describe/describe_helpers.c
	libgit2/describe/describe_helpers.h
	libgit2/describe/t6120.c
	libgit2/diff/binary.c
	libgit2/diff/blob.c
	libgit2/diff/diff_helpers.c
	libgit2/diff/diff_helpers.h
	libgit2/diff/diffiter.c
	libgit2/diff/drivers.c
	libgit2/diff/externalmodifications.c
	libgit2/diff/format_email.c
	libgit2/diff/header.c
	libgit2/diff/index.c
	libgit2/diff/notify.c
	libgit2/diff/parse.c
	libgit2/diff/patch.c
	libgit2/diff/patchid.c
	libgit2/diff/pathspec.c
	libgit2/diff/racediffiter.c
	libgit2/diff/rename.c
	libgit2/diff/stats.c
	libgit2/diff/submodules.c
	libgit2/diff/tree.c
	libgit2/diff/userdiff.c
	libgit2/diff/workdir.c
	libgit2/email/create.c
	libgit2/fetch/local.c
	libgit2/fetchhead/fetchhead_data.h
	libgit2/fetchhead/nonetwork.c
	libgit2/filter/bare.c
	libgit2/filter/blob.c
	libgit2/filter/crlf.c
	libgit2/filter/crlf.h
	libgit2/filter/custom.c
	libgit2/filter/custom_helpers.c
	libgit2/filter/custom_helpers.h
	libgit2/filter/file.c
	libgit2/filter/ident.c
	libgit2/filter/query.c
	libgit2/filter/stream.c
	libgit2/filter/systemattrs.c
	libgit2/filter/wildcard.c
	libgit2/grafts/basic.c
	libgit2/grafts/parse.c
	libgit2/grafts/shallow.c
	libgit2/graph/ahead_behind.c
	libgit2/graph/commitgraph.c
	libgit2/graph/descendant_of.c
	libgit2/graph/reachable_from_any.c
	libgit2/ignore/path.c
	libgit2/ignore/status.c
	libgit2/index/add.c
	libgit2/index/addall.c
	libgit2/index/bypath.c
	libgit2/index/cache.c
	libgit2/index/collision.c
	libgit2/index/conflicts.c
	libgit2/index/conflicts.h
	libgit2/index/crlf.c
	libgit2/index/filemodes.c
	libgit2/index/inmemory.c
	libgit2/index/names.c
	libgit2/index/nsec.c
	libgit2/index/racy.c
	libgit2/index/read_index.c
	libgit2/index/read_tree.c
	libgit2/index/rename.c
	libgit2/index/reuc.c
	libgit2/index/splitindex.c
	libgit2/index/stage.c
	libgit2/index/tests.c
	libgit2/index/tests256.c
	libgit2/index/version.c
	libgit2/iterator/index.c
	libgit2/iterator/iterator_helpers.c
	libgit2/iterator/iterator_helpers.h
	libgit2/iterator/tree.c
	libgit2/iterator/workdir.c
	libgit2/mailmap/basic.c
	libgit2/mailmap/blame.c
	libgit2/mailmap/mailmap_testdata.h
	libgit2/mailmap/parsing.c
	libgit2/merge/analysis.c
	libgit2/merge/annotated_commit.c
	libgit2/merge/conflict_data.h
	libgit2/merge/driver.c
	libgit2/merge/files.c
	libgit2/merge/merge_helpers.c
	libgit2/merge/merge_helpers.h
	libgit2/merge/trees/automerge.c
	libgit2/merge/trees/commits.c
	libgit2/merge/trees/modeconflict.c
	libgit2/merge/trees/recursive.c
	libgit2/merge/trees/renames.c
	libgit2/merge/trees/treediff.c
	libgit2/merge/trees/trivial.c
	libgit2/merge/trees/whitespace.c
	libgit2/merge/workdir/dirty.c
	libgit2/merge/workdir/recursive.c
	libgit2/merge/workdir/renames.c
	libgit2/merge/workdir/setup.c
	libgit2/merge/workdir/simple.c
	libgit2/merge/workdir/submodules.c
	libgit2/merge/workdir/trivial.c
	libgit2/message/trailer.c
	libgit2/network/fetchlocal.c
	libgit2/network/refspecs.c
	libgit2/network/remote/defaultbranch.c
	libgit2/network/remote/delete.c
	libgit2/network/remote/isvalidname.c
	libgit2/network/remote/local.c
	libgit2/network/remote/push.c
	libgit2/network/remote/remotes.c
	libgit2/network/remote/rename.c
	libgit2/network/remote/tag.c
	libgit2/notes/notes.c
	libgit2/notes/notesref.c
	libgit2/object/blob/filter.c
	libgit2/object/blob/fromstream.c
	libgit2/object/blob/write.c
	libgit2/object/cache.c
	libgit2/object/commit/commitstagedfile.c
	libgit2/object/commit/parse.c
	libgit2/object/lookup.c
	libgit2/object/lookupbypath.c
	libgit2/object/lookup256.c
	libgit2/object/message.c
	libgit2/object/peel.c
	libgit2/object/raw/chars.c
	libgit2/object/raw/compare.c
	libgit2/object/raw/convert.c
	libgit2/object/raw/data.h
	libgit2/object/raw/fromstr.c
	libgit2/object/raw/hash.c
	libgit2/object/raw/short.c
	libgit2/object/raw/size.c
	libgit2/object/raw/type2string.c
	libgit2/object/raw/write.c
	libgit2/object/shortid.c
	libgit2/object/tag/list.c
	libgit2/object/tag/parse.c
	libgit2/object/tag/peel.c
	libgit2/object/tag/read.c
	libgit2/object/tag/write.c
	libgit2/object/tree/attributes.c
	libgit2/object/tree/duplicateentries.c
	libgit2/object/tree/frompath.c
	libgit2/object/tree/parse.c
	libgit2/object/tree/read.c
	libgit2/object/tree/update.c
	libgit2/object/tree/walk.c
	libgit2/object/tree/write.c
	libgit2/object/validate.c
	libgit2/odb/alternates.c
	libgit2/odb/backend/backend_helpers.c
	libgit2/odb/backend/backend_helpers.h
	libgit2/odb/backend/loose.c
	libgit2/odb/backend/mempack.c
	libgit2/odb/backend/multiple.c
	libgit2/odb/backend/nobackend.c
	libgit2/odb/backend/nonrefreshing.c
	libgit2/odb/backend/refreshing.c
	libgit2/odb/backend/simple.c
	libgit2/odb/emptyobjects.c
	libgit2/odb/foreach.c
	libgit2/odb/freshen.c
	libgit2/odb/loose.c
	libgit2/odb/loose_data.h
	libgit2/odb/mixed.c
	libgit2/odb/open.c
	libgit2/odb/pack_data.h
	libgit2/odb/pack_data_256.h
	libgit2/odb/pack_data_one.h
	libgit2/odb/pack_data_one256.h
	libgit2/odb/packed.c
	libgit2/odb/packed256.c
	libgit2/odb/packedone.c
	libgit2/odb/packedone256.c
	libgit2/odb/sorting.c
	libgit2/odb/streamwrite.c
	libgit2/pack/filelimit.c
	libgit2/pack/indexer.c
	libgit2/pack/midx.c
	libgit2/pack/packbuilder.c
	libgit2/pack/sharing.c
	libgit2/patch/parse.c
	libgit2/patch/patch_common.h
	libgit2/patch/print.c
	libgit2/path/validate.c
	libgit2/rebase/abort.c
	libgit2/rebase/inmemory.c
	libgit2/rebase/iterator.c
	libgit2/rebase/merge.c
	libgit2/rebase/setup.c
	libgit2/rebase/sign.c
	libgit2/rebase/submodule.c
	libgit2/refs/basic.c
	libgit2/refs/branches/checkedout.c
	libgit2/refs/branches/create.c
	libgit2/refs/branches/delete.c
	libgit2/refs/branches/ishead.c
	libgit2/refs/branches/iterator.c
	libgit2/refs/branches/lookup.c
	libgit2/refs/branches/move.c
	libgit2/refs/branches/name.c
	libgit2/refs/branches/remote.c
	libgit2/refs/branches/upstream.c
	libgit2/refs/branches/upstreamname.c
	libgit2/refs/cmp.c
	libgit2/refs/crashes.c
	libgit2/refs/create.c
	libgit2/refs/delete.c
	libgit2/refs/dup.c
	libgit2/refs/foreachglob.c
	libgit2/refs/isvalidname.c
	libgit2/refs/iterator.c
	libgit2/refs/list.c
	libgit2/refs/listall.c
	libgit2/refs/lookup.c
	libgit2/refs/namespaces.c
	libgit2/refs/normalize.c
	libgit2/refs/overwrite.c
	libgit2/refs/pack.c
	libgit2/refs/peel.c
	libgit2/refs/races.c
	libgit2/refs/read.c
	libgit2/refs/ref_helpers.c
	libgit2/refs/ref_helpers.h
	libgit2/refs/reflog/drop.c
	libgit2/refs/reflog/messages.c
	libgit2/refs/reflog/reflog.c
	libgit2/refs/reflog/reflog_helpers.c
	libgit2/refs/reflog/reflog_helpers.h
	libgit2/refs/rename.c
	libgit2/refs/revparse.c
	libgit2/refs/setter.c
	libgit2/refs/shorthand.c
	libgit2/refs/tags/name.c
	libgit2/refs/transactions.c
	libgit2/refs/unicode.c
	libgit2/refs/update.c
	libgit2/remote/create.c
	libgit2/remote/fetch.c
	libgit2/remote/insteadof.c
	libgit2/remote/list.c
	libgit2/repo/config.c
	libgit2/repo/discover.c
	libgit2/repo/env.c
	libgit2/repo/extensions.c
	libgit2/repo/getters.c
	libgit2/repo/hashfile.c
	libgit2/repo/head.c
	libgit2/repo/headtree.c
	libgit2/repo/init.c
	libgit2/repo/message.c
	libgit2/repo/new.c
	libgit2/repo/objectformat.c
	libgit2/repo/open.c
	libgit2/repo/pathspec.c
	libgit2/repo/repo_helpers.c
	libgit2/repo/repo_helpers.h
	libgit2/repo/reservedname.c
	libgit2/repo/setters.c
	libgit2/repo/shallow.c
	libgit2/repo/state.c
	libgit2/repo/template.c
	libgit2/reset/default.c
	libgit2/reset/hard.c
	libgit2/reset/mixed.c
	libgit2/reset/reset_helpers.c
	libgit2/reset/reset_helpers.h
	libgit2/reset/soft.c
	libgit2/revert/bare.c
	libgit2/revert/rename.c
	libgit2/revert/workdir.c
	libgit2/revwalk/basic.c
	libgit2/revwalk/hidecb.c
	libgit2/revwalk/mergebase.c
	libgit2/revwalk/signatureparsing.c
	libgit2/revwalk/simplify.c
	libgit2/stash/apply.c
	libgit2/stash/drop.c
	libgit2/stash/foreach.c
	libgit2/stash/save.c
	libgit2/stash/stash_helpers.c
	libgit2/stash/stash_helpers.h
	libgit2/stash/submodules.c
	libgit2/status/renames.c
	libgit2/status/single.c
	libgit2/status/status_data.h
	libgit2/status/status_helpers.c
	libgit2/status/status_helpers.h
	libgit2/status/submodules.c
	libgit2/status/worktree.c
	libgit2/status/worktree_init.c
	libgit2/submodule/add.c
	libgit2/submodule/escape.c
	libgit2/submodule/init.c
	libgit2/submodule/inject_option.c
	libgit2/submodule/lookup.c
	libgit2/submodule/modify.c
	libgit2/submodule/nosubs.c
	libgit2/submodule/open.c
	libgit2/submodule/repository_init.c
	libgit2/submodule/status.c
	libgit2/submodule/submodule_helpers.c
	libgit2/submodule/submodule_helpers.h
	libgit2/submodule/update.c
	libgit2/util/alloc.c
	libgit2/util/array.c
	libgit2/util/assert.c
	libgit2/util/bitvec.c
	libgit2/util/copy.c
	libgit2/util/crlf.h
	libgit2/util/dirent.c
	libgit2/util/encoding.c
	libgit2/util/errors.c
	libgit2/util/filebuf.c
	libgit2/util/futils.c
	libgit2/util/gitstr.c
	libgit2/util/hashmap.c
	libgit2/util/hex.c
	libgit2/util/init.c
	libgit2/util/integer.c
	libgit2/util/link.c
	libgit2/util/memmem.c
	libgit2/util/mkdir.c
	libgit2/util/path/core.c
	libgit2/util/posix.c
	libgit2/util/pqueue.c
	libgit2/util/precompiled.c
	libgit2/util/precompiled.h
	libgit2/util/qsort.c
	libgit2/util/regexp.c
	libgit2/util/rmdir.c
	libgit2/util/sha1.c
	libgit2/util/sha256.c
	libgit2/util/sortedcache.c
	libgit2/util/stat.c
	libgit2/util/str/basic.c
	libgit2/util/str/oom.c
	libgit2/util/str/percent.c
	libgit2/util/str/quote.c
	libgit2/util/str/splice.c
	libgit2/util/string.c
	libgit2/util/strtol.c
	libgit2/util/utf8.c
	libgit2/util/vector.c
	libgit2/util/wildmatch.c
	libgit2/util/zstream.c
	libgit2/worktree/bare.c
	libgit2/worktree/config.c
	libgit2/worktree/merge.c
	libgit2/worktree/open.c
	libgit2/worktree/reflog.c
	libgit2/worktree/refs.c
	libgit2/worktree/repository.c
	libgit2/worktree/submodule.c
	libgit2/worktree/worktree.c
	libgit2/worktree/worktree_helpers.c
	libgit2/worktree/worktree_helpers.h)
set(SRC_INCLUDES "")
set(PROJECT_INCLUDES
	"${CMAKE_CURRENT_SOURCE_DIR}/../test_case/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../test_case/src"
)
set(TEST_INCLUDES "${CLAR_PATH}" "${TEST_PATH}" "${CMAKE_CURRENT_BINARY_DIR}")
set(DEPENDENCY_INCLUDES "")
set(SYSTEM_INCLUDES "")
set(LIBS "")

# ============================== Libgit2 Specific ================================================
set(USE_COMPRESSION         "" CACHE STRING "Selects compression backend. Either zlib or builtin (Defaults to builtin).")
option(USE_NSEC             "Enable nanosecond precision timestamps." ON)
option(EXPERIMENTAL_SHA256  "Enable experimental SHA256 support (for R&D/testing)"  OFF)
option(DEPRECATE_HARD       "Do not include deprecated functions in the library"    OFF)

if(USE_COMPRESSION STREQUAL "zlib")
	add_definitions(-DGIT_COMPRESSION_ZLIB)
	list(APPEND LIBGIT2_SYSTEM_LIBS z)
	list(APPEND LIBGIT2_PC_REQUIRES "zlib")
else()
	add_definitions(-DGIT_COMPRESSION_BUILTIN)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../test_case/deps/zlib" "${CMAKE_CURRENT_BINARY_DIR}/deps/zlib")
	list(APPEND LIBGIT2_DEPENDENCY_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../test_case/deps/zlib")
	list(APPEND LIBGIT2_DEPENDENCY_OBJECTS $<TARGET_OBJECTS:zlib>)
endif()

set(SRC_INCLUDES "${PROJECT_SOURCE_DIR}/../test_case/src/libgit2" "${PROJECT_SOURCE_DIR}/../test_case/src/util")
set(DEPENDENCY_INCLUDES "${PROJECT_SOURCE_DIR}/../test_case/deps/xdiff" ${LIBGIT2_DEPENDENCY_INCLUDES})

add_definitions(-DGIT_REGEX_PCRE2)
list(APPEND LIBGIT2_SYSTEM_LIBS pcre2-8)
list(APPEND LIBGIT2_PC_REQUIRES "libpcre2-8")

add_definitions(-DGIT_SHA1_BUILTIN)
add_definitions(-DGIT_SHA256_BUILTIN)
if(EXPERIMENTAL_SHA256)
	add_definitions(-DGIT_EXPERIMENTAL_SHA256)
endif()
if(USE_NSEC)
	add_definitions(-DGIT_NSEC)
endif()
if(DEPRECATE_HARD)
        add_definitions(-DGIT_DEPRECATE_HARD)
endif()

# Ensure that we do not use deprecated functions internally
add_definitions(-D_DEBUG) # This is only for tests, doesn't impact that actual main library

set(SYSTEM_INCLUDES ${LIBGIT2_SYSTEM_INCLUDES})
set(LIBS ${LIBGIT2_SYSTEM_LIBS} z) # Tests test against zlib so we always need to include it
# ================================================================================================

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clar.suite ${CMAKE_CURRENT_BINARY_DIR}/clar_suite.h
	COMMAND ${PYTHON_EXECUTABLE} ${CLAR_PATH}/generate.py -o "${CMAKE_CURRENT_BINARY_DIR}" -f .
	DEPENDS ${SRC_TEST}
	WORKING_DIRECTORY ${TEST_PATH}
)

set_source_files_properties(
	${CLAR_PATH}/clar.c
	PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/clar.suite
)

add_executable(tests ${SRC_CLAR} ${SRC_TEST})
set_target_properties(tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
target_include_directories(tests PRIVATE ${SRC_INCLUDES} ${TEST_INCLUDES} ${PROJECT_INCLUDES} ${DEPENDENCY_INCLUDES} ${SYSTEM_INCLUDES})

# If the user specified a custom path to link against, use that
# Otherwise, look for the shared object `libdriver.so` in the parent's build-ninja direcotyr
if(NOT LIB_LINK_PATH)
	set(LIB_LINK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../build-ninja/libdriver.so")
else()
    # Resolve paths
    get_filename_component(LIB_LINK_PATH "${LIB_LINK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
endif()

target_link_options(tests PRIVATE "-Wl,--warn-unresolved-symbols")
target_link_libraries(tests ${LIB_LINK_PATH} util ${LIBS})
