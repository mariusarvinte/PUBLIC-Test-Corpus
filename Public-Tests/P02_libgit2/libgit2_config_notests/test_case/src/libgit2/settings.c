/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "settings.h"

#include <git2.h>
#include <sys/types.h>
#include "alloc.h"
#include "buf.h"
#include "cache.h"
#include "common.h"
#include "filter.h"
#include "grafts.h"
#include "hash.h"
#include "index.h"
#include "merge_driver.h"
#include "mwindow.h"
#include "object.h"
#include "odb.h"
#include "rand.h"
#include "refs.h"
#include "runtime.h"
#include "sysdir.h"
#include "git2/global.h"

/* Declarations for tuneable settings */
extern size_t git_mwindow__window_size;
extern size_t git_mwindow__mapped_limit;
extern size_t git_mwindow__file_limit;
extern size_t git_indexer__max_objects;
extern bool git_disable_pack_keep_file_checks;
extern int git_odb__packed_priority;
extern int git_odb__loose_priority;

static void settings_global_shutdown(void)
{
	git_repository__free_extensions();
}

int git_settings_global_init(void)
{
	return git_runtime_shutdown_register(settings_global_shutdown);
}

static int config_level_to_sysdir(int *out, int config_level)
{
	git_str msg = GIT_STR_INIT;

	switch (config_level) {
	case GIT_CONFIG_LEVEL_SYSTEM:
		*out = GIT_SYSDIR_SYSTEM;
		return 0;
	case GIT_CONFIG_LEVEL_XDG:
		*out = GIT_SYSDIR_XDG;
		return 0;
	case GIT_CONFIG_LEVEL_GLOBAL:
		*out = GIT_SYSDIR_GLOBAL;
		return 0;
	case GIT_CONFIG_LEVEL_PROGRAMDATA:
		*out = GIT_SYSDIR_PROGRAMDATA;
		return 0;
	default:
		break;
	}

	if (git_str_puts(&msg, "invalid config path selector ") < 0 ||
	    git_str_puti(&msg, config_level) < 0)
		git_error_set_oom();
	else
		git_error_set(GIT_ERROR_INVALID, msg.ptr);

	git_str_dispose(&msg);
	return -1;
}

/* mmap window tuning */

int git_libgit2_get_mwindow_size(size_t *out)
{
	*out = git_mwindow__window_size;
	return 0;
}

int git_libgit2_set_mwindow_size(size_t size)
{
	git_mwindow__window_size = size;
	return 0;
}

int git_libgit2_get_mwindow_mapped_limit(size_t *out)
{
	*out = git_mwindow__mapped_limit;
	return 0;
}

int git_libgit2_set_mwindow_mapped_limit(size_t limit)
{
	git_mwindow__mapped_limit = limit;
	return 0;
}

int git_libgit2_get_mwindow_file_limit(size_t *out)
{
	*out = git_mwindow__file_limit;
	return 0;
}

int git_libgit2_set_mwindow_file_limit(size_t limit)
{
	git_mwindow__file_limit = limit;
	return 0;
}

/* Sysdir-backed paths (config search / template / home) */

static int sysdir_get_to_buf(git_buf *out, int sysdir)
{
	git_str str = GIT_STR_INIT;
	const git_str *tmp;
	int error;

	if ((error = git_buf_tostr(&str, out)) < 0 ||
	    (error = git_sysdir_get(&tmp, sysdir)) < 0 ||
	    (error = git_str_put(&str, tmp->ptr, tmp->size)) < 0)
		return error;

	return git_buf_fromstr(out, &str);
}

int git_libgit2_get_search_path(int level, git_buf *out)
{
	int sysdir, error;

	if ((error = config_level_to_sysdir(&sysdir, level)) < 0)
		return error;

	return sysdir_get_to_buf(out, sysdir);
}

