/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_ctype_compat_h__
#define INCLUDE_ctype_compat_h__

# define git__tolower(a) tolower((unsigned char)(a))
# define git__toupper(a) toupper((unsigned char)(a))

# define git__isalpha(a)  (!!isalpha((unsigned char)(a)))
# define git__isdigit(a)  (!!isdigit((unsigned char)(a)))
# define git__isalnum(a)  (!!isalnum((unsigned char)(a)))
# define git__isspace(a)  (!!isspace((unsigned char)(a)))
# define git__isxdigit(a) (!!isxdigit((unsigned char)(a)))
# define git__isprint(a)  (!!isprint((unsigned char)(a)))

#endif
