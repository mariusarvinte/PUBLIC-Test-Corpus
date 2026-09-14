#include "clar_libgit2.h"
#include "sortedcache.h"
#include "packref.h"

void test_util_sortedcache__upsert_lookup_sort(void)
{
	git_sortedcache *sc;
	struct packref *item;
	size_t pos;

	cl_git_pass(git_sortedcache_new(&sc, NULL));

	/* insert out of order so we can check sort behavior */
	cl_git_pass(git_sortedcache_upsert(&item, sc, "aaa"));
	item->flags = 10;
	cl_git_pass(git_sortedcache_upsert(&item, sc, "bbb"));
	item->flags = 20;
	cl_git_pass(git_sortedcache_upsert(&item, sc, "zzz"));
	item->flags = 30;
	cl_git_pass(git_sortedcache_upsert(&item, sc, "mmm"));
	item->flags = 40;
	cl_git_pass(git_sortedcache_upsert(&item, sc, "iii"));
	item->flags = 50;

	cl_assert_equal_sz(5, git_sortedcache_entrycount(sc));

	/* lookup by name returns the stored data */
	cl_assert((item = git_sortedcache_lookup(sc, "aaa")) != NULL);
	cl_assert_equal_s("aaa", item->name);
	cl_assert_equal_i(10, item->flags);
	cl_assert((item = git_sortedcache_lookup(sc, "mmm")) != NULL);
	cl_assert_equal_i(40, item->flags);
	cl_assert((item = git_sortedcache_lookup(sc, "zzz")) != NULL);
	cl_assert_equal_i(30, item->flags);
	cl_assert(git_sortedcache_lookup(sc, "qqq") == NULL);

	/* entry-by-index returns items in sorted order */
	cl_assert_equal_s("aaa", git_sortedcache_entry(sc, 0)->name);
	cl_assert_equal_s("bbb", git_sortedcache_entry(sc, 1)->name);
	cl_assert_equal_s("iii", git_sortedcache_entry(sc, 2)->name);
	cl_assert_equal_s("mmm", git_sortedcache_entry(sc, 3)->name);
	cl_assert_equal_s("zzz", git_sortedcache_entry(sc, 4)->name);
	cl_assert(git_sortedcache_entry(sc, 5) == NULL);

	/* lookup_index returns position in sorted order */
	cl_git_pass(git_sortedcache_lookup_index(&pos, sc, "aaa"));
	cl_assert_equal_sz(0, pos);
	cl_git_pass(git_sortedcache_lookup_index(&pos, sc, "iii"));
	cl_assert_equal_sz(2, pos);
	cl_git_pass(git_sortedcache_lookup_index(&pos, sc, "zzz"));
	cl_assert_equal_sz(4, pos);
	cl_assert_equal_i(
	        GIT_ENOTFOUND,
	        git_sortedcache_lookup_index(&pos, sc, "xxx"));

	/* upsert of an existing key returns the existing item, not a new one */
	cl_git_pass(git_sortedcache_upsert(&item, sc, "mmm"));
	cl_assert_equal_i(40, item->flags); /* prior data preserved */
	cl_assert_equal_sz(5, git_sortedcache_entrycount(sc));

	git_sortedcache_free(sc);
}

void test_util_sortedcache__remove_and_clear(void)
{
	git_sortedcache *sc;
	struct packref *item;
	size_t pos;

	cl_git_pass(git_sortedcache_new(&sc, NULL));
	cl_git_pass(git_sortedcache_upsert(&item, sc, "alpha"));
	cl_git_pass(git_sortedcache_upsert(&item, sc, "beta"));
	cl_git_pass(git_sortedcache_upsert(&item, sc, "gamma"));

	cl_assert_equal_sz(3, git_sortedcache_entrycount(sc));

	cl_git_pass(git_sortedcache_lookup_index(&pos, sc, "beta"));
	cl_git_pass(git_sortedcache_remove(sc, pos));

	cl_assert_equal_sz(2, git_sortedcache_entrycount(sc));
	cl_assert(git_sortedcache_lookup(sc, "beta") == NULL);
	cl_assert_equal_i(
	        GIT_ENOTFOUND,
	        git_sortedcache_lookup_index(&pos, sc, "beta"));

	/* surrounding items still present and findable */
	cl_assert(git_sortedcache_lookup(sc, "alpha") != NULL);
	cl_assert(git_sortedcache_lookup(sc, "gamma") != NULL);

	cl_git_pass(git_sortedcache_clear(sc));
	cl_assert_equal_sz(0, git_sortedcache_entrycount(sc));
	cl_assert(git_sortedcache_lookup(sc, "alpha") == NULL);
	cl_assert(git_sortedcache_entry(sc, 0) == NULL);

	git_sortedcache_free(sc);
}