int git_libgit2_set_search_path(int level, const char *path)
{
	int sysdir, error;

	if ((error = config_level_to_sysdir(&sysdir, level)) < 0)
		return error;

	return git_sysdir_set(sysdir, path);
}

int git_libgit2_get_template_path(git_buf *out)
{
	return sysdir_get_to_buf(out, GIT_SYSDIR_TEMPLATE);
}

int git_libgit2_set_template_path(const char *path)
{
	return git_sysdir_set(GIT_SYSDIR_TEMPLATE, path);
}

int git_libgit2_get_homedir(git_buf *out)
{
	return sysdir_get_to_buf(out, GIT_SYSDIR_HOME);
}

int git_libgit2_set_homedir(const char *path)
{
	return git_sysdir_set(GIT_SYSDIR_HOME, path);
}

/* Object cache */

int git_libgit2_set_cache_object_limit(int type, size_t size)
{
	return git_cache_set_max_object_size((git_object_t)type, size);
}

int git_libgit2_set_cache_max_size(ssize_t max_storage_bytes)
{
	git_cache__max_storage = max_storage_bytes;
	return 0;
}

int git_libgit2_enable_caching(int enabled)
{
	git_cache__enabled = (enabled != 0);
	return 0;
}

int git_libgit2_get_cached_memory(ssize_t *current, ssize_t *allowed)
{
	*current = git_cache__current_storage;
	*allowed = git_cache__max_storage;
	return 0;
}

/* Behavioral toggles */

int git_libgit2_enable_strict_object_creation(int enabled)
{
	git_object__strict_input_validation = (enabled != 0);
	return 0;
}

int git_libgit2_enable_strict_symbolic_ref_creation(int enabled)
{
	git_reference__enable_symbolic_ref_target_validation = (enabled != 0);
	return 0;
}

int git_libgit2_enable_fsync_gitdir(int enabled)
{
	git_repository__fsync_gitdir = (enabled != 0);
	return 0;
}

int git_libgit2_enable_strict_hash_verification(int enabled)
{
	git_odb__strict_hash_verification = (enabled != 0);
	return 0;
}

int git_libgit2_enable_unsaved_index_safety(int enabled)
{
	git_index__enforce_unsaved_safety = (enabled != 0);
	return 0;
}

int git_libgit2_disable_pack_keep_file_checks(int enabled)
{
	git_disable_pack_keep_file_checks = (enabled != 0);
	return 0;
}

/* Allocator */

int git_libgit2_set_allocator(git_allocator *allocator)
{
	return git_allocator_setup(allocator);
}

/* Pack indexer */

int git_libgit2_get_pack_max_objects(size_t *out)
{
	*out = git_indexer__max_objects;
	return 0;
}

int git_libgit2_set_pack_max_objects(size_t objects)
{
	git_indexer__max_objects = objects;
	return 0;
}

/* ODB backend priorities */

int git_libgit2_set_odb_packed_priority(int priority)
{
	git_odb__packed_priority = priority;
	return 0;
}

int git_libgit2_set_odb_loose_priority(int priority)
{
	git_odb__loose_priority = priority;
	return 0;
}

/* Repository extensions */

int git_libgit2_get_extensions(git_strarray *out)
{
	char **extensions;
	size_t len;
	int error;

	if ((error = git_repository__extensions(&extensions, &len)) < 0)
		return error;

	out->strings = extensions;
	out->count = len;
	return 0;
}

int git_libgit2_set_extensions(const char **extensions, size_t len)
{
	return git_repository__set_extensions(extensions, len);
}

/* Owner validation */

int git_libgit2_get_owner_validation(int *enabled)
{
	*enabled = git_repository__validate_ownership;
	return 0;
}

int git_libgit2_set_owner_validation(int enabled)
{
	git_repository__validate_ownership = (enabled != 0);
	return 0;
}

const char *git_libgit2_buildinfo(git_buildinfo_t key)
{
	return NULL;
}
