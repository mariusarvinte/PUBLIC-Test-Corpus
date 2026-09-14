/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-2024 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#ifndef ZCONF_H
#define ZCONF_H

#define z_const const
typedef unsigned long z_size_t;

/* Maximum value for memLevel in deflateInit2 */
#define MAX_MEM_LEVEL 9

/* Maximum value for windowBits in deflateInit2 and inflateInit2. */
#define MAX_WBITS 15 /* 32K LZ77 window */

/* Type declarations */

#define ZEXTERN extern
#define ZEXPORT
#define FAR

typedef unsigned char Byte; /* 8 bits */
typedef unsigned int uInt; /* 16 bits or more */
typedef unsigned long uLong; /* 32 bits or more */

typedef Byte FAR Bytef;
typedef char FAR charf;
typedef int FAR intf;
typedef uInt FAR uIntf;
typedef uLong FAR uLongf;

typedef void const *voidpc;
typedef void FAR *voidpf;
typedef void *voidp;

typedef unsigned z_crc_t;

#define z_off_t long
#define z_off64_t z_off_t

#endif /* ZCONF_H */
