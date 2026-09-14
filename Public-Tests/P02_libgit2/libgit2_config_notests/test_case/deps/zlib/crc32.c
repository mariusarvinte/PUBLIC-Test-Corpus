/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2022 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * This interleaved implementation of a CRC makes use of pipelined multiple
 * arithmetic-logic units, commonly found in modern CPU cores. It is due to
 * Kadatch and Jenkins (2010). See doc/crc-doc.1.0.pdf in this distribution.
 */

/* @(#) $Id$ */

#include "zutil.h" /* for Z_U4, Z_U8, z_crc_t, and FAR definitions */

/*
 A CRC of a message is computed on N braids of words in the message, where
 each word consists of W bytes (4 or 8). If N is 3, for example, then three
 running sparse CRCs are calculated respectively on each braid, at these
 indices in the array of words: 0, 3, 6, ..., 1, 4, 7, ..., and 2, 5, 8, ...
 This is done starting at a word boundary, and continues until as many blocks
 of N * W bytes as are available have been processed. The results are combined
 into a single CRC at the end. For this code, N must be in the range 1..6 and
 W must be 4 or 8. The upper limit on N can be increased if desired by adding
 more #if blocks, extending the patterns apparent in the code. In addition,
 crc32.h would need to be regenerated, if the maximum N value is increased.

 N and W are chosen empirically by benchmarking the execution time on a given
 processor. The choices for N and W below were based on testing on Intel Kaby
 Lake i7, AMD Ryzen 7, ARM Cortex-A57, Sparc64-VII, PowerPC POWER9, and MIPS64
 Octeon II processors. The Intel, AMD, and ARM processors were all fastest
 with N=5, W=8. The Sparc, PowerPC, and MIPS64 were all fastest at N=5, W=4.
 They were all tested with either gcc or clang, all using the -O3 optimization
 level. Your mileage may vary.
*/
#define N 6

/*
  Define W and the associated z_word_t type. If W is not defined, then a
  braided calculation is not used, and the associated tables and code are not
  compiled.
 */
#define W 8
typedef Z_U8 z_word_t;

/* =========================================================================
 * Tables for byte-wise and braided CRC-32 calculations, and a table of powers
 * of x for combining CRC-32s, all made by make_crc_table().
 */
#include "crc32.h"

/* CRC polynomial. */
#define POLY 0xedb88320 /* p(x) reflected, with x^32 implied */

/*
  Return a(x) multiplied by b(x) modulo p(x), where p(x) is the CRC polynomial,
  reflected. For speed, this requires that a not be zero.
 */
local z_crc_t multmodp(z_crc_t a, z_crc_t b)
{
	z_crc_t m, p;

	m = (z_crc_t)1 << 31;
	p = 0;
	for (;;) {
		if (a & m) {
			p ^= b;
			if ((a & (m - 1)) == 0)
				break;
		}
		m >>= 1;
		b = b & 1 ? (b >> 1) ^ POLY : b >> 1;
	}
	return p;
}

/*
  Return x^(n * 2^k) modulo p(x). Requires that x2n_table[] has been
  initialized.
 */
local z_crc_t x2nmodp(z_off64_t n, unsigned k)
{
	z_crc_t p;

	p = (z_crc_t)1 << 31; /* x^0 == 1 */
	while (n) {
		if (n & 1)
			p = multmodp(x2n_table[k & 31], p);
		n >>= 1;
		k++;
	}
	return p;
}

/* =========================================================================
 * This function can be used by asm versions of crc32(), and to force the
 * generation of the CRC tables in a threaded application.
 */
const z_crc_t FAR *ZEXPORT get_crc_table(void)
{
	return (const z_crc_t FAR *)crc_table;
}

/*
  Return the CRC of the W bytes in the word_t data, taking the
  least-significant byte of the word as the first byte of data, without any pre
  or post conditioning. This is used to combine the CRCs of each braid.
 */
local z_crc_t crc_word(z_word_t data)
{
	int k;
	for (k = 0; k < W; k++)
		data = (data >> 8) ^ crc_table[data & 0xff];
	return (z_crc_t)data;
}

local z_word_t crc_word_big(z_word_t data)
{
	int k;
	for (k = 0; k < W; k++)
		data = (data << 8) ^
		       crc_big_table[(data >> ((W - 1) << 3)) & 0xff];
	return data;
}

