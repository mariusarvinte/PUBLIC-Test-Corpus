#include "clar_libgit2.h"
#include "repository.h"
#include "../submodule/submodule_helpers.h"

static git_repository *g_repo;
static git_index *g_idx;

void test_index_bypath__initialize(void)
{
	g_repo = setup_fixture_submod2();
	cl_git_pass(git_repository_index__weakptr(&g_idx, g_repo));
}

void test_index_bypath__cleanup(void)
{
	g_repo = NULL;
	g_idx = NULL;
}

void test_index_bypath__add_directory(void)
{
	cl_git_fail_with(GIT_EDIRECTORY, git_index_add_bypath(g_idx, "just_a_dir"));
}

void test_index_bypath__add_submodule(void)
{
	unsigned int status;
	const char *sm_name = "sm_changed_head";

	cl_git_pass(git_submodule_status(&status, g_repo, sm_name, 0));
	cl_assert_equal_i(GIT_SUBMODULE_STATUS_WD_MODIFIED, status & GIT_SUBMODULE_STATUS_WD_MODIFIED);
	cl_git_pass(git_index_add_bypath(g_idx, sm_name));
	cl_git_pass(git_submodule_status(&status, g_repo, sm_name, 0));
	cl_assert_equal_i(0, status & GIT_SUBMODULE_STATUS_WD_MODIFIED);
}

void test_index_bypath__add_submodule_unregistered(void)
{
	const char *sm_name = "not-submodule";
	const char *sm_head = "68e92c611b80ee1ed8f38314ff9577f0d15b2444";
	const git_index_entry *entry;

	cl_git_pass(git_index_add_bypath(g_idx, sm_name));

	cl_assert(entry = git_index_get_bypath(g_idx, sm_name, 0));
	cl_assert_equal_s(sm_head, git_oid_tostr_s(&entry->id));
	cl_assert_equal_s(sm_name, entry->path);
}

void test_index_bypath__add_honors_mode(void)
{
	const git_index_entry *entry;
	git_index_entry new_entry;

	cl_assert((entry = git_index_get_bypath(g_idx, "README.txt", 0)) != NULL);

	memcpy(&new_entry, entry, sizeof(git_index_entry));
	new_entry.path = "README.txt";
	new_entry.mode = GIT_FILEMODE_BLOB_EXECUTABLE;

	cl_must_pass(p_chmod("submod2/README.txt", GIT_FILEMODE_BLOB_EXECUTABLE));

	cl_git_pass(git_index_add(g_idx, &new_entry));
	cl_git_pass(git_index_write(g_idx));

	cl_git_rewritefile("submod2/README.txt", "Modified but still executable");

	cl_git_pass(git_index_add_bypath(g_idx, "README.txt"));
	cl_git_pass(git_index_write(g_idx));

	cl_assert((entry = git_index_get_bypath(g_idx, "README.txt", 0)) != NULL);
	cl_assert_equal_i(GIT_FILEMODE_BLOB_EXECUTABLE, entry->mode);
}

void test_index_bypath__add_honors_conflict_mode(void)
{
	const git_index_entry *entry;
	git_index_entry new_entry;
	int stage = 0;

	cl_assert((entry = git_index_get_bypath(g_idx, "README.txt", 0)) != NULL);

	memcpy(&new_entry, entry, sizeof(git_index_entry));
	new_entry.path = "README.txt";
	new_entry.mode = GIT_FILEMODE_BLOB_EXECUTABLE;

	cl_must_pass(p_chmod("submod2/README.txt", GIT_FILEMODE_BLOB_EXECUTABLE));

	cl_git_pass(git_index_remove_bypath(g_idx, "README.txt"));

	for (stage = 1; stage <= 3; stage++) {
		new_entry.flags = stage << GIT_INDEX_ENTRY_STAGESHIFT;
		cl_git_pass(git_index_add(g_idx, &new_entry));
	}

	cl_git_pass(git_index_write(g_idx));

	cl_git_rewritefile("submod2/README.txt", "Modified but still executable");

	cl_git_pass(git_index_add_bypath(g_idx, "README.txt"));
	cl_git_pass(git_index_write(g_idx));

	cl_assert((entry = git_index_get_bypath(g_idx, "README.txt", 0)) != NULL);
	cl_assert_equal_i(GIT_FILEMODE_BLOB_EXECUTABLE, entry->mode);
}

void test_index_bypath__add_honors_conflict_case(void)
{
	const git_index_entry *entry;
	git_index_entry new_entry;
	int stage = 0;

	cl_assert((entry = git_index_get_bypath(g_idx, "README.txt", 0)) != NULL);

	memcpy(&new_entry, entry, sizeof(git_index_entry));
	new_entry.path = "README.txt";
	new_entry.mode = GIT_FILEMODE_BLOB_EXECUTABLE;

	cl_must_pass(p_chmod("submod2/README.txt", GIT_FILEMODE_BLOB_EXECUTABLE));

	cl_git_pass(git_index_remove_bypath(g_idx, "README.txt"));

	for (stage = 1; stage <= 3; stage++) {
		new_entry.flags = stage << GIT_INDEX_ENTRY_STAGESHIFT;
		cl_git_pass(git_index_add(g_idx, &new_entry));
	}

	cl_git_pass(git_index_write(g_idx));

	cl_git_rewritefile("submod2/README.txt", "Modified but still executable");

	cl_git_pass(git_index_add_bypath(g_idx, "README.txt"));
	cl_git_pass(git_index_write(g_idx));

	cl_assert((entry = git_index_get_bypath(g_idx, "README.txt", 0)) != NULL);
	cl_assert_equal_i(GIT_FILEMODE_BLOB_EXECUTABLE, entry->mode);
}
