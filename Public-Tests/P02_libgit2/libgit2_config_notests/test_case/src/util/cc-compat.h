/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_cc_compat_h__
#define INCLUDE_cc_compat_h__

#include <stdarg.h>

/* Default to safer but a bit wasteful traditional style */
#ifndef GIT_FLEX_ARRAY
#	define GIT_FLEX_ARRAY 1
#endif

#define GIT_ALIGN(x,size) x

#define GIT_UNUSED(x) ((void)(x))
#define GIT_UNUSED_ARG
#define GIT_UNUSED_FUNCTION

/* Define the printf format specifier to use for size_t output */
#define PRIuZ "zu"
#define PRIxZ "zx"
#define PRIXZ "zX"
#define PRIdZ "zd"

#include <stdbool.h>

#endif