void test_util_sortedcache__copy(void)
{
	git_sortedcache *src, *dst;
	struct packref *item;

	cl_git_pass(git_sortedcache_new(&src, NULL));

	cl_git_pass(git_sortedcache_upsert(&item, src, "alpha"));
	item->flags = 1;
	cl_git_pass(git_sortedcache_upsert(&item, src, "beta"));
	item->flags = 2;

	cl_git_pass(git_sortedcache_copy(&dst, src));

	/* copy contains the same entries with the same data */
	cl_assert_equal_sz(2, git_sortedcache_entrycount(dst));
	cl_assert((item = git_sortedcache_lookup(dst, "alpha")) != NULL);
	cl_assert_equal_i(1, item->flags);
	cl_assert((item = git_sortedcache_lookup(dst, "beta")) != NULL);
	cl_assert_equal_i(2, item->flags);

	/* mutating the copy must not affect the original */
	item->flags = 99;
	cl_assert((item = git_sortedcache_lookup(src, "beta")) != NULL);
	cl_assert_equal_i(2, item->flags);

	git_sortedcache_free(src);
	git_sortedcache_free(dst);
}

/* Parse one whitespace-separated `flags name` per line into the cache. */
static void parse_into_cache(git_sortedcache *sc, git_str *buf)
{
	struct packref *item;
	char *scan, *eol;

	cl_git_pass(git_sortedcache_clear(sc));

	for (scan = buf->ptr; scan && *scan; scan = eol ? eol + 1 : NULL) {
		char *after;
		long flags = strtol(scan, &after, 0);
		cl_assert(after > scan);

		while (git__isspace(*after))
			after++;

		eol = strchr(after, '\n');
		if (eol)
			*eol = '\0';

		cl_git_pass(git_sortedcache_upsert(&item, sc, after));
		item->flags = (char)flags;
	}
}

/*
 * Verify the full reload cycle used by refdb_fs:
 *   - first load sees the file as out-of-date (returns 1) and parses items
 *   - immediate reload with no file change returns 0
 *   - after a rewrite, load returns 1 and re-parsed items replace the prior
 *     set in sorted order (old entries gone, new entries present)
 *
 * Rewrite content has a different length so the change is detected even
 * when mtime granularity would hide a same-second rewrite.
 */
void test_util_sortedcache__on_disk_reload(void)
{
	git_sortedcache *sc;
	git_str buf = GIT_STR_INIT;
	struct packref *item;

	cl_git_mkfile("cacheitems.txt", "10 abc\n20 bcd\n30 cde\n");
	cl_git_pass(git_sortedcache_new(&sc, "cacheitems.txt"));

	cl_assert(git_sortedcache_load(sc, &buf) > 0);
	parse_into_cache(sc, &buf);
	git_str_dispose(&buf);

	cl_assert_equal_sz(3, git_sortedcache_entrycount(sc));
	cl_assert((item = git_sortedcache_lookup(sc, "abc")) != NULL);
	cl_assert_equal_i(10, item->flags);
	cl_assert((item = git_sortedcache_lookup(sc, "cde")) != NULL);
	cl_assert_equal_i(30, item->flags);
	cl_assert_equal_s("abc", git_sortedcache_entry(sc, 0)->name);
	cl_assert_equal_s("bcd", git_sortedcache_entry(sc, 1)->name);
	cl_assert_equal_s("cde", git_sortedcache_entry(sc, 2)->name);

	/* immediate reload sees no change */
	cl_assert_equal_i(0, git_sortedcache_load(sc, NULL));

	/* rewrite the backing file with different entries (and different size) */
	cl_git_rewritefile(
	        "cacheitems.txt", "100 abc\n200 zzz\n500 aaa\n10 final\n");
	cl_assert(git_sortedcache_load(sc, &buf) > 0);
	parse_into_cache(sc, &buf);
	git_str_dispose(&buf);

	/* old entries are gone, new entries are present in sorted order */
	cl_assert_equal_sz(4, git_sortedcache_entrycount(sc));
	cl_assert(git_sortedcache_lookup(sc, "bcd") == NULL);
	cl_assert(git_sortedcache_lookup(sc, "cde") == NULL);
	cl_assert((item = git_sortedcache_lookup(sc, "abc")) != NULL);
	cl_assert_equal_i(100, item->flags);
	cl_assert((item = git_sortedcache_lookup(sc, "final")) != NULL);
	cl_assert_equal_i(10, item->flags);
	cl_assert_equal_s("aaa", git_sortedcache_entry(sc, 0)->name);
	cl_assert_equal_s("abc", git_sortedcache_entry(sc, 1)->name);
	cl_assert_equal_s("final", git_sortedcache_entry(sc, 2)->name);
	cl_assert_equal_s("zzz", git_sortedcache_entry(sc, 3)->name);

	git_sortedcache_free(sc);
}
