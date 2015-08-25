/*
 * bn32.c - the high-level bignum interface
 *
 * WARNING: This file was automatically generated from bn16.c -- DO NOT EDIT!
 *
 * Like bni32.c, this reserves the string "32" for textual replacement.
 * The string must not appear anywhere unless it is intended to be replaced
 * to generate other bignum interface functions.
 *
 * Written by Colin Plumb
 *
 * $Id: bn32.c,v 1.5 1999/02/14 11:20:48 wprice Exp $
 */

/*
 * Some compilers complain about #if FOO if FOO isn't defined,
 * so do the ANSI-mandated thing explicitly...
 */
#ifndef NO_STRING_H
#define NO_STRING_H 0
#endif
#ifndef HAVE_STRINGS_H
#define HAVE_STRINGS_H 0
#endif
#ifndef NEED_MEMORY_H
#define NEED_MEMORY_H 0
#endif

#define MALLOCDB

#if NEED_MEMORY_H
#include <memory.h>
#endif

#include <assert.h>

#ifdef PGP_MACINTOSH
#include <string.h>
#endif

#ifdef PGP_WIN32
#define pgpAssert assert
#else
#include "pgpDebug.h"
#endif


#include "bni.h"
#include "bni32.h"
#include "bnimem.h"
#include "bn32.h"
#include "bn.h"

/* Work-arounds for some particularly broken systems */
#include "bnkludge.h"	/* For memmove() */


/* Functions */
void
bnInit_32(void)
{
	bnEnd = bnEnd_32;
	bnPrealloc = bnPrealloc_32;
	bnCopy = bnCopy_32;
	bnNorm = bnNorm_32;
	bnExtractBigBytes = bnExtractBigBytes_32;
	bnInsertBigBytes = bnInsertBigBytes_32;
	bnExtractLittleBytes = bnExtractLittleBytes_32;
	bnInsertLittleBytes = bnInsertLittleBytes_32;
	bnLSWord = bnLSWord_32;
	bnBits = bnBits_32;
	bnAdd = bnAdd_32;
	bnSub = bnSub_32;
	bnCmpQ = bnCmpQ_32;
	bnSetQ = bnSetQ_32;
	bnAddQ = bnAddQ_32;
	bnSubQ = bnSubQ_32;
	bnCmp = bnCmp_32;
	bnSquare = bnSquare_32;
	bnMul = bnMul_32;
	bnMulQ = bnMulQ_32;
	bnDivMod = bnDivMod_32;
	bnMod = bnMod_32;
	bnModQ = bnModQ_32;
	bnExpMod = bnExpMod_32;
	bnDoubleExpMod = bnDoubleExpMod_32;
	bnTwoExpMod = bnTwoExpMod_32;
	bnGcd = bnGcd_32;
	bnInv = bnInv_32;
	bnLShift = bnLShift_32;
	bnRShift = bnRShift_32;
	bnMakeOdd = bnMakeOdd_32;
}

void
bnEnd_32(BigNum *bn)
{
	if (bn->ptr) {
		BNIFREE((BNWORD32 *)bn->ptr, bn->allocated);
		bn->ptr = 0;
	}
	bn->size = 0;
	bn->allocated = 0;

}

/* Internal function.  It operates in words. */
static int
bnResize_32(BigNum *bn, unsigned len)
{
	void *p;

	/* Round size up: most mallocs impose 8-byte granularity anyway */
	len = (len + (8/sizeof(BNWORD32) - 1)) & ~(8/sizeof(BNWORD32) - 1);
	p = BNIREALLOC((BNWORD32 *)bn->ptr, bn->allocated, len);
	if (!p)
		return -1;
	bn->ptr = p;
	bn->allocated = len;


	return 0;
}

#define bnSizeCheck(bn, size) \
	if (bn->allocated < size && bnResize_32(bn, size) < 0) \
		return -1

int
bnPrealloc_32(BigNum *bn, unsigned bits)
{
	bits = (bits + 32-1)/32;
	bnSizeCheck(bn, bits);
	MALLOCDB;
	return 0;
}

