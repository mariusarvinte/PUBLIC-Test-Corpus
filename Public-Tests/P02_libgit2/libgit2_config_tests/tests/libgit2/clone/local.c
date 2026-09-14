#include "clar_libgit2.h"

#include "git2/clone.h"
#include "clone.h"
#include "path.h"
#include "posix.h"
#include "futils.h"

static int file_url(git_str *buf, const char *host, const char *path)
{
	if (path[0] == '/')
		path++;

	git_str_clear(buf);
	return git_str_printf(buf, "file://%s/%s", host, path);
}

void test_clone_local__should_clone_local(void)
{
	git_str buf = GIT_STR_INIT;
	bool local;

	/* we use a fixture path because it needs to exist for us to want to
	 * clone */
	const char *path = cl_fixture("testrepo.git");

	/* empty string */
	cl_git_pass(file_url(&buf, "", path));
	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_AUTO));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_AUTO));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_NO_LINKS));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_NO_LOCAL));
	cl_assert_equal_i(false, local);

	/* localhost is special */
	cl_git_pass(file_url(&buf, "localhost", path));
	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_AUTO));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_NO_LINKS));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_NO_LOCAL));
	cl_assert_equal_i(false, local);

	/* a remote host */
	cl_git_pass(file_url(&buf, "other-host.mycompany.com", path));

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_AUTO));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_NO_LINKS));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_NO_LOCAL));
	cl_assert_equal_i(false, local);

	/* Ensure that file:/// urls are percent decoded: .git == %2e%67%69%74
	 */
	cl_git_pass(file_url(&buf, "", path));
	git_str_shorten(&buf, 4);
	cl_git_pass(git_str_puts(&buf, "%2e%67%69%74"));

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_AUTO));
	cl_assert_equal_i(false, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_LOCAL_NO_LINKS));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, buf.ptr, GIT_CLONE_NO_LOCAL));
	cl_assert_equal_i(false, local);

	/* a local path on disk */
	cl_git_pass(git_clone__should_clone_local(
	        &local, path, GIT_CLONE_LOCAL_AUTO));
	cl_assert_equal_i(true, local);

	cl_git_pass(
	        git_clone__should_clone_local(&local, path, GIT_CLONE_LOCAL));

	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, path, GIT_CLONE_LOCAL_NO_LINKS));
	cl_assert_equal_i(true, local);

	cl_git_pass(git_clone__should_clone_local(
	        &local, path, GIT_CLONE_NO_LOCAL));
	cl_assert_equal_i(false, local);

	git_str_dispose(&buf);
}

void test_clone_local__hardlinks(void)
{
	git_repository *repo;
	git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
	git_str buf = GIT_STR_INIT;
	struct stat st;

	/*
	 * In this first clone, we just copy over, since the temp dir
	 * will often be in a different filesystem, so we cannot
	 * link. It also allows us to control the number of links
	 */
	opts.bare = true;
	opts.local = GIT_CLONE_LOCAL_NO_LINKS;
	cl_git_pass(git_clone(
	        &repo, cl_fixture("testrepo.git"), "./clone.git", &opts));
	git_repository_free(repo);

	/* This second clone is in the same filesystem, so we can hardlink */

	opts.local = GIT_CLONE_LOCAL;
	cl_git_pass(git_clone(
	        &repo, cl_git_path_url("clone.git"), "./clone2.git", &opts));

	git_str_clear(&buf);
	{
		const char *parts[] = {
			git_repository_path(repo), "objects", "08",
			"b041783f40edfe12bb406c9c9a8a040177c125"
		};
		cl_git_pass(git_str_join_array(&buf, '/', 4, parts));
	}

	cl_git_pass(p_stat(buf.ptr, &st));
	cl_assert_equal_i(2, st.st_nlink);

	git_repository_free(repo);
	git_str_clear(&buf);

	opts.local = GIT_CLONE_LOCAL_NO_LINKS;
	cl_git_pass(git_clone(
	        &repo, cl_git_path_url("clone.git"), "./clone3.git", &opts));

	git_str_clear(&buf);
	{
		const char *parts[] = {
			git_repository_path(repo), "objects", "08",
			"b041783f40edfe12bb406c9c9a8a040177c125"
		};
		cl_git_pass(git_str_join_array(&buf, '/', 4, parts));
	}

	cl_git_pass(p_stat(buf.ptr, &st));
	cl_assert_equal_i(1, st.st_nlink);

	git_repository_free(repo);

	/* this one should automatically use links */
	cl_git_pass(git_clone(&repo, "./clone.git", "./clone4.git", NULL));

	git_str_clear(&buf);
	{
		const char *parts[] = {
			git_repository_path(repo), "objects", "08",
			"b041783f40edfe12bb406c9c9a8a040177c125"
		};
		cl_git_pass(git_str_join_array(&buf, '/', 4, parts));
	}

	cl_git_pass(p_stat(buf.ptr, &st));
	cl_assert_equal_i(3, st.st_nlink);

	git_str_dispose(&buf);
	git_repository_free(repo);

	cl_git_pass(git_futils_rmdir_r(
	        "./clone.git", NULL, GIT_RMDIR_REMOVE_FILES));
	cl_git_pass(git_futils_rmdir_r(
	        "./clone2.git", NULL, GIT_RMDIR_REMOVE_FILES));
	cl_git_pass(git_futils_rmdir_r(
	        "./clone3.git", NULL, GIT_RMDIR_REMOVE_FILES));
	cl_git_pass(git_futils_rmdir_r(
	        "./clone4.git", NULL, GIT_RMDIR_REMOVE_FILES));
}

void test_clone_local__shallow_fails(void)
{
	git_repository *repo;
	git_clone_options opts = GIT_CLONE_OPTIONS_INIT;

	opts.fetch_opts.depth = 4;

	cl_git_fail_with(
	        GIT_ENOTSUPPORTED, git_clone(
	                                   &repo, cl_fixture("testrepo.git"),
	                                   "./clone.git", &opts));
}
