/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_trace_h__
#define INCLUDE_trace_h__

#include "common.h"

#include <git2/trace.h>
#include "str.h"

struct git_trace_data {
	git_trace_level_t level;
	git_trace_cb callback;
};

extern struct git_trace_data git_trace__data;

#define git_trace_level() (git_trace__data.level)

GIT_INLINE(void) git_trace(git_trace_level_t level, const char *message)
{
	if (git_trace__data.level >= level && git_trace__data.callback != NULL)
		git_trace__data.callback(level, message);
}

#endif