int
bnCopy_32(BigNum *dest, BigNum const *src)
{
	bnSizeCheck(dest, src->size);
	dest->size = src->size;
	bniCopy_32((BNWORD32 *)dest->ptr, (BNWORD32 *)src->ptr, src->size);
	return 0;
}

void
bnNorm_32(BigNum *bn)
{
	bn->size = bniNorm_32((BNWORD32 *)bn->ptr, bn->size);
}

/*
 * Convert a bignum to big-endian bytes.  Returns, in big-endian form, a
 * substring of the bignum starting from lsbyte and "len" bytes long.
 * Unused high-order (leading) bytes are filled with 0.
 */
void
bnExtractBigBytes_32(BigNum const *bn, unsigned char *dest,
                  unsigned lsbyte, unsigned len)
{
	unsigned s = bn->size * (32 / 8);

	/* Fill unused leading bytes with 0 */
	while (s < lsbyte+len && len) {
		*dest++ = 0;
		len--;
	}

	if (len)
		bniExtractBigBytes_32((BNWORD32 *)bn->ptr, dest, lsbyte, len);
}

int
bnInsertBigBytes_32(BigNum *bn, unsigned char const *src,
                 unsigned lsbyte, unsigned len)
{
	unsigned s = bn->size;
	unsigned words = (len+lsbyte+sizeof(BNWORD32)-1) / sizeof(BNWORD32);

	/* Pad with zeros as required */
	bnSizeCheck(bn, words);

	if (s < words) {
		bniZero_32((BNWORD32 *)bn->ptr BIGLITTLE(-s,+s), words-s);
		s = words;
	}

	bniInsertBigBytes_32((BNWORD32 *)bn->ptr, src, lsbyte, len);

	bn->size = bniNorm_32((BNWORD32 *)bn->ptr, s);

	return 0;
}


/*
 * Convert a bignum to little-endian bytes.  Returns, in little-endian form, a
 * substring of the bignum starting from lsbyte and "len" bytes long.
 * Unused high-order (trailing) bytes are filled with 0.
 */
void
bnExtractLittleBytes_32(BigNum const *bn, unsigned char *dest,
                  unsigned lsbyte, unsigned len)
{
	unsigned s = bn->size * (32 / 8);

	/* Fill unused leading bytes with 0 */
	while (s < lsbyte+len && len)
		dest[--len] = 0;

	if (len)
		bniExtractLittleBytes_32((BNWORD32 *)bn->ptr, dest,
		                         lsbyte, len);
}

int
bnInsertLittleBytes_32(BigNum *bn, unsigned char const *src,
                       unsigned lsbyte, unsigned len)
{
	unsigned s = bn->size;
	unsigned words = (len+lsbyte+sizeof(BNWORD32)-1) / sizeof(BNWORD32);

	/* Pad with zeros as required */
	bnSizeCheck(bn, words);

	if (s < words) {
		bniZero_32((BNWORD32 *)bn->ptr BIGLITTLE(-s,+s), words-s);
		s = words;
	}

	bniInsertLittleBytes_32((BNWORD32 *)bn->ptr, src, lsbyte, len);

	bn->size = bniNorm_32((BNWORD32 *)bn->ptr, s);

	return 0;
}

/* Return the least-significant word of the input. */
unsigned
bnLSWord_32(BigNum const *src)
{
	return src->size ? (unsigned)((BNWORD32 *)src->ptr)[BIGLITTLE(-1,0)]
			 : 0;
}

unsigned
bnBits_32(BigNum const *src)
{
	return bniBits_32((BNWORD32 *)src->ptr, src->size);
}

int
bnAdd_32(BigNum *dest, BigNum const *src)
{
	unsigned s = src->size, d = dest->size;
	BNWORD32 t;

	if (!s)
		return 0;

	bnSizeCheck(dest, s);

	if (d < s) {
		bniZero_32((BNWORD32 *)dest->ptr BIGLITTLE(-d,+d), s-d);
		dest->size = d = s;
	}
	t = bniAddN_32((BNWORD32 *)dest->ptr, (BNWORD32 *)src->ptr, s);
	MALLOCDB;
	if (t) {
		if (d > s) {
			t = bniAdd1_32((BNWORD32 *)dest->ptr BIGLITTLE(-s,+s),
			               d-s, t);
			MALLOCDB;
		}
		if (t) {
			bnSizeCheck(dest, d+1);
			((BNWORD32 *)dest->ptr)[BIGLITTLE(-1-d,d)] = t;
			dest->size = d+1;
		}
	}
	return 0;
}

