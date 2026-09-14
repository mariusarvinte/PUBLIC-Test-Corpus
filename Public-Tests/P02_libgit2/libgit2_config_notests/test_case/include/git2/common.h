/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_git_common_h__
#define INCLUDE_git_common_h__

#include <time.h>
#include <stdlib.h>
#include <inttypes.h>

/** Start declarations in C mode */
#define GIT_BEGIN_DECL /* empty */
/** End declarations in C mode */
#define GIT_END_DECL /* empty */

/** Declare a public function exported for application use. */
#define GIT_EXTERN(type) extern type

/** Declare a callback function for application use. */
#define GIT_CALLBACK(name) (*name)

/** Declare a function as deprecated. */
#define GIT_DEPRECATED(func) func

/** Declare a function's takes printf style arguments. */
#define GIT_FORMAT_PRINTF(a, b) /* empty */

/**
 * @file git2/common.h
 * @brief Base platform functionality
 * @defgroup git_common Git common platform definitions
 * @ingroup Git
 *
 * Common platform functionality including introspecting libgit2
 * itself - information like how it was built, and the current
 * running version.
 * @{
 */

GIT_BEGIN_DECL

#define GIT_PATH_LIST_SEPARATOR ':'

/**
 * The maximum length of a valid git path.
 */
#define GIT_PATH_MAX 4096

/**
 * Return the version of the libgit2 library
 * being currently used.
 *
 * @param major Store the major version number
 * @param minor Store the minor version number
 * @param rev Store the revision (patch) number
 * @return 0 on success or an error code on failure
 */
GIT_EXTERN(int) git_libgit2_version(int *major, int *minor, int *rev);

/**
 * Return the prerelease state of the libgit2 library currently being
 * used.  For nightly builds during active development, this will be
 * "alpha".  Releases may have a "beta" or release candidate ("rc1",
 * "rc2", etc) prerelease.  For a final release, this function returns
 * NULL.
 *
 * @return the name of the prerelease state or NULL
 */
GIT_EXTERN(const char *) git_libgit2_prerelease(void);

/**
 * Configurable features of libgit2; either optional settings, or
 * features that can be enabled by one of a number of different backend
 * "providers"
 */
typedef enum {
	/** Sub-second resolution in index timestamps */
	GIT_FEATURE_NSEC = (1 << 3),

	/** Regular expression support; always available */
	GIT_FEATURE_REGEX = (1 << 5),

	/** zlib support; always available */
	GIT_FEATURE_COMPRESSION = (1 << 9),

	/** SHA1 object support; always available */
	GIT_FEATURE_SHA1 = (1 << 10),

	/** SHA256 object support */
	GIT_FEATURE_SHA256 = (1 << 11)
} git_feature_t;

/**
 * Query compile time options for libgit2.
 *
 * @return A combination of GIT_FEATURE_* values.
 */
GIT_EXTERN(int) git_libgit2_features(void);

/**
 * Query the backend details for the compile-time feature in libgit2.
 *
 * This will return the "backend" for the feature
 *
 * The backend _may_ be "builtin" for
 * features that are provided by libgit2 itself.
 *
 * If the feature is not supported by the library, this API returns
 * `NULL`.
 *
 * @param feature the feature to query details for
 * @return the provider details, or NULL if the feature is not supported
 */
GIT_EXTERN(const char *) git_libgit2_feature_backend(git_feature_t feature);

/* Forward declarations for global option argument types. */
struct git_buf;
struct git_strarray;
struct git_allocator;

/**
 * Global library options.
 *
 * Each setting has a typed getter and/or setter below. Functions return
 * 0 on success and a negative value on failure unless otherwise noted.
 *
 * `level` arguments are `git_config_level_t` values; `type` is a
 * `git_object_t`. They are declared `int` here to avoid pulling in
 * additional headers.
 */

/* mmap window tuning */
GIT_EXTERN(int) git_libgit2_get_mwindow_size(size_t *out);
GIT_EXTERN(int) git_libgit2_set_mwindow_size(size_t size);
GIT_EXTERN(int) git_libgit2_get_mwindow_mapped_limit(size_t *out);
GIT_EXTERN(int) git_libgit2_set_mwindow_mapped_limit(size_t limit);
GIT_EXTERN(int) git_libgit2_get_mwindow_file_limit(size_t *out);
GIT_EXTERN(int) git_libgit2_set_mwindow_file_limit(size_t limit);

/* Config / template / home search paths */
GIT_EXTERN(int) git_libgit2_get_search_path(int level, struct git_buf *out);
GIT_EXTERN(int) git_libgit2_set_search_path(int level, const char *path);
GIT_EXTERN(int) git_libgit2_get_template_path(struct git_buf *out);
GIT_EXTERN(int) git_libgit2_set_template_path(const char *path);
GIT_EXTERN(int) git_libgit2_get_homedir(struct git_buf *out);
GIT_EXTERN(int) git_libgit2_set_homedir(const char *path);

/* Object cache */
GIT_EXTERN(int) git_libgit2_set_cache_object_limit(int type, size_t size);
GIT_EXTERN(int) git_libgit2_set_cache_max_size(ssize_t max_storage_bytes);
GIT_EXTERN(int) git_libgit2_enable_caching(int enabled);
GIT_EXTERN(int) git_libgit2_get_cached_memory(ssize_t *current, ssize_t *allowed);

/* Behavioral toggles */
GIT_EXTERN(int) git_libgit2_enable_strict_object_creation(int enabled);
GIT_EXTERN(int) git_libgit2_enable_strict_symbolic_ref_creation(int enabled);
GIT_EXTERN(int) git_libgit2_enable_fsync_gitdir(int enabled);
GIT_EXTERN(int) git_libgit2_enable_strict_hash_verification(int enabled);
GIT_EXTERN(int) git_libgit2_enable_unsaved_index_safety(int enabled);
GIT_EXTERN(int) git_libgit2_disable_pack_keep_file_checks(int enabled);

/* Allocator */
GIT_EXTERN(int) git_libgit2_set_allocator(struct git_allocator *allocator);

/* Pack indexer */
GIT_EXTERN(int) git_libgit2_get_pack_max_objects(size_t *out);
GIT_EXTERN(int) git_libgit2_set_pack_max_objects(size_t objects);

/* ODB backend priorities */
GIT_EXTERN(int) git_libgit2_set_odb_packed_priority(int priority);
GIT_EXTERN(int) git_libgit2_set_odb_loose_priority(int priority);

/* Repository extensions */
GIT_EXTERN(int) git_libgit2_get_extensions(struct git_strarray *out);
GIT_EXTERN(int) git_libgit2_set_extensions(const char **extensions, size_t len);

/* Owner validation */
GIT_EXTERN(int) git_libgit2_get_owner_validation(int *enabled);
GIT_EXTERN(int) git_libgit2_set_owner_validation(int enabled);


/**
 * Build information items to query. This is the information type
 * passed to `git_libgit2_buildinfo` to get particular information
 * about the libgit2 build.
 */
typedef enum {
	/** The CPU type that libgit2 was built for. */
	GIT_BUILDINFO_CPU = 1,

	/** The commit that libgit2 was built from. */
	GIT_BUILDINFO_COMMIT
} git_buildinfo_t;

/**
 * Query information about the compile-time information about
 * libgit2.
 *
 * @param info the build information to query
 * @return the requested information, or `NULL` on error
 */
GIT_EXTERN(const char *) git_libgit2_buildinfo(git_buildinfo_t info);

/** @} */
GIT_END_DECL

#endif
