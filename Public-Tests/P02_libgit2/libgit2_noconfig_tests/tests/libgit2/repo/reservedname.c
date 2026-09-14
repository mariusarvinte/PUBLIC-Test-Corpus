#include "clar_libgit2.h"
#include "../submodule/submodule_helpers.h"
#include "repository.h"

void test_repo_reservedname__cleanup(void)
{
	cl_git_sandbox_cleanup();
}

void test_repo_reservedname__includes_shortname_when_requested(void)
{
	git_repository *repo;
	git_str *reserved;
	size_t reserved_len;

	repo = cl_git_sandbox_init("nasty");
	cl_assert(git_repository__reserved_names(&reserved, &reserved_len, repo, true));

	cl_assert_equal_i(2, reserved_len);
	cl_assert_equal_s(".git", reserved[0].ptr);
	cl_assert_equal_s("GIT~1", reserved[1].ptr);
}

/* Like the `submodule_pointer` test (above), this ensures that we do not
 * follow the gitlink to the submodule's repository location and treat that
 * as a reserved name.  This tests at an initial submodule update, where the
 * submodule repo is being created.
 */
void test_repo_reservedname__submodule_pointer_during_create(void)
{
	git_repository *repo;
	git_submodule *sm;
	git_submodule_update_options update_options = GIT_SUBMODULE_UPDATE_OPTIONS_INIT;
	git_str url = GIT_STR_INIT;

	repo = setup_fixture_super();

	cl_git_pass(git_str_joinpath(&url, clar_sandbox_path(), "sub.git"));
	cl_repo_set_string(repo, "submodule.sub.url", url.ptr);

	cl_git_pass(git_submodule_lookup(&sm, repo, "sub"));
	cl_git_pass(git_submodule_update(sm, 1, &update_options));

	git_submodule_free(sm);
	git_str_dispose(&url);
}
