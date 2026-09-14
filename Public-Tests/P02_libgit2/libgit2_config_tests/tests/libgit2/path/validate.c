#include "clar_libgit2.h"
#include "path.h"

void test_path_validate__cleanup(void)
{
	cl_git_sandbox_cleanup();
}

void test_path_validate__length(void)
{
	cl_must_pass(git_path_validate_length(NULL, "/foo/bar"));
	cl_must_pass(git_path_validate_length(NULL, "C:\\Foo\\Bar"));
	cl_must_pass(git_path_validate_length(NULL, "\\\\?\\C:\\Foo\\Bar"));
	cl_must_pass(git_path_validate_length(NULL, "\\\\?\\C:\\Foo\\Bar"));
	cl_must_pass(git_path_validate_length(NULL, "\\\\?\\UNC\\server\\C$\\folder"));

	cl_must_pass(git_path_validate_length(NULL, "/c/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/ok.txt"));
	cl_must_pass(git_path_validate_length(NULL, "/c/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/260.txt"));
	cl_must_pass(git_path_validate_length(NULL, "/c/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/aaaaaaaaa/longer_than_260.txt"));
	cl_must_pass(git_path_validate_length(NULL, "C:\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\\260.txt"));
	cl_must_pass(git_path_validate_length(NULL, "C:\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\aaaaaaaaa\\\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\xc2\xa2\\long.txt"));
}

void test_path_validate__length_with_core_longpath(void)
{
}