/*
 * dest -= src.
 * If dest goes negative, this produces the absolute value of
 * the difference (the negative of the true value) and returns 1.
 * Otherwise, it returls 0.
 */
int
bnSub_32(BigNum *dest, BigNum const *src)
{
	unsigned s = src->size, d = dest->size;
	BNWORD32 t;

	if (d < s  &&  d < (s = bniNorm_32((BNWORD32 *)src->ptr, s))) {
		bnSizeCheck(dest, s);
		bniZero_32((BNWORD32 *)dest->ptr BIGLITTLE(-d,+d), s-d);
		dest->size = d = s;
		MALLOCDB;
	}
	if (!s)
		return 0;
	t = bniSubN_32((BNWORD32 *)dest->ptr, (BNWORD32 *)src->ptr, s);
	MALLOCDB;
	if (t) {
		if (d > s) {
			t = bniSub1_32((BNWORD32 *)dest->ptr BIGLITTLE(-s,+s),
			               d-s, t);
			MALLOCDB;
		}
		if (t) {
			bniNeg_32((BNWORD32 *)dest->ptr, d);
			dest->size = bniNorm_32((BNWORD32 *)dest->ptr,
			                        dest->size);
			MALLOCDB;
			return 1;
		}
	}
	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, dest->size);
	return 0;
}

/*
 * Compare the BigNum to the given value, which must be < 65536.
 * Returns -1. 0 or 1 if a<b, a == b or a>b.
 * a <=> b --> bnCmpQ(a,b) <=> 0
 */
int
bnCmpQ_32(BigNum const *a, unsigned b)
{
	unsigned t;
	BNWORD32 v;

	t = bniNorm_32((BNWORD32 *)a->ptr, a->size);
	/* If a is more than one word long or zero, it's easy... */
	if (t != 1)
		return (t > 1) ? 1 : (b ? -1 : 0);
	v = (unsigned)((BNWORD32 *)a->ptr)[BIGLITTLE(-1,0)];
	return (v > b) ? 1 : ((v < b) ? -1 : 0);
}

int
bnSetQ_32(BigNum *dest, unsigned src)
{
	if (src) {
		bnSizeCheck(dest, 1);

		((BNWORD32 *)dest->ptr)[BIGLITTLE(-1,0)] = (BNWORD32)src;
		dest->size = 1;
	} else {
		dest->size = 0;
	}
	return 0;
}

int
bnAddQ_32(BigNum *dest, unsigned src)
{
	BNWORD32 t;

	if (!dest->size)
		return bnSetQ(dest, src);
	
	t = bniAdd1_32((BNWORD32 *)dest->ptr, dest->size, (BNWORD32)src);
	MALLOCDB;
	if (t) {
		src = dest->size;
		bnSizeCheck(dest, src+1);
		((BNWORD32 *)dest->ptr)[BIGLITTLE(-1-src,src)] = t;
		dest->size = src+1;
	}
	return 0;
}

/*
 * Return value as for bnSub: 1 if subtract underflowed, in which
 * case the return is the negative of the computed value.
 */
int
bnSubQ_32(BigNum *dest, unsigned src)
{
	BNWORD32 t;

	if (!dest->size)
		return bnSetQ(dest, src) < 0 ? -1 : (src != 0);

	t = bniSub1_32((BNWORD32 *)dest->ptr, dest->size, src);
	MALLOCDB;
	if (t) {
		/* Underflow. <= 1 word, so do it simply. */
		bniNeg_32((BNWORD32 *)dest->ptr, 1);
		dest->size = 1;
		return 1;
	}
/* Try to normalize?  Needing this is going to be pretty damn rare. */
/*		dest->size = bniNorm_32((BNWORD32 *)dest->ptr, dest->size); */
	return 0;
}

/*
 * Compare two BigNums.  Returns -1. 0 or 1 if a<b, a == b or a>b.
 * a <=> b --> bnCmp(a,b) <=> 0
 */