/* ========================================================================= */
unsigned long ZEXPORT
crc32_z(unsigned long crc, const unsigned char FAR *buf, z_size_t len)
{
	/* Return initial CRC, if requested. */
	if (buf == Z_NULL)
		return 0;

	/* Pre-condition the CRC */
	crc = (~crc) & 0xffffffff;

	/* If provided enough bytes, do a braided CRC calculation. */
	if (len >= N * W + W - 1) {
		z_size_t blks;
		z_word_t const *words;
		int k;

		/* Compute the CRC up to a z_word_t boundary. */
		while (len && ((z_size_t)buf & (W - 1)) != 0) {
			len--;
			crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		}

		/* Compute the CRC on as many N z_word_t blocks as are
		 * available. */
		blks = len / (N * W);
		len -= blks * N * W;
		words = (z_word_t const *)buf;

		z_crc_t crc0;
		z_word_t word0;
		z_crc_t crc1;
		z_word_t word1;
		z_crc_t crc2;
		z_word_t word2;
		z_crc_t crc3;
		z_word_t word3;
		z_crc_t crc4;
		z_word_t word4;
		z_crc_t crc5;
		z_word_t word5;

		/* Initialize the CRC for each braid. */
		crc0 = crc;
		crc1 = 0;
		crc2 = 0;
		crc3 = 0;
		crc4 = 0;
		crc5 = 0;

		/*
		  Process the first blks-1 blocks, computing the CRCs on each
		  braid independently.
		 */
		while (--blks) {
			/* Load the word for each braid into registers. */
			word0 = crc0 ^ words[0];
			word1 = crc1 ^ words[1];
			word2 = crc2 ^ words[2];
			word3 = crc3 ^ words[3];
			word4 = crc4 ^ words[4];
			word5 = crc5 ^ words[5];
			words += N;

			/* Compute and update the CRC for each word. The loop
			   should get unrolled. */
			crc0 = crc_braid_table[0][word0 & 0xff];
			crc1 = crc_braid_table[0][word1 & 0xff];
			crc2 = crc_braid_table[0][word2 & 0xff];
			crc3 = crc_braid_table[0][word3 & 0xff];
			crc4 = crc_braid_table[0][word4 & 0xff];
			crc5 = crc_braid_table[0][word5 & 0xff];
			for (k = 1; k < W; k++) {
				crc0 ^= crc_braid_table[k]
				                       [(word0 >> (k << 3)) &
				                        0xff];
				crc1 ^= crc_braid_table[k]
				                       [(word1 >> (k << 3)) &
				                        0xff];
				crc2 ^= crc_braid_table[k]
				                       [(word2 >> (k << 3)) &
				                        0xff];
				crc3 ^= crc_braid_table[k]
				                       [(word3 >> (k << 3)) &
				                        0xff];
				crc4 ^= crc_braid_table[k]
				                       [(word4 >> (k << 3)) &
				                        0xff];
				crc5 ^= crc_braid_table[k]
				                       [(word5 >> (k << 3)) &
				                        0xff];
			}
		}

		/*
		  Process the last block, combining the CRCs of the N braids at
		  the same time.
		 */
		crc = crc_word(crc0 ^ words[0]);
		crc = crc_word(crc1 ^ words[1] ^ crc);
		crc = crc_word(crc2 ^ words[2] ^ crc);
		crc = crc_word(crc3 ^ words[3] ^ crc);
		crc = crc_word(crc4 ^ words[4] ^ crc);
		crc = crc_word(crc5 ^ words[5] ^ crc);
		words += N;

		/*
		  Update the pointer to the remaining bytes to process.
		 */
		buf = (unsigned char const *)words;
	}

	/* Complete the computation of the CRC on any remaining bytes. */
	while (len >= 8) {
		len -= 8;
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
	}
	while (len) {
		len--;
		crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
	}

	/* Return the CRC, post-conditioned. */
	return crc ^ 0xffffffff;
}

/* ========================================================================= */
unsigned long ZEXPORT
crc32(unsigned long crc, const unsigned char FAR *buf, uInt len)
{
	return crc32_z(crc, buf, len);
}
