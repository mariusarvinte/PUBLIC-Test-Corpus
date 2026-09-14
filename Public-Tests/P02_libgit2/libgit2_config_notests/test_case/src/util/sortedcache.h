/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_sorted_cache_h__
#define INCLUDE_sorted_cache_h__

#include "git2_util.h"

#include "util.h"
#include "futils.h"
#include "vector.h"
#include "hashmap_str.h"

#include <stddef.h>

/* defined in libgit2/packref.h */
struct packref;

/*
 * Cache of `struct packref` items parsed from a backing file.  Items can
 * be looked up by name and traversed in sorted order.  The cache owns its
 * items and frees them when removed or when the cache is freed.
 *
 * This used to be a generic container parameterized by an item layout
 * offset.  It is now specialized to `struct packref` so the data
 * structure carries a real item type rather than `void *` plus an offset.
 */

typedef struct {
	git_refcount rc;
	git_vector items;
	git_hashmap_str map;
	git_futils_filestamp stamp;
	char path[GIT_FLEX_ARRAY];
} git_sortedcache;

/* Create a new sortedcache.
 *
 * @param out The allocated git_sortedcache
 * @param path The path to the backing store file for this cache; this
 *        may be NULL.  The cache makes it easy to load this and check
 *        if it has been modified since the last load and/or write.
 */
GIT_WARN_UNUSED_RESULT int git_sortedcache_new(
        git_sortedcache **out,
        const char *path);

/* Copy a sorted cache.  Items are deep-copied (a fresh packref is
 * allocated for each entry).
 */
GIT_WARN_UNUSED_RESULT int git_sortedcache_copy(
        git_sortedcache **out,
        git_sortedcache *src);

/* Free sorted cache (and all packrefs it owns) */
void git_sortedcache_free(git_sortedcache *sc);

/* Increment reference count - balance with call to free */
void git_sortedcache_incref(git_sortedcache *sc);

/* Get the pathname associated with this cache at creation time */
const char *git_sortedcache_path(git_sortedcache *sc);

/*
 * CACHE WRITE FUNCTIONS
 */

/* Load backing file into a buffer.
 *
 * This looks at the modification time and size of the file on disk.
 *
 * If the file appears to have changed, this loads the file contents into
 * the buffer and returns a positive value - the caller should parse the
 * file content, update the cache as needed
 *
 * If the file appears to be unchanged, then this clears the buffer, and returns
 * 0.
 *
 * @return 0 if up-to-date, 1 if out-of-date, <0 on error
 */
GIT_WARN_UNUSED_RESULT int
git_sortedcache_load(git_sortedcache *sc, git_str *buf);

/* Refresh file timestamp after write completes
 */
void git_sortedcache_updated(git_sortedcache *sc);

/* Release all items in sorted cache */
GIT_WARN_UNUSED_RESULT int git_sortedcache_clear(git_sortedcache *sc);

/* Find and/or insert an item, returning pointer to packref. */
GIT_WARN_UNUSED_RESULT int
git_sortedcache_upsert(struct packref **out, git_sortedcache *sc, const char *key);

/* Removes entry at pos from cache */
int git_sortedcache_remove(git_sortedcache *sc, size_t pos);

/*
 * CACHE READ FUNCTIONS
 */

/* Lookup item by key - returns NULL if not found */
struct packref *git_sortedcache_lookup(git_sortedcache *sc, const char *key);

/* Get how many items are in the cache */
size_t git_sortedcache_entrycount(const git_sortedcache *sc);

/* Lookup item by index - returns NULL if out of range */
struct packref *git_sortedcache_entry(git_sortedcache *sc, size_t pos);

/* Lookup index of item by key - returns GIT_ENOTFOUND if not found */
int git_sortedcache_lookup_index(
        size_t *out,
        git_sortedcache *sc,
        const char *key);

#endif
