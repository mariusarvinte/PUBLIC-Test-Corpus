# © 2026 Massachusetts Institute of Technology
# MIT License

set(CLI_INCLUDES
	"${libgit2_BINARY_DIR}/src/util"
	"${libgit2_BINARY_DIR}/include"
	"${libgit2_SOURCE_DIR}/src/util"
	"${libgit2_SOURCE_DIR}/src/cli"
	"${libgit2_SOURCE_DIR}/include"
	"${LIBGIT2_DEPENDENCY_INCLUDES}"
	"${LIBGIT2_SYSTEM_INCLUDES}")

set(CLI_SRC
	cmd/add.c
	cmd/blame.c
	cmd/branch.c
	cmd/cat_file.c
	cmd/cherry_pick.c
	cmd/checkout.c
	cmd/clone.c
	cmd/cmd.c
	cmd/cmd.h
	cmd/commit.c
	cmd/config.c
	cmd/demo.c
	cmd/describe.c
	cmd/diff.c
	cmd/fetch.c
	cmd/for_each_ref.c
	cmd/hash_object.c
	cmd/help.c
	cmd/index_pack.c
	cmd/init.c
	cmd/log.c
	cmd/ls_files.c
	cmd/ls_remote.c
	cmd/merge.c
	cmd/note.c
	cmd/push.c
	cmd/rebase.c
	cmd/remote.c
	cmd/reset.c
	cmd/rev_list.c
	cmd/rev_parse.c
	cmd/show_index.c
	cmd/stash.c
	cmd/status.c
	cmd/submodule.c
	cmd/tag.c
	cmd/version.c
	cmd/worktree.c
	common.c
	common.h
	error.h
	main.c
	opt.c
	opt.h
	opt_usage.c
	opt_usage.h
	progress.c
	progress.h)

set(CLI_LIBGIT2_LIBRARY libgit2package)

# Compile and link the CLI
#

add_executable(git2_cli ${CLI_SRC} ${CLI_OBJECTS}
	${CLI_LIBGIT2_OBJECTS})
target_link_libraries(git2_cli ${CLI_LIBGIT2_LIBRARY} ${LIBGIT2_SYSTEM_LIBS})

set_target_properties(git2_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(git2_cli PROPERTIES OUTPUT_NAME driver)
set_target_properties(git2_cli PROPERTIES BUILD_RPATH "$ORIGIN")
set_target_properties(git2_cli PROPERTIES INSTALL_RPATH "$ORIGIN")

target_include_directories(git2_cli PRIVATE ${CLI_INCLUDES})
