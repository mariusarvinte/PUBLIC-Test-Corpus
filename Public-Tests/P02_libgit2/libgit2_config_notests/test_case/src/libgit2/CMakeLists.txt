# © 2026 Massachusetts Institute of Technology
# MIT License

# libgit2: the shared library: this CMakeLists.txt compiles the core
# git library functionality.

add_library(libgit2 OBJECT)
target_link_libraries(libgit2 PRIVATE util)

set(LIBGIT2_INCLUDES
	"${PROJECT_BINARY_DIR}/src/util"
	"${PROJECT_BINARY_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/libgit2"
	"${PROJECT_SOURCE_DIR}/src/util"
	"${PROJECT_SOURCE_DIR}/include")

# Collect sourcefiles
set(SRC_H
	${PROJECT_SOURCE_DIR}/include/git2.h
	${PROJECT_SOURCE_DIR}/include/git2/annotated_commit.h
	${PROJECT_SOURCE_DIR}/include/git2/apply.h
	${PROJECT_SOURCE_DIR}/include/git2/attr.h
	${PROJECT_SOURCE_DIR}/include/git2/blame.h
	${PROJECT_SOURCE_DIR}/include/git2/blob.h
	${PROJECT_SOURCE_DIR}/include/git2/branch.h
	${PROJECT_SOURCE_DIR}/include/git2/buffer.h
	${PROJECT_SOURCE_DIR}/include/git2/checkout.h
	${PROJECT_SOURCE_DIR}/include/git2/cherrypick.h
	${PROJECT_SOURCE_DIR}/include/git2/clone.h
	${PROJECT_SOURCE_DIR}/include/git2/commit.h
	${PROJECT_SOURCE_DIR}/include/git2/common.h
	${PROJECT_SOURCE_DIR}/include/git2/config.h
	${PROJECT_SOURCE_DIR}/include/git2/deprecated.h
	${PROJECT_SOURCE_DIR}/include/git2/describe.h
	${PROJECT_SOURCE_DIR}/include/git2/diff.h
	${PROJECT_SOURCE_DIR}/include/git2/email.h
	${PROJECT_SOURCE_DIR}/include/git2/errors.h
	${PROJECT_SOURCE_DIR}/include/git2/filter.h
	${PROJECT_SOURCE_DIR}/include/git2/global.h
	${PROJECT_SOURCE_DIR}/include/git2/graph.h
	${PROJECT_SOURCE_DIR}/include/git2/ignore.h
	${PROJECT_SOURCE_DIR}/include/git2/index.h
	${PROJECT_SOURCE_DIR}/include/git2/indexer.h
	${PROJECT_SOURCE_DIR}/include/git2/mailmap.h
	${PROJECT_SOURCE_DIR}/include/git2/merge.h
	${PROJECT_SOURCE_DIR}/include/git2/message.h
	${PROJECT_SOURCE_DIR}/include/git2/net.h
	${PROJECT_SOURCE_DIR}/include/git2/notes.h
	${PROJECT_SOURCE_DIR}/include/git2/object.h
	${PROJECT_SOURCE_DIR}/include/git2/odb.h
	${PROJECT_SOURCE_DIR}/include/git2/odb_backend.h
	${PROJECT_SOURCE_DIR}/include/git2/oid.h
	${PROJECT_SOURCE_DIR}/include/git2/oidarray.h
	${PROJECT_SOURCE_DIR}/include/git2/pack.h
	${PROJECT_SOURCE_DIR}/include/git2/patch.h
	${PROJECT_SOURCE_DIR}/include/git2/pathspec.h
	${PROJECT_SOURCE_DIR}/include/git2/rebase.h
	${PROJECT_SOURCE_DIR}/include/git2/refdb.h
	${PROJECT_SOURCE_DIR}/include/git2/reflog.h
	${PROJECT_SOURCE_DIR}/include/git2/refs.h
	${PROJECT_SOURCE_DIR}/include/git2/refspec.h
	${PROJECT_SOURCE_DIR}/include/git2/remote.h
	${PROJECT_SOURCE_DIR}/include/git2/repository.h
	${PROJECT_SOURCE_DIR}/include/git2/reset.h
	${PROJECT_SOURCE_DIR}/include/git2/revert.h
	${PROJECT_SOURCE_DIR}/include/git2/revparse.h
	${PROJECT_SOURCE_DIR}/include/git2/revwalk.h
	${PROJECT_SOURCE_DIR}/include/git2/signature.h
	${PROJECT_SOURCE_DIR}/include/git2/stash.h
	${PROJECT_SOURCE_DIR}/include/git2/status.h
	${PROJECT_SOURCE_DIR}/include/git2/stdint.h
	${PROJECT_SOURCE_DIR}/include/git2/strarray.h
	${PROJECT_SOURCE_DIR}/include/git2/submodule.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/alloc.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/commit.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/commit_graph.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/config.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/diff.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/email.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/errors.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/filter.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/hashsig.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/index.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/mempack.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/merge.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/midx.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/odb_backend.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/path.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/refdb_backend.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/refs.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/remote.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/repository.h
	${PROJECT_SOURCE_DIR}/include/git2/sys/transport.h
	${PROJECT_SOURCE_DIR}/include/git2/tag.h
	${PROJECT_SOURCE_DIR}/include/git2/trace.h
	${PROJECT_SOURCE_DIR}/include/git2/transaction.h
	${PROJECT_SOURCE_DIR}/include/git2/transport.h
	${PROJECT_SOURCE_DIR}/include/git2/tree.h
	${PROJECT_SOURCE_DIR}/include/git2/types.h
	${PROJECT_SOURCE_DIR}/include/git2/version.h
	${PROJECT_SOURCE_DIR}/include/git2/worktree.h)
