/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "git2_util.h"
#include "runtime.h"

static git_runtime_shutdown_fn shutdown_callback[32];
static int shutdown_callback_count;

static int init_count;

static int init_common(git_runtime_init_fn init_fns[], size_t cnt)
{
	size_t i;
	int ret;

	/* Initialize subsystems that have global state */
	for (i = 0; i < cnt; i++) {
		if ((ret = init_fns[i]()) != 0)
			break;
	}

	return ret;
}

static void shutdown_common(void)
{
	git_runtime_shutdown_fn cb;
	int pos;

	for (pos = shutdown_callback_count;
	     pos > 0;
	     pos = --shutdown_callback_count) {
		cb = git_ptr_swap(shutdown_callback[pos - 1], NULL);

		if (cb != NULL)
			cb();
	}
}

int git_runtime_shutdown_register(git_runtime_shutdown_fn callback)
{
	int count = ++shutdown_callback_count;

	if (count > (int)ARRAY_SIZE(shutdown_callback) || count == 0) {
		git_error_set(GIT_ERROR_INVALID,
		              "too many shutdown callbacks registered");
		--shutdown_callback_count;
		return -1;
	}

	shutdown_callback[count - 1] = callback;

	return 0;
}


int git_runtime_init(git_runtime_init_fn init_fns[], size_t cnt)
{
	int ret;

	/* Only do work on a 0 -> 1 transition of the refcount */
	if ((ret = ++init_count) == 1) {
		if (init_common(init_fns, cnt) < 0)
			ret = -1;
	}

	return ret;
}

int git_runtime_init_count(void)
{
	int ret;

	ret = init_count;

	return ret;
}

int git_runtime_shutdown(void)
{
	int ret;

	/* Only do work on a 1 -> 0 transition of the refcount */
	if ((ret = --init_count) == 0)
		shutdown_common();

	return ret;
}
