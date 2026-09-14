#include "blame_helpers.h"

static git_repository *g_repo;
static git_blame *g_blame;

void test_blame_simple__initialize(void)
{
	g_repo = NULL;
	g_blame = NULL;
}

void test_blame_simple__cleanup(void)
{
	git_blame_free(g_blame);
	git_repository_free(g_repo);
}

/*
 * $ git blame -s branch_file.txt
 *    orig line no                        final line no
 * commit   V  author       timestamp                 V
 * c47800c7 1 (Scott Chacon 2010-05-25 11:58:14 -0700 1
 * a65fedf3 2 (Scott Chacon 2011-08-09 19:33:46 -0700 2
 */
void test_blame_simple__trivial_testrepo(void)
{
	cl_git_pass(git_repository_open(&g_repo, cl_fixture("testrepo/.gitted")));
	cl_git_pass(git_blame_file(&g_blame, g_repo, "branch_file.txt", NULL));

	cl_assert_equal_i(2, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0, 1, 1, 0, "c47800c7", "branch_file.txt");
	check_blame_hunk_index(g_repo, g_blame, 1, 2, 1, 0, "a65fedf3", "branch_file.txt");
}

/*
 * $ git blame -n b.txt
 *    orig line no                          final line no
 * commit    V  author     timestamp                  V
 * da237394  1 (Ben Straub 2013-02-12 15:11:30 -0800  1
 * da237394  2 (Ben Straub 2013-02-12 15:11:30 -0800  2
 * da237394  3 (Ben Straub 2013-02-12 15:11:30 -0800  3
 * da237394  4 (Ben Straub 2013-02-12 15:11:30 -0800  4
 * ^b99f7ac  1 (Ben Straub 2013-02-12 15:10:12 -0800  5
 * 63d671eb  6 (Ben Straub 2013-02-12 15:13:04 -0800  6
 * 63d671eb  7 (Ben Straub 2013-02-12 15:13:04 -0800  7
 * 63d671eb  8 (Ben Straub 2013-02-12 15:13:04 -0800  8
 * 63d671eb  9 (Ben Straub 2013-02-12 15:13:04 -0800  9
 * 63d671eb 10 (Ben Straub 2013-02-12 15:13:04 -0800 10
 * aa06ecca  6 (Ben Straub 2013-02-12 15:14:46 -0800 11
 * aa06ecca  7 (Ben Straub 2013-02-12 15:14:46 -0800 12
 * aa06ecca  8 (Ben Straub 2013-02-12 15:14:46 -0800 13
 * aa06ecca  9 (Ben Straub 2013-02-12 15:14:46 -0800 14
 * aa06ecca 10 (Ben Straub 2013-02-12 15:14:46 -0800 15
 */
void test_blame_simple__trivial_blamerepo(void)
{
	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));
	cl_git_pass(git_blame_file(&g_blame, g_repo, "b.txt", NULL));

	cl_assert_equal_i(4, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  1, 4, 0, "da237394", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 1,  5, 1, 1, "b99f7ac0", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 2,  6, 5, 0, "63d671eb", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 3, 11, 5, 0, "aa06ecca", "b.txt");
}


/*
 * $ git blame -n b.txt -L 8
 *    orig line no                          final line no
 * commit    V  author     timestamp                  V
 * 63d671eb  8 (Ben Straub 2013-02-12 15:13:04 -0800  8
 * 63d671eb  9 (Ben Straub 2013-02-12 15:13:04 -0800  9
 * 63d671eb 10 (Ben Straub 2013-02-12 15:13:04 -0800 10
 * aa06ecca  6 (Ben Straub 2013-02-12 15:14:46 -0800 11
 * aa06ecca  7 (Ben Straub 2013-02-12 15:14:46 -0800 12
 * aa06ecca  8 (Ben Straub 2013-02-12 15:14:46 -0800 13
 * aa06ecca  9 (Ben Straub 2013-02-12 15:14:46 -0800 14
 * aa06ecca 10 (Ben Straub 2013-02-12 15:14:46 -0800 15
 */
void test_blame_simple__can_restrict_lines_min(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));

	opts.min_line = 8;
	cl_git_pass(git_blame_file(&g_blame, g_repo, "b.txt", &opts));
	cl_assert_equal_i(2, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  8, 3, 0, "63d671eb", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 1, 11, 5, 0, "aa06ecca", "b.txt");
}

/*
 * $ git blame -n c.txt
 *    orig line no                          final line no
 * commit    V  author      timestamp                  V
 * 702c7aa5  1 (Carl Schwan 2020-01-29 01:52:31 +0100  4
 */