int
bnCmp_32(BigNum const *a, BigNum const *b)
{
	unsigned s, t;

	s = bniNorm_32((BNWORD32 *)a->ptr, a->size);
	t = bniNorm_32((BNWORD32 *)b->ptr, b->size);
	
	if (s != t)
		return s > t ? 1 : -1;
	return bniCmp_32((BNWORD32 *)a->ptr, (BNWORD32 *)b->ptr, s);
}

int
bnSquare_32(BigNum *dest, BigNum const *src)
{
	unsigned s;
	BNWORD32 *srcbuf;

	s = bniNorm_32((BNWORD32 *)src->ptr, src->size);
	if (!s) {
		dest->size = 0;
		return 0;
	}
	bnSizeCheck(dest, 2*s);

	if (src == dest) {
		BNIALLOC(srcbuf, BNWORD32, s);
		if (!srcbuf)
			return -1;
		bniCopy_32(srcbuf, (BNWORD32 *)src->ptr, s);
		bniSquare_32((BNWORD32 *)dest->ptr, (BNWORD32 *)srcbuf, s);
		BNIFREE(srcbuf, s);
	} else {
		bniSquare_32((BNWORD32 *)dest->ptr, (BNWORD32 *)src->ptr, s);
	}

	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, 2*s);
	MALLOCDB;
	return 0;
}

int
bnMul_32(BigNum *dest, BigNum const *a, BigNum const *b)
{
	unsigned s, t;
	BNWORD32 *srcbuf;

	s = bniNorm_32((BNWORD32 *)a->ptr, a->size);
	t = bniNorm_32((BNWORD32 *)b->ptr, b->size);

	if (!s || !t) {
		dest->size = 0;
		return 0;
	}

	if (a == b)
		return bnSquare_32(dest, a);

	bnSizeCheck(dest, s+t);

	if (dest == a) {
		BNIALLOC(srcbuf, BNWORD32, s);
		if (!srcbuf)
			return -1;
		bniCopy_32(srcbuf, (BNWORD32 *)a->ptr, s);
		bniMul_32((BNWORD32 *)dest->ptr, srcbuf, s,
		                                 (BNWORD32 *)b->ptr, t);
		BNIFREE(srcbuf, s);
	} else if (dest == b) {
		BNIALLOC(srcbuf, BNWORD32, t);
		if (!srcbuf)
			return -1;
		bniCopy_32(srcbuf, (BNWORD32 *)b->ptr, t);
		bniMul_32((BNWORD32 *)dest->ptr, (BNWORD32 *)a->ptr, s,
		                                 srcbuf, t);
		BNIFREE(srcbuf, t);
	} else {
		bniMul_32((BNWORD32 *)dest->ptr, (BNWORD32 *)a->ptr, s,
		                                 (BNWORD32 *)b->ptr, t);
	}
	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, s+t);
	MALLOCDB;
	return 0;
}

int
bnMulQ_32(BigNum *dest, BigNum const *a, unsigned b)
{
	unsigned s;

	s = bniNorm_32((BNWORD32 *)a->ptr, a->size);
	if (!s || !b) {
		dest->size = 0;
		return 0;
	}
	if (b == 1)
		return bnCopy_32(dest, a);
	bnSizeCheck(dest, s+1);
	bniMulN1_32((BNWORD32 *)dest->ptr, (BNWORD32 *)a->ptr, s, b);
	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, s+1);
	MALLOCDB;
	return 0;
}

