/* zutil.h -- internal interface and configuration of the compression library
 * Copyright (C) 1995-2024 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id$ */

#ifndef ZUTIL_H
#define ZUTIL_H

#define ZLIB_INTERNAL

#include "zlib.h"

#include <string.h>
#include <stdlib.h>

#ifndef local
#define local static
#endif
/* since "static" is used to mean two completely different things in C, we
   define "local" for the non-static meaning of "static", for readability
   (compile with -Dlocal if your debugger can't find static symbols) */

typedef unsigned char uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long ulg;

#define Z_U8 unsigned long

extern z_const char *const z_errmsg[10]; /* indexed by 2-zlib_error */
/* (size given to avoid silly warnings with Visual C++) */

#define ERR_MSG(err) z_errmsg[(err) < -6 || (err) > 2 ? 9 : 2 - (err)]

#define ERR_RETURN(strm, err) return (strm->msg = ERR_MSG(err), (err))
/* To be used only when the state is known to be valid */

/* common constants */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES 2
/* The three kinds of block type */

#define MIN_MATCH 3
#define MAX_MATCH 258
/* The minimum and maximum match lengths */

#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header */

/* common defaults */

#define OS_CODE 3 /* assume Unix */

#define F_OPEN(name, mode) fopen((name), (mode))

/* functions */

#define zmemcpy memcpy
#define zmemcmp memcmp
#define zmemzero(dest, len) memset(dest, 0, len)

voidpf ZLIB_INTERNAL zcalloc(voidpf opaque, unsigned items, unsigned size);
void ZLIB_INTERNAL zcfree(voidpf opaque, voidpf ptr);

#define ZALLOC(strm, items, size) \
	(*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr) (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
#define TRY_FREE(s, p)               \
	{                            \
		if (p)               \
			ZFREE(s, p); \
	}

/* Reverse the bytes in a 32-bit value */
#define ZSWAP32(q)                                      \
	((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
	 (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

#endif /* ZUTIL_H */