target_sources(libgit2 PRIVATE ${SRC_H})

set(SRC_GIT2
	annotated_commit.c
	annotated_commit.h
	apply.c
	apply.h
	attr.c
	attr.h
	attr_file.c
	attr_file.h
	attrcache.c
	attrcache.h
	blame.c
	blame.h
	blame_git.c
	blame_git.h
	blob.c
	blob.h
	branch.c
	branch.h
	buf.c
	buf.h
	cache.c
	cache.h
	checkout.c
	checkout.h
	cherrypick.c
	clone.c
	clone.h
	commit.c
	commit.h
	commit_graph.c
	commit_graph.h
	commit_list.c
	commit_list.h
	common.h
	config.c
	config.h
	config_backend.h
	config_cache.c
	config_file.c
	config_list.c
	config_list.h
	config_mem.c
	config_parse.c
	config_parse.h
	config_snapshot.c
	crlf.c
	delta.c
	delta.h
	describe.c
	diff.c
	diff.h
	diff_driver.c
	diff_driver.h
	diff_file.c
	diff_file.h
	diff_generate.c
	diff_generate.h
	diff_parse.c
	diff_parse.h
	diff_print.c
	diff_stats.c
	diff_stats.h
	diff_tform.c
	diff_tform.h
	diff_xdiff.c
	diff_xdiff.h
	email.c
	email.h
	fetch.c
	fetch.h
	fetchhead.c
	fetchhead.h
	filter.c
	filter.h
	grafts.c
	grafts.h
	graph.c
	hashmap_oid.h
	hashsig.c
	ident.c
	ignore.c
	ignore.h
	index.c
	index.h
	index_map.c
	index_map.h
	indexer.c
	indexer.h
	iterator.c
	iterator.h
	libgit2.c
	mailmap.c
	mailmap.h
	merge.c
	merge.h
	merge_driver.c
	merge_driver.h
	merge_file.c
	message.c
	midx.c
	midx.h
	mwindow.c
	mwindow.h
	notes.c
	notes.h
	object.c
	object.h
	object_api.c
	odb.c
	odb.h
	odb_loose.c
	odb_mempack.c
	odb_pack.c
	oid.c
	oid.h
	oidarray.c
	oidarray.h
	pack-objects.c
	pack-objects.h
	pack.c
	pack.h
	packref.h
	parse.c
	parse.h
	patch.c
	patch.h
	patch_generate.c
	patch_generate.h
	patch_parse.c
	patch_parse.h
	path.c
	path.h
	pathspec.c
	pathspec.h
	push.c
	push.h
	reader.c
	reader.h
	rebase.c
	refdb.c
	refdb.h
	refdb_fs.c
	reflog.c
	reflog.h
	refs.c
	refs.h
	refspec.c
	refspec.h
	remote.c
	remote.h
	repo_template.h
	repository.c
	repository.h
	reset.c
	revert.c
	revparse.c
	revwalk.c
	revwalk.h
	settings.c
	settings.h
	signature.c
	signature.h
	stash.c
	status.c
	status.h
	strarray.c
	strarray.h
	stream.h
	submodule.c
	submodule.h
	sysdir.c
	sysdir.h
	tag.c
	tag.h
	trace.c
	trace.h
	trailer.c
	transaction.c
	transaction.h
	transport.c
	transports/local.c
	tree-cache.c
	tree-cache.h
	tree.c
	tree.h
	userdiff.h
	worktree.c
	worktree.h)
target_sources(libgit2 PRIVATE ${SRC_GIT2})

list(APPEND LIBGIT2_OBJECTS $<TARGET_OBJECTS:util> $<TARGET_OBJECTS:libgit2> ${LIBGIT2_DEPENDENCY_OBJECTS})
list(APPEND LIBGIT2_INCLUDES ${LIBGIT2_DEPENDENCY_INCLUDES})

target_include_directories(libgit2 PRIVATE ${LIBGIT2_INCLUDES} ${LIBGIT2_DEPENDENCY_INCLUDES} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(libgit2 SYSTEM PRIVATE ${LIBGIT2_SYSTEM_INCLUDES})

set(LIBGIT2_INCLUDES ${LIBGIT2_INCLUDES} PARENT_SCOPE)
set(LIBGIT2_OBJECTS ${LIBGIT2_OBJECTS} PARENT_SCOPE)
set(LIBGIT2_DEPENDENCY_INCLUDES ${LIBGIT2_DEPENDENCY_INCLUDES} PARENT_SCOPE)
set(LIBGIT2_DEPENDENCY_OBJECTS ${LIBGIT2_DEPENDENCY_OBJECTS} PARENT_SCOPE)
set(LIBGIT2_SYSTEM_INCLUDES ${LIBGIT2_SYSTEM_INCLUDES} PARENT_SCOPE)
set(LIBGIT2_SYSTEM_LIBS ${LIBGIT2_SYSTEM_LIBS} PARENT_SCOPE)

#
# Compile and link libgit2
#

add_library(libgit2package SHARED ${SRC_RC} ${LIBGIT2_OBJECTS})
target_link_libraries(libgit2package ${LIBGIT2_SYSTEM_LIBS})
target_include_directories(libgit2package SYSTEM PRIVATE ${LIBGIT2_INCLUDES})
target_include_directories(libgit2package INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(libgit2package PROPERTIES C_STANDARD 90)
set_target_properties(libgit2package PROPERTIES C_EXTENSIONS OFF)
set_target_properties(libgit2package PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(libgit2package PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(libgit2package PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(libgit2package PROPERTIES OUTPUT_NAME driver)
