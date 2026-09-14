/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "sortedcache.h"
#include "hashmap.h"
#include "../libgit2/packref.h"

static int packref_cmp(const void *a_, const void *b_)
{
	const struct packref *a = a_, *b = b_;
	return strcmp(a->name, b->name);
}

int git_sortedcache_new(git_sortedcache **out, const char *path)
{
	git_sortedcache *sc;
	size_t pathlen, alloclen;

	pathlen = path ? strlen(path) : 0;

	GIT_ERROR_CHECK_ALLOC_ADD(&alloclen, sizeof(git_sortedcache), pathlen);
	GIT_ERROR_CHECK_ALLOC_ADD(&alloclen, alloclen, 1);
	sc = git__calloc(1, alloclen);
	GIT_ERROR_CHECK_ALLOC(sc);

	if (git_vector_init(&sc->items, 4, packref_cmp) < 0)
		goto fail;

	GIT_REFCOUNT_INC(sc);
	if (pathlen)
		memcpy(sc->path, path, pathlen);

	*out = sc;
	return 0;

fail:
	git_vector_dispose(&sc->items);
	git__free(sc);
	return -1;
}

void git_sortedcache_incref(git_sortedcache *sc)
{
	GIT_REFCOUNT_INC(sc);
}

const char *git_sortedcache_path(git_sortedcache *sc)
{
	return sc->path;
}

static void sortedcache_clear(git_sortedcache *sc)
{
	git_hashmap_str_clear(&sc->map);

	/* dispose_deep frees each packref allocation owned by the vector */
	git_vector_dispose_deep(&sc->items);
	(void)git_vector_init(&sc->items, 4, sc->items._cmp);
}

static void sortedcache_free(git_sortedcache *sc)
{
	sortedcache_clear(sc);
	git_vector_dispose(&sc->items);
	git_hashmap_str_dispose(&sc->map);

	git__free(sc);
}

void git_sortedcache_free(git_sortedcache *sc)
{
	if (!sc)
		return;
	GIT_REFCOUNT_DEC(sc, sortedcache_free);
}

/* copy a sorted cache */
int git_sortedcache_copy(git_sortedcache **out, git_sortedcache *src)
{
	int error = 0;
	git_sortedcache *tgt;
	size_t i;
	struct packref *src_item, *tgt_item;

	if ((error = git_sortedcache_new(&tgt, src->path)) < 0)
		return error;

	git_vector_foreach (&src->items, i, src_item) {
		if ((error = git_sortedcache_upsert(
		             &tgt_item, tgt, src_item->name)) < 0)
			break;

		/* name was already copied by upsert; copy the rest of the
		 * packref fields explicitly */
		tgt_item->oid = src_item->oid;
		tgt_item->peel = src_item->peel;
		tgt_item->flags = src_item->flags;
	}

	if (error)
		git_sortedcache_free(tgt);

	*out = !error ? tgt : NULL;

	return error;
}

/* if the file has changed, load file contents into buf;
 * returns <0 on error, >0 if file has not changed
 */
int git_sortedcache_load(git_sortedcache *sc, git_str *buf)
{
	int error, fd;
	struct stat st;

	if ((error = git_futils_filestamp_check(&sc->stamp, sc->path)) <= 0)
		return error;

	if ((fd = git_futils_open_ro(sc->path)) < 0) {
		error = fd;
		return error;
	}

	if (p_fstat(fd, &st) < 0) {
		git_error_set(GIT_ERROR_OS, "failed to stat file");
		error = -1;
		(void)p_close(fd);
		return error;
	}

	if (!git__is_sizet(st.st_size)) {
		git_error_set(
		        GIT_ERROR_INVALID,
		        "unable to load file larger than size_t");
		error = -1;
		(void)p_close(fd);
		return error;
	}

	if (buf)
		error = git_futils_readbuffer_fd(buf, fd, (size_t)st.st_size);

	(void)p_close(fd);

	if (error < 0)
		return error;

	return 1; /* return 1 -> file needs reload and was successfully loaded
	           */
}

void git_sortedcache_updated(git_sortedcache *sc)
{
	/* update filestamp to latest value */
	git_futils_filestamp_check(&sc->stamp, sc->path);
}

/* release all items in sorted cache */
int git_sortedcache_clear(git_sortedcache *sc)
{
	sortedcache_clear(sc);
	return 0;
}

/* find and/or insert item, returning pointer to packref */
int git_sortedcache_upsert(struct packref **out, git_sortedcache *sc, const char *key)
{
	int error = 0;
	struct packref *item;
	size_t namelen, alloclen;

	if (git_hashmap_str_get((void **)&item, &sc->map, key) == 0)
		goto done;

	namelen = strlen(key);
	GIT_ERROR_CHECK_ALLOC_ADD(&alloclen, sizeof(struct packref), namelen);
	GIT_ERROR_CHECK_ALLOC_ADD(&alloclen, alloclen, 1);

	item = git__calloc(1, alloclen);
	if (!item) {
		error = -1;
		goto done;
	}

	memcpy(item->name, key, namelen);

	if ((error = git_hashmap_str_put(&sc->map, item->name, item)) < 0) {
		git__free(item);
		goto done;
	}

	if ((error = git_vector_insert(&sc->items, item)) < 0) {
		git_hashmap_str_remove(&sc->map, item->name);
		git__free(item);
	}

done:
	if (out)
		*out = !error ? item : NULL;
	return error;
}

/* lookup item by key */
struct packref *git_sortedcache_lookup(git_sortedcache *sc, const char *key)
{
	struct packref *value;

	return git_hashmap_str_get((void **)&value, &sc->map, key) == 0 ? value
	                                                                : NULL;
}

/* find out how many items are in the cache */
size_t git_sortedcache_entrycount(const git_sortedcache *sc)
{
	return git_vector_length(&sc->items);
}

/* lookup item by index */
struct packref *git_sortedcache_entry(git_sortedcache *sc, size_t pos)
{
	/* make sure the items are sorted so this gets the correct item */
	if (!git_vector_is_sorted(&sc->items))
		git_vector_sort(&sc->items);

	return git_vector_get(&sc->items, pos);
}

static int sortedcache_search_cmp(const void *key, const void *value)
{
	const char *needle = key;
	const struct packref *item = value;
	return strcmp(needle, item->name);
}

/* lookup index of item by key */
int git_sortedcache_lookup_index(
        size_t *out,
        git_sortedcache *sc,
        const char *key)
{
	return git_vector_bsearch2(
	        out, &sc->items, sortedcache_search_cmp, key);
}

/* remove entry from cache */
int git_sortedcache_remove(git_sortedcache *sc, size_t pos)
{
	struct packref *item;

	if ((item = git_vector_get(&sc->items, pos)) == NULL) {
		git_error_set(GIT_ERROR_INVALID, "removing item out of range");
		return GIT_ENOTFOUND;
	}

	(void)git_vector_remove(&sc->items, pos);

	git_hashmap_str_remove(&sc->map, item->name);

	git__free(item);

	return 0;
}
