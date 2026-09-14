#include "clar_libgit2.h"

#include <locale.h>

#include "regexp.h"

#if LC_ALL > 0
static const char *old_locales[LC_ALL];
#endif

static git_regexp regex;

void test_util_regexp__initialize(void)
{
#if LC_ALL > 0
	memset(&old_locales, 0, sizeof(old_locales));
#endif
}

void test_util_regexp__cleanup(void)
{
	git_regexp_dispose(&regex);
}

void test_util_regexp__simple_search_matches(void)
{
	cl_git_pass(git_regexp_compile(&regex, "a", 0));
	cl_git_pass(git_regexp_search(&regex, "a", 0, NULL));
}

void test_util_regexp__case_insensitive_search_matches(void)
{
	cl_git_pass(git_regexp_compile(&regex, "a", GIT_REGEXP_ICASE));
	cl_git_pass(git_regexp_search(&regex, "A", 0, NULL));
}

void test_util_regexp__nonmatching_search_returns_error(void)
{
	cl_git_pass(git_regexp_compile(&regex, "a", 0));
	cl_git_fail(git_regexp_search(&regex, "b", 0, NULL));
}

void test_util_regexp__search_finds_complete_match(void)
{
	git_regmatch matches[1];

	cl_git_pass(git_regexp_compile(&regex, "abc", 0));
	cl_git_pass(git_regexp_search(&regex, "abc", 1, matches));
	cl_assert_equal_i(matches[0].start, 0);
	cl_assert_equal_i(matches[0].end, 3);
}

void test_util_regexp__search_finds_correct_offsets(void)
{
	git_regmatch matches[3];

	cl_git_pass(git_regexp_compile(&regex, "(a*)(b*)", 0));
	cl_git_pass(git_regexp_search(&regex, "ab", 3, matches));
	cl_assert_equal_i(matches[0].start, 0);
	cl_assert_equal_i(matches[0].end, 2);
	cl_assert_equal_i(matches[1].start, 0);
	cl_assert_equal_i(matches[1].end, 1);
	cl_assert_equal_i(matches[2].start, 1);
	cl_assert_equal_i(matches[2].end, 2);
}

void test_util_regexp__search_finds_empty_group(void)
{
	git_regmatch matches[3];

	cl_git_pass(git_regexp_compile(&regex, "(a*)(b*)c", 0));
	cl_git_pass(git_regexp_search(&regex, "ac", 3, matches));
	cl_assert_equal_i(matches[0].start, 0);
	cl_assert_equal_i(matches[0].end, 2);
	cl_assert_equal_i(matches[1].start, 0);
	cl_assert_equal_i(matches[1].end, 1);
	cl_assert_equal_i(matches[2].start, 1);
	cl_assert_equal_i(matches[2].end, 1);
}

void test_util_regexp__search_fills_matches_with_first_matching_groups(void)
{
	git_regmatch matches[2];

	cl_git_pass(git_regexp_compile(&regex, "(a)(b)(c)", 0));
	cl_git_pass(git_regexp_search(&regex, "abc", 2, matches));
	cl_assert_equal_i(matches[0].start, 0);
	cl_assert_equal_i(matches[0].end, 3);
	cl_assert_equal_i(matches[1].start, 0);
	cl_assert_equal_i(matches[1].end, 1);
}

void test_util_regexp__search_skips_nonmatching_group(void)
{
	git_regmatch matches[4];

	cl_git_pass(git_regexp_compile(&regex, "(a)(b)?(c)", 0));
	cl_git_pass(git_regexp_search(&regex, "ac", 4, matches));
	cl_assert_equal_i(matches[0].start, 0);
	cl_assert_equal_i(matches[0].end, 2);
	cl_assert_equal_i(matches[1].start, 0);
	cl_assert_equal_i(matches[1].end, 1);
	cl_assert_equal_i(matches[2].start, -1);
	cl_assert_equal_i(matches[2].end, -1);
	cl_assert_equal_i(matches[3].start, 1);
	cl_assert_equal_i(matches[3].end, 2);
}

void test_util_regexp__search_initializes_trailing_nonmatching_groups(void)
{
	git_regmatch matches[3];

	cl_git_pass(git_regexp_compile(&regex, "(a)bc", 0));
	cl_git_pass(git_regexp_search(&regex, "abc", 3, matches));
	cl_assert_equal_i(matches[0].start, 0);
	cl_assert_equal_i(matches[0].end, 3);
	cl_assert_equal_i(matches[1].start, 0);
	cl_assert_equal_i(matches[1].end, 1);
	cl_assert_equal_i(matches[2].start, -1);
	cl_assert_equal_i(matches[2].end, -1);
}
