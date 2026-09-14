/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "failalloc.h"

void *git_failalloc_malloc(size_t len)
{
	GIT_UNUSED(len);

	return NULL;
}

void *git_failalloc_realloc(void *ptr, size_t size)
{
	GIT_UNUSED(ptr);
	GIT_UNUSED(size);

	return NULL;
}

void git_failalloc_free(void *ptr)
{
	GIT_UNUSED(ptr);
}