int
bnDivMod_32(BigNum *q, BigNum *r, BigNum const *n,
            BigNum const *d)
{
	unsigned dsize, nsize;
	BNWORD32 qhigh;

	dsize = bniNorm_32((BNWORD32 *)d->ptr, d->size);
	nsize = bniNorm_32((BNWORD32 *)n->ptr, n->size);

	if (nsize < dsize) {
		q->size = 0;	/* No quotient */
		r->size = nsize;
		return 0;	/* Success */
	}

	bnSizeCheck(q, nsize-dsize);

	if (r != n) {	/* You are allowed to reduce in place */
		bnSizeCheck(r, nsize);
		bniCopy_32((BNWORD32 *)r->ptr, (BNWORD32 *)n->ptr, nsize);
	}
		
	qhigh = bniDiv_32((BNWORD32 *)q->ptr, (BNWORD32 *)r->ptr, nsize,
	                  (BNWORD32 *)d->ptr, dsize);
	nsize -= dsize;
	if (qhigh) {
		bnSizeCheck(q, nsize+1);
		*((BNWORD32 *)q->ptr BIGLITTLE(-nsize-1,+nsize)) = qhigh;
		q->size = nsize+1;
	} else {
		q->size = bniNorm_32((BNWORD32 *)q->ptr, nsize);
	}
	r->size = bniNorm_32((BNWORD32 *)r->ptr, dsize);
	MALLOCDB;
	return 0;
}

int
bnMod_32(BigNum *dest, BigNum const *src, BigNum const *d)
{
	unsigned dsize, nsize;

	nsize = bniNorm_32((BNWORD32 *)src->ptr, src->size);
	dsize = bniNorm_32((BNWORD32 *)d->ptr, d->size);


	if (dest != src) {
		bnSizeCheck(dest, nsize);
		bniCopy_32((BNWORD32 *)dest->ptr, (BNWORD32 *)src->ptr, nsize);
	}

	if (nsize < dsize) {
		dest->size = nsize;	/* No quotient */
		return 0;
	}

	(void)bniDiv_32((BNWORD32 *)dest->ptr BIGLITTLE(-dsize,+dsize),
	                (BNWORD32 *)dest->ptr, nsize,
	                (BNWORD32 *)d->ptr, dsize);
	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, dsize);
	MALLOCDB;
	return 0;
}

unsigned
bnModQ_32(BigNum const *src, unsigned d)
{
	unsigned s;

	s = bniNorm_32((BNWORD32 *)src->ptr, src->size);
	if (!s)
		return 0;
	
	if (d & (d-1))	/* Not a power of 2 */
		d = bniModQ_32((BNWORD32 *)src->ptr, s, d);
	else
		d = (unsigned)((BNWORD32 *)src->ptr)[BIGLITTLE(-1,0)] & (d-1);
	return d;
}

int
bnExpMod_32(BigNum *dest, BigNum const *n,
	BigNum const *exp, BigNum const *mod)
{
	unsigned nsize, esize, msize;

	nsize = bniNorm_32((BNWORD32 *)n->ptr, n->size);
	esize = bniNorm_32((BNWORD32 *)exp->ptr, exp->size);
	msize = bniNorm_32((BNWORD32 *)mod->ptr, mod->size);

	if (!msize || (((BNWORD32 *)mod->ptr)[BIGLITTLE(-1,0)] & 1) == 0)
		return -1;	/* Illegal modulus! */

	bnSizeCheck(dest, msize);

	/* Special-case base of 2 */
	if (nsize == 1 && ((BNWORD32 *)n->ptr)[BIGLITTLE(-1,0)] == 2) {
		if (bniTwoExpMod_32((BNWORD32 *)dest->ptr,
				    (BNWORD32 *)exp->ptr, esize,
				    (BNWORD32 *)mod->ptr, msize) < 0)
			return -1;
	} else {
		if (bniExpMod_32((BNWORD32 *)dest->ptr,
		                 (BNWORD32 *)n->ptr, nsize,
				 (BNWORD32 *)exp->ptr, esize,
				 (BNWORD32 *)mod->ptr, msize) < 0)
		return -1;
	}

	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, msize);
	MALLOCDB;
	return 0;
}

