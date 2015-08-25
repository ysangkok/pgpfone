/*
 * bn.c - the high-level bignum interface
 *
 * Written by Colin Plumb
 *
 * $Id: bn.c,v 1.2 1998/09/20 12:03:34 wprice Exp $
 */

#include "bn.h"

/* Functions */
void
bnBegin(BigNum *bn)
{
	static short bninit = 0;

	if (!bninit) {
		bnInit();
		bninit = 1;
	}

	bn->ptr = 0;
	bn->size = 0;
	bn->allocated = 0;
}

void
bnSwap(BigNum *a, BigNum *b)
{
	void *p;
	unsigned t;

	p = a->ptr;
	a->ptr = b->ptr;
	b->ptr = p;

	t = a->size;
	a->size = b->size;
	b->size = t;

	t = a->allocated;
	a->allocated = b->allocated;
	b->allocated = t;
}

int (*bnYield)(void);

void (*bnEnd)(BigNum *bn);
int (*bnPrealloc)(BigNum *bn, unsigned bits);
int (*bnCopy)(BigNum *dest, BigNum const *src);
void (*bnNorm)(BigNum *bn);
void (*bnExtractBigBytes)(BigNum const *bn, unsigned char *dest,
	unsigned lsbyte, unsigned len);
int (*bnInsertBigBytes)(BigNum *bn, unsigned char const *src,
	unsigned lsbyte, unsigned len);
void (*bnExtractLittleBytes)(BigNum const *bn, unsigned char *dest,
	unsigned lsbyte, unsigned len);
int (*bnInsertLittleBytes)(BigNum *bn, unsigned char const *src,
	unsigned lsbyte, unsigned len);
unsigned (*bnLSWord)(BigNum const *src);
unsigned (*bnBits)(BigNum const *src);
int (*bnAdd)(BigNum *dest, BigNum const *src);
int (*bnSub)(BigNum *dest, BigNum const *src);
int (*bnCmpQ)(BigNum const *a, unsigned b);
int (*bnSetQ)(BigNum *dest, unsigned src);
int (*bnAddQ)(BigNum *dest, unsigned src);
int (*bnSubQ)(BigNum *dest, unsigned src);
int (*bnCmp)(BigNum const *a, BigNum const *b);
int (*bnSquare)(BigNum *dest, BigNum const *src);
int (*bnMul)(BigNum *dest, BigNum const *a,
	BigNum const *b);
int (*bnMulQ)(BigNum *dest, BigNum const *a, unsigned b);
int (*bnDivMod)(BigNum *q, BigNum *r, BigNum const *n,
	BigNum const *d);
int (*bnMod)(BigNum *dest, BigNum const *src,
	BigNum const *d);
unsigned (*bnModQ)(BigNum const *src, unsigned d);
int (*bnExpMod)(BigNum *result, BigNum const *n,
	BigNum const *exp, BigNum const *mod);
int (*bnDoubleExpMod)(BigNum *dest,
	BigNum const *n1, BigNum const *e1,
	BigNum const *n2, BigNum const *e2,
	BigNum const *mod);
int (*bnTwoExpMod)(BigNum *n, BigNum const *exp,
	BigNum const *mod);
int (*bnGcd)(BigNum *dest, BigNum const *a,
	BigNum const *b);
int (*bnInv)(BigNum *dest, BigNum const *src,
	BigNum const *mod);
int (*bnLShift)(BigNum *dest, unsigned amt);
void (*bnRShift)(BigNum *dest, unsigned amt);
unsigned (*bnMakeOdd)(BigNum *n);