void test_blame_simple__can_ignore_whitespace_change(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));

	opts.flags |= GIT_BLAME_IGNORE_WHITESPACE;
	cl_git_pass(git_blame_file(&g_blame, g_repo, "c.txt", &opts));
	cl_assert_equal_i(1, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  1, 4, 0, "702c7aa5", "c.txt");
}

/*
 * $ git blame -n b.txt -L ,6
 *    orig line no                          final line no
 * commit    V  author     timestamp                  V
 * da237394  1 (Ben Straub 2013-02-12 15:11:30 -0800  1
 * da237394  2 (Ben Straub 2013-02-12 15:11:30 -0800  2
 * da237394  3 (Ben Straub 2013-02-12 15:11:30 -0800  3
 * da237394  4 (Ben Straub 2013-02-12 15:11:30 -0800  4
 * ^b99f7ac  1 (Ben Straub 2013-02-12 15:10:12 -0800  5
 * 63d671eb  6 (Ben Straub 2013-02-12 15:13:04 -0800  6
 */
void test_blame_simple__can_restrict_lines_max(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));

	opts.max_line = 6;
	cl_git_pass(git_blame_file(&g_blame, g_repo, "b.txt", &opts));
	cl_assert_equal_i(3, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  1, 4, 0, "da237394", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 1,  5, 1, 1, "b99f7ac0", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 2,  6, 1, 0, "63d671eb", "b.txt");
}

/*
 * $ git blame -n b.txt -L 2,7
 *    orig line no                          final line no
 * commit   V  author     timestamp                 V
 * da237394 2 (Ben Straub 2013-02-12 15:11:30 -0800 2
 * da237394 3 (Ben Straub 2013-02-12 15:11:30 -0800 3
 * da237394 4 (Ben Straub 2013-02-12 15:11:30 -0800 4
 * ^b99f7ac 1 (Ben Straub 2013-02-12 15:10:12 -0800 5
 * 63d671eb 6 (Ben Straub 2013-02-12 15:13:04 -0800 6
 * 63d671eb 7 (Ben Straub 2013-02-12 15:13:04 -0800 7
 */
void test_blame_simple__can_restrict_lines_both(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));

	opts.min_line = 2;
	opts.max_line = 7;
	cl_git_pass(git_blame_file(&g_blame, g_repo, "b.txt", &opts));
	cl_assert_equal_i(3, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  2, 3, 0, "da237394", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 1,  5, 1, 1, "b99f7ac0", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 2,  6, 2, 0, "63d671eb", "b.txt");
}

void test_blame_simple__can_blame_huge_file(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));

	cl_git_pass(git_blame_file(&g_blame, g_repo, "huge.txt", &opts));
	cl_assert_equal_i(2, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0, 1,     65536, 0, "4eecfea", "huge.txt");
	check_blame_hunk_index(g_repo, g_blame, 1, 65537, 1,     0, "6653ff4", "huge.txt");
}

/*
 * $ git blame -n branch_file.txt be3563a..HEAD
 *    orig line no                          final line no
 * commit   V  author       timestamp                 V
 * ^be3563a 1 (Scott Chacon 2010-05-25 11:58:27 -0700 1) hi
 * a65fedf3 2 (Scott Chacon 2011-08-09 19:33:46 -0700 2) bye!
 */
void test_blame_simple__can_restrict_to_newish_commits(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("testrepo.git")));

	{
		git_object *obj;
		cl_git_pass(git_revparse_single(&obj, g_repo, "be3563a"));
		git_oid_cpy(&opts.oldest_commit, git_object_id(obj));
		git_object_free(obj);
	}

	cl_git_pass(git_blame_file(&g_blame, g_repo, "branch_file.txt", &opts));

	cl_assert_equal_i(2, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  1, 1, 1, "be3563a", "branch_file.txt");
	check_blame_hunk_index(g_repo, g_blame, 1,  2, 1, 0, "a65fedf", "branch_file.txt");
}

void test_blame_simple__can_restrict_to_first_parent_commits(void)
{
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;
	opts.flags |= GIT_BLAME_FIRST_PARENT;

	cl_git_pass(git_repository_open(&g_repo, cl_fixture("blametest.git")));

	cl_git_pass(git_blame_file(&g_blame, g_repo, "b.txt", &opts));
	cl_assert_equal_i(4, git_blame_hunkcount(g_blame));
	check_blame_hunk_index(g_repo, g_blame, 0,  1, 4, 0, "da237394", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 1,  5, 1, 1, "b99f7ac0", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 2,  6, 5, 0, "63d671eb", "b.txt");
	check_blame_hunk_index(g_repo, g_blame, 3, 11, 5, 0, "bc7c5ac2", "b.txt");
}