int
bnDoubleExpMod_32(BigNum *dest,
	BigNum const *n1, BigNum const *e1,
	BigNum const *n2, BigNum const *e2,
	BigNum const *mod)
{
	unsigned n1size, e1size, n2size, e2size, msize;

	n1size = bniNorm_32((BNWORD32 *)n1->ptr, n1->size);
	e1size = bniNorm_32((BNWORD32 *)e1->ptr, e1->size);
	n2size = bniNorm_32((BNWORD32 *)n2->ptr, n2->size);
	e2size = bniNorm_32((BNWORD32 *)e2->ptr, e2->size);
	msize = bniNorm_32((BNWORD32 *)mod->ptr, mod->size);

	if (!msize || (((BNWORD32 *)mod->ptr)[BIGLITTLE(-1,0)] & 1) == 0)
		return -1;	/* Illegal modulus! */

	bnSizeCheck(dest, msize);

	if (bniDoubleExpMod_32((BNWORD32 *)dest->ptr,
		(BNWORD32 *)n1->ptr, n1size, (BNWORD32 *)e1->ptr, e1size,
		(BNWORD32 *)n2->ptr, n2size, (BNWORD32 *)e2->ptr, e2size,
		(BNWORD32 *)mod->ptr, msize) < 0)
		return -1;

	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, msize);
	MALLOCDB;
	return 0;
}

int
bnTwoExpMod_32(BigNum *n, BigNum const *exp,
	BigNum const *mod)
{
	unsigned esize, msize;

	esize = bniNorm_32((BNWORD32 *)exp->ptr, exp->size);
	msize = bniNorm_32((BNWORD32 *)mod->ptr, mod->size);

	if (!msize || (((BNWORD32 *)mod->ptr)[BIGLITTLE(-1,0)] & 1) == 0)
		return -1;	/* Illegal modulus! */

	bnSizeCheck(n, msize);

	if (bniTwoExpMod_32((BNWORD32 *)n->ptr, (BNWORD32 *)exp->ptr, esize,
	                    (BNWORD32 *)mod->ptr, msize) < 0)
		return -1;

	n->size = bniNorm_32((BNWORD32 *)n->ptr, msize);
	MALLOCDB;
	return 0;
}

int
bnGcd_32(BigNum *dest, BigNum const *a, BigNum const *b)
{
	BNWORD32 *tmp;
	unsigned asize, bsize;
	int i;

	/* Kind of silly, but we might as well permit it... */
	if (a == b)
		return dest == a ? 0 : bnCopy(dest, a);

	/* Ensure a is not the same as "dest" */
	if (a == dest) {
		a = b;
		b = dest;
	}

	asize = bniNorm_32((BNWORD32 *)a->ptr, a->size);
	bsize = bniNorm_32((BNWORD32 *)b->ptr, b->size);

	bnSizeCheck(dest, bsize+1);

	/* Copy a to tmp */
	BNIALLOC(tmp, BNWORD32, asize+1);
	if (!tmp)
		return -1;
	bniCopy_32(tmp, (BNWORD32 *)a->ptr, asize);

	/* Copy b to dest, if necessary */
	if (dest != b)
		bniCopy_32((BNWORD32 *)dest->ptr,
			   (BNWORD32 *)b->ptr, bsize);
	if (bsize > asize || (bsize == asize &&
	        bniCmp_32((BNWORD32 *)b->ptr, (BNWORD32 *)a->ptr, asize) > 0))
	{
		i = bniGcd_32((BNWORD32 *)dest->ptr, bsize, tmp, asize,
			&dest->size);
		if (i > 0)	/* Result in tmp, not dest */
			bniCopy_32((BNWORD32 *)dest->ptr, tmp, dest->size);
	} else {
		i = bniGcd_32(tmp, asize, (BNWORD32 *)dest->ptr, bsize,
			&dest->size);
		if (i == 0)	/* Result in tmp, not dest */
			bniCopy_32((BNWORD32 *)dest->ptr, tmp, dest->size);
	}
	BNIFREE(tmp, asize+1);
	MALLOCDB;
	return (i < 0) ? i : 0;
}

