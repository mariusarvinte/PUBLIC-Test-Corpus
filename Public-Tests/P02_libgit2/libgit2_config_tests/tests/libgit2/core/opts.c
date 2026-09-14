#include "clar_libgit2.h"
#include "cache.h"

void test_core_opts__cleanup(void)
{
	cl_git_pass(git_libgit2_set_extensions(NULL, 0));
}

void test_core_opts__readwrite(void)
{
	size_t old_val = 0;
	size_t new_val = 0;

	git_libgit2_get_mwindow_size(&old_val);
	git_libgit2_set_mwindow_size(1234);
	git_libgit2_get_mwindow_size(&new_val);

	cl_assert(new_val == 1234);

	git_libgit2_set_mwindow_size(old_val);
	git_libgit2_get_mwindow_size(&new_val);

	cl_assert(new_val == old_val);
}

void test_core_opts__extensions_query(void)
{
	git_strarray out = { 0 };

	cl_git_pass(git_libgit2_get_extensions(&out));

	cl_assert_equal_sz(out.count, 5);
	cl_assert_equal_s("noop", out.strings[0]);
	cl_assert_equal_s("objectformat", out.strings[1]);
	cl_assert_equal_s("preciousobjects", out.strings[2]);
	cl_assert_equal_s("refstorage", out.strings[3]);
	cl_assert_equal_s("worktreeconfig", out.strings[4]);

	git_strarray_dispose(&out);
}

void test_core_opts__extensions_add(void)
{
	const char *in[] = { "foo" };
	git_strarray out = { 0 };

	cl_git_pass(git_libgit2_set_extensions(in, ARRAY_SIZE(in)));
	cl_git_pass(git_libgit2_get_extensions(&out));

	cl_assert_equal_sz(out.count, 6);
	cl_assert_equal_s("foo", out.strings[0]);
	cl_assert_equal_s("noop", out.strings[1]);
	cl_assert_equal_s("objectformat", out.strings[2]);
	cl_assert_equal_s("preciousobjects", out.strings[3]);
	cl_assert_equal_s("refstorage", out.strings[4]);
	cl_assert_equal_s("worktreeconfig", out.strings[5]);

	git_strarray_dispose(&out);
}

void test_core_opts__extensions_remove(void)
{
	const char *in[] = { "bar", "!negate", "!noop", "baz" };
	git_strarray out = { 0 };

	cl_git_pass(git_libgit2_set_extensions(in, ARRAY_SIZE(in)));
	cl_git_pass(git_libgit2_get_extensions(&out));

	cl_assert_equal_sz(out.count, 6);
	cl_assert_equal_s("bar", out.strings[0]);
	cl_assert_equal_s("baz", out.strings[1]);
	cl_assert_equal_s("objectformat", out.strings[2]);
	cl_assert_equal_s("preciousobjects", out.strings[3]);
	cl_assert_equal_s("refstorage", out.strings[4]);
	cl_assert_equal_s("worktreeconfig", out.strings[5]);

	git_strarray_dispose(&out);
}

void test_core_opts__extensions_uniq(void)
{
	const char *in[] = {
		"foo", "noop", "bar", "bar", "foo", "objectformat"
	};
	git_strarray out = { 0 };

	cl_git_pass(git_libgit2_set_extensions(in, ARRAY_SIZE(in)));
	cl_git_pass(git_libgit2_get_extensions(&out));

	cl_assert_equal_sz(out.count, 7);
	cl_assert_equal_s("bar", out.strings[0]);
	cl_assert_equal_s("foo", out.strings[1]);
	cl_assert_equal_s("noop", out.strings[2]);
	cl_assert_equal_s("objectformat", out.strings[3]);
	cl_assert_equal_s("preciousobjects", out.strings[4]);
	cl_assert_equal_s("refstorage", out.strings[5]);
	cl_assert_equal_s("worktreeconfig", out.strings[6]);

	git_strarray_dispose(&out);
}
