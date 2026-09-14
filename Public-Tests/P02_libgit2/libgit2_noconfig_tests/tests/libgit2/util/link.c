#include "clar_libgit2.h"
#include "posix.h"


void test_util_link__cleanup(void)
{
}

static bool should_run(void)
{
	return true;
}

static void do_symlink(const char *old, const char *new, int is_dir)
{
	GIT_UNUSED(is_dir);

	cl_must_pass(symlink(old, new));
}

static void do_hardlink(const char *old, const char *new)
{
	cl_must_pass(link(old, new));
}


void test_util_link__stat_regular_file(void)
{
	struct stat st;

	cl_git_rewritefile("stat_regfile", "This is a regular file!\n");

	cl_must_pass(p_stat("stat_regfile", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(24, st.st_size);
}

void test_util_link__lstat_regular_file(void)
{
	struct stat st;

	cl_git_rewritefile("lstat_regfile", "This is a regular file!\n");

	cl_must_pass(p_stat("lstat_regfile", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(24, st.st_size);
}

void test_util_link__stat_symlink(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	cl_git_rewritefile("stat_target", "This is the target of a symbolic link.\n");
	do_symlink("stat_target", "stat_symlink", 0);

	cl_must_pass(p_stat("stat_target", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(39, st.st_size);

	cl_must_pass(p_stat("stat_symlink", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(39, st.st_size);
}

void test_util_link__stat_symlink_directory(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	p_mkdir("stat_dirtarget", 0777);
	do_symlink("stat_dirtarget", "stat_dirlink", 1);

	cl_must_pass(p_stat("stat_dirtarget", &st));
	cl_assert(S_ISDIR(st.st_mode));

	cl_must_pass(p_stat("stat_dirlink", &st));
	cl_assert(S_ISDIR(st.st_mode));
}

void test_util_link__stat_symlink_chain(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	cl_git_rewritefile("stat_final_target", "Final target of some symbolic links...\n");
	do_symlink("stat_final_target", "stat_chain_3", 0);
	do_symlink("stat_chain_3", "stat_chain_2", 0);
	do_symlink("stat_chain_2", "stat_chain_1", 0);

	cl_must_pass(p_stat("stat_chain_1", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(39, st.st_size);
}

void test_util_link__stat_dangling_symlink(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	do_symlink("stat_nonexistent", "stat_dangling", 0);

	cl_must_fail(p_stat("stat_nonexistent", &st));
	cl_must_fail(p_stat("stat_dangling", &st));
}

void test_util_link__stat_dangling_symlink_directory(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	do_symlink("stat_nonexistent", "stat_dangling_dir", 1);

	cl_must_fail(p_stat("stat_nonexistent_dir", &st));
	cl_must_fail(p_stat("stat_dangling", &st));
}

void test_util_link__lstat_symlink(void)
{
	git_str target_path = GIT_STR_INIT;
	struct stat st;

	if (!should_run())
		clar__skip();

	/* Windows always writes the canonical path as the link target, so
	 * write the full path on all platforms.
	 */
	git_str_join(&target_path, '/', clar_sandbox_path(), "lstat_target");

	cl_git_rewritefile("lstat_target", "This is the target of a symbolic link.\n");
	do_symlink(git_str_cstr(&target_path), "lstat_symlink", 0);

	cl_must_pass(p_lstat("lstat_target", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(39, st.st_size);

	cl_must_pass(p_lstat("lstat_symlink", &st));
	cl_assert(S_ISLNK(st.st_mode));
	cl_assert_equal_i(git_str_len(&target_path), st.st_size);

	git_str_dispose(&target_path);
}

void test_util_link__lstat_symlink_directory(void)
{
	git_str target_path = GIT_STR_INIT;
	struct stat st;

	if (!should_run())
		clar__skip();

	git_str_join(&target_path, '/', clar_sandbox_path(), "lstat_dirtarget");

	p_mkdir("lstat_dirtarget", 0777);
	do_symlink(git_str_cstr(&target_path), "lstat_dirlink", 1);

	cl_must_pass(p_lstat("lstat_dirtarget", &st));
	cl_assert(S_ISDIR(st.st_mode));

	cl_must_pass(p_lstat("lstat_dirlink", &st));
	cl_assert(S_ISLNK(st.st_mode));
	cl_assert_equal_i(git_str_len(&target_path), st.st_size);

	git_str_dispose(&target_path);
}

void test_util_link__lstat_dangling_symlink(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	do_symlink("lstat_nonexistent", "lstat_dangling", 0);

	cl_must_fail(p_lstat("lstat_nonexistent", &st));

	cl_must_pass(p_lstat("lstat_dangling", &st));
	cl_assert(S_ISLNK(st.st_mode));
	cl_assert_equal_i(strlen("lstat_nonexistent"), st.st_size);
}

void test_util_link__lstat_dangling_symlink_directory(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	do_symlink("lstat_nonexistent", "lstat_dangling_dir", 1);

	cl_must_fail(p_lstat("lstat_nonexistent", &st));

	cl_must_pass(p_lstat("lstat_dangling_dir", &st));
	cl_assert(S_ISLNK(st.st_mode));
	cl_assert_equal_i(strlen("lstat_nonexistent"), st.st_size);
}

void test_util_link__stat_hardlink(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	cl_git_rewritefile("stat_hardlink1", "This file has many names!\n");
	do_hardlink("stat_hardlink1", "stat_hardlink2");

	cl_must_pass(p_stat("stat_hardlink1", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(26, st.st_size);

	cl_must_pass(p_stat("stat_hardlink2", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(26, st.st_size);
}

void test_util_link__lstat_hardlink(void)
{
	struct stat st;

	if (!should_run())
		clar__skip();

	cl_git_rewritefile("lstat_hardlink1", "This file has many names!\n");
	do_hardlink("lstat_hardlink1", "lstat_hardlink2");

	cl_must_pass(p_lstat("lstat_hardlink1", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(26, st.st_size);

	cl_must_pass(p_lstat("lstat_hardlink2", &st));
	cl_assert(S_ISREG(st.st_mode));
	cl_assert_equal_i(26, st.st_size);
}

void test_util_link__readlink_nonexistent_file(void)
{
	char buf[2048];

	cl_must_fail(p_readlink("readlink_nonexistent", buf, 2048));
	cl_assert_equal_i(ENOENT, errno);
}

void test_util_link__readlink_normal_file(void)
{
	char buf[2048];

	cl_git_rewritefile("readlink_regfile", "This is a regular file!\n");
	cl_must_fail(p_readlink("readlink_regfile", buf, 2048));
	cl_assert_equal_i(EINVAL, errno);
}

void test_util_link__readlink_symlink(void)
{
	git_str target_path = GIT_STR_INIT;
	int len;
	char buf[2048];

	if (!should_run())
		clar__skip();

	git_str_join(&target_path, '/', clar_sandbox_path(), "readlink_target");

	cl_git_rewritefile("readlink_target", "This is the target of a symlink\n");
	do_symlink(git_str_cstr(&target_path), "readlink_link", 0);

	len = p_readlink("readlink_link", buf, 2048);
	cl_must_pass(len);

	buf[len] = 0;

	cl_assert_equal_s(git_str_cstr(&target_path), buf);

	git_str_dispose(&target_path);
}

void test_util_link__readlink_dangling(void)
{
	git_str target_path = GIT_STR_INIT;
	int len;
	char buf[2048];

	if (!should_run())
		clar__skip();

	git_str_join(&target_path, '/', clar_sandbox_path(), "readlink_nonexistent");

	do_symlink(git_str_cstr(&target_path), "readlink_dangling", 0);

	len = p_readlink("readlink_dangling", buf, 2048);
	cl_must_pass(len);

	buf[len] = 0;

	cl_assert_equal_s(git_str_cstr(&target_path), buf);

	git_str_dispose(&target_path);
}

void test_util_link__readlink_multiple(void)
{
	git_str target_path = GIT_STR_INIT,
		path3 = GIT_STR_INIT, path2 = GIT_STR_INIT, path1 = GIT_STR_INIT;
	int len;
	char buf[2048];

	if (!should_run())
		clar__skip();

	git_str_join(&target_path, '/', clar_sandbox_path(), "readlink_final");
	git_str_join(&path3, '/', clar_sandbox_path(), "readlink_3");
	git_str_join(&path2, '/', clar_sandbox_path(), "readlink_2");
	git_str_join(&path1, '/', clar_sandbox_path(), "readlink_1");

	do_symlink(git_str_cstr(&target_path), git_str_cstr(&path3), 0);
	do_symlink(git_str_cstr(&path3), git_str_cstr(&path2), 0);
	do_symlink(git_str_cstr(&path2), git_str_cstr(&path1), 0);

	len = p_readlink("readlink_1", buf, 2048);
	cl_must_pass(len);

	buf[len] = 0;

	cl_assert_equal_s(git_str_cstr(&path2), buf);

	git_str_dispose(&path1);
	git_str_dispose(&path2);
	git_str_dispose(&path3);
	git_str_dispose(&target_path);
}
