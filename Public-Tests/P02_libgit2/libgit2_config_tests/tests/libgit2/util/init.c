#include "clar_libgit2.h"

void test_util_init__returns_count(void)
{
	/* libgit2_tests initializes us first, so we have an existing
	 * initialization.
	 */
	cl_assert_equal_i(2, git_libgit2_init());
	cl_assert_equal_i(3, git_libgit2_init());

	cl_assert_equal_i(2, git_libgit2_shutdown());
	cl_assert_equal_i(1, git_libgit2_shutdown());
}

void test_util_init__reinit_succeeds(void)
{
	cl_assert_equal_i(0, git_libgit2_shutdown());
	cl_assert_equal_i(1, git_libgit2_init());
	cl_sandbox_set_search_path_defaults();
}
