/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <git2.h>
#include "alloc.h"
#include "buf.h"
#include "common.h"
#include "filter.h"
#include "hash.h"
#include "merge_driver.h"
#include "mwindow.h"
#include "rand.h"
#include "runtime.h"
#include "settings.h"
#include "sysdir.h"
#include "git2/global.h"

int git_libgit2_init(void)
{
	static git_runtime_init_fn init_fns[] = {
		git_allocator_global_init,    git_error_global_init,
		git_rand_global_init,         git_hash_global_init,
		git_sysdir_global_init,       git_filter_global_init,
		git_merge_driver_global_init, git_mwindow_global_init,
		git_settings_global_init
	};

	return git_runtime_init(init_fns, ARRAY_SIZE(init_fns));
}

int git_libgit2_shutdown(void)
{
	return git_runtime_shutdown();
}

int git_libgit2_version(int *major, int *minor, int *rev)
{
	*major = LIBGIT2_VERSION_MAJOR;
	*minor = LIBGIT2_VERSION_MINOR;
	*rev = LIBGIT2_VERSION_REVISION;

	return 0;
}

const char *git_libgit2_prerelease(void)
{
	return LIBGIT2_VERSION_PRERELEASE;
}

int git_libgit2_features(void)
{
	return 0
#ifdef GIT_NSEC
		| GIT_FEATURE_NSEC
#endif
		| GIT_FEATURE_REGEX
		| GIT_FEATURE_COMPRESSION
		| GIT_FEATURE_SHA1
#ifdef GIT_EXPERIMENTAL_SHA256
		| GIT_FEATURE_SHA256
#endif
	;
}

const char *git_libgit2_feature_backend(git_feature_t feature)
{
	switch (feature) {
	case GIT_FEATURE_NSEC:
#if defined(GIT_NSEC)
		return "mtim";
#else
		GIT_ASSERT_WITH_RETVAL(
		        !"Unknown high-resolution time backend", NULL);
#endif
		break;


	case GIT_FEATURE_REGEX:
#if defined(GIT_REGEX_PCRE2)
		return "pcre2";
#else
		GIT_ASSERT_WITH_RETVAL(
		        !"Unknown regular expression backend", NULL);
#endif
		break;

	case GIT_FEATURE_COMPRESSION:
#if defined(GIT_COMPRESSION_ZLIB)
		return "zlib";
#elif defined(GIT_COMPRESSION_BUILTIN)
		return "builtin";
#else
		GIT_ASSERT_WITH_RETVAL(!"Unknown compression backend", NULL);
#endif
		break;

	case GIT_FEATURE_SHA1:
#if defined(GIT_SHA1_BUILTIN)
		return "builtin";
#else
		GIT_ASSERT_WITH_RETVAL(!"Unknown SHA1 backend", NULL);
#endif
		break;

	case GIT_FEATURE_SHA256:
#if defined(GIT_EXPERIMENTAL_SHA256) && defined(GIT_SHA256_BUILTIN)
		return "builtin";
#else
		GIT_ASSERT_WITH_RETVAL(!"Unknown SHA1 backend", NULL);
#endif
		break;
	}

	return NULL;
}