int
bnInv_32(BigNum *dest, BigNum const *src,
         BigNum const *mod)
{
	unsigned s, m;
	int i;

	s = bniNorm_32((BNWORD32 *)src->ptr, src->size);
	m = bniNorm_32((BNWORD32 *)mod->ptr, mod->size);

	/* bniInv_32 requires that the input be less than the modulus */
	if (m < s ||
	    (m==s && bniCmp_32((BNWORD32 *)src->ptr, (BNWORD32 *)mod->ptr, s)))
	{
		bnSizeCheck(dest, s + (m==s));
		if (dest != src)
			bniCopy_32((BNWORD32 *)dest->ptr,
			           (BNWORD32 *)src->ptr, s);
		/* Pre-reduce modulo the modulus */
		(void)bniDiv_32((BNWORD32 *)dest->ptr BIGLITTLE(-m,+m),
			        (BNWORD32 *)dest->ptr, s,
		                (BNWORD32 *)mod->ptr, m);
		s = bniNorm_32((BNWORD32 *)dest->ptr, m);
		MALLOCDB;
	} else {
		bnSizeCheck(dest, m+1);
		if (dest != src)
			bniCopy_32((BNWORD32 *)dest->ptr,
			           (BNWORD32 *)src->ptr, s);
	}

	i = bniInv_32((BNWORD32 *)dest->ptr, s, (BNWORD32 *)mod->ptr, m);
	if (i == 0)
		dest->size = bniNorm_32((BNWORD32 *)dest->ptr, m);

	MALLOCDB;
	return i;
}

/*
 * Shift a bignum left the appropriate number of bits,
 * multiplying by 2^amt.
 */
int 
bnLShift_32(BigNum *dest, unsigned amt)
{
	unsigned s = dest->size;
	BNWORD32 carry;

	if (amt % 32) {
		carry = bniLshift_32((BNWORD32 *)dest->ptr, s, amt % 32);
		if (carry) {
			s++;
			bnSizeCheck(dest, s);
			((BNWORD32 *)dest->ptr)[BIGLITTLE(-s,s-1)] = carry;
		}
	}

	amt /= 32;
	if (amt) {
		bnSizeCheck(dest, s+amt);
		memmove((BNWORD32 *)dest->ptr BIGLITTLE(-s-amt, +amt),
		        (BNWORD32 *)dest->ptr BIG(-s),
			s * sizeof(BNWORD32));
		bniZero_32((BNWORD32 *)dest->ptr, amt);
		s += amt;
	}
	dest->size = s;
	MALLOCDB;
	return 0;
}

/*
 * Shift a bignum right the appropriate number of bits,
 * dividing by 2^amt.
 */
void bnRShift_32(BigNum *dest, unsigned amt)
{
	unsigned s = dest->size;

	if (amt >= 32) {
		memmove(
		        (BNWORD32 *)dest->ptr BIG(-s+amt/32),
			(BNWORD32 *)dest->ptr BIGLITTLE(-s, +amt/32),
			(s-amt/32) * sizeof(BNWORD32));
		s -= amt/32;
		amt %= 32;
	}

	if (amt)
		(void)bniRshift_32((BNWORD32 *)dest->ptr, s, amt);

	dest->size = bniNorm_32((BNWORD32 *)dest->ptr, s);
	MALLOCDB;
}

/*
 * Shift a bignum right until it is odd, and return the number of
 * bits shifted.  n = d * 2^s.  Replaces n with d and returns s.
 * Returns 0 when given 0.  (Another valid answer is infinity.)
 */
unsigned
bnMakeOdd_32(BigNum *n)
{
	unsigned size;
	unsigned s;	/* shift amount */
	BNWORD32 *p;
	BNWORD32 t;

	p = (BNWORD32 *)n->ptr;
	size = bniNorm_32(p, n->size);
	if (!size)
		return 0;

	t = BIGLITTLE(p[-1],p[0]);
	s = 0;

	/* See how many words we have to shift */
	if (!t) {
		/* Shift by words */
		do {
			
			s++;
			BIGLITTLE(--p,p++);
		} while ((t = BIGLITTLE(p[-1],p[0])) == 0);
		size -= s;
		s *= 32;
		memmove((BNWORD32 *)n->ptr BIG(-size), p BIG(-size),
			size * sizeof(BNWORD32));
		p = (BNWORD32 *)n->ptr;
		MALLOCDB;
	}

	pgpAssert(t);

	/* Now count the bits */
	while ((t & 1) == 0) {
		t >>= 1;
		s++;
	}

	/* Shift the bits */
	if (s & (32-1)) {
		bniRshift_32(p, size, s & (32-1));
		/* Renormalize */
		if (BIGLITTLE(*(p-size),*(p+(size-1))) == 0)
			--size;
	}
	n->size = size;

	MALLOCDB;
	return s;
}
