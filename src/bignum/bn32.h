/*
 * bn32.h - interface to 32-bit bignum routines.
 *
 * WARNING: This file was automatically generated from bn16.h -- DO NOT EDIT!
 *
 * $Id: bn32.h,v 1.2 1998/09/20 12:03:36 wprice Exp $
 */


#include "bn.h"

void bnInit_32(void);
void bnEnd_32(BigNum *bn);
int bnPrealloc_32(BigNum *bn, unsigned bits);
int bnCopy_32(BigNum *dest, BigNum const *src);
int bnSwap_32(BigNum *a, BigNum *b);
void bnNorm_32(BigNum *bn);
void bnExtractBigBytes_32(BigNum const *bn, unsigned char *dest,
	unsigned lsbyte, unsigned dlen);
int bnInsertBigBytes_32(BigNum *bn, unsigned char const *src,
	unsigned lsbyte, unsigned len);
void bnExtractLittleBytes_32(BigNum const *bn, unsigned char *dest,
	unsigned lsbyte, unsigned dlen);
int bnInsertLittleBytes_32(BigNum *bn, unsigned char const *src,
	unsigned lsbyte, unsigned len);
unsigned bnLSWord_32(BigNum const *src);
unsigned bnBits_32(BigNum const *src);
int bnAdd_32(BigNum *dest, BigNum const *src);
int bnSub_32(BigNum *dest, BigNum const *src);
int bnCmpQ_32(BigNum const *a, unsigned b);
int bnSetQ_32(BigNum *dest, unsigned src);
int bnAddQ_32(BigNum *dest, unsigned src);
int bnSubQ_32(BigNum *dest, unsigned src);
int bnCmp_32(BigNum const *a, BigNum const *b);
int bnSquare_32(BigNum *dest, BigNum const *src);
int bnMul_32(BigNum *dest, BigNum const *a,
	BigNum const *b);
int bnMulQ_32(BigNum *dest, BigNum const *a, unsigned b);
int bnDivMod_32(BigNum *q, BigNum *r, BigNum const *n,
	BigNum const *d);
int bnMod_32(BigNum *dest, BigNum const *src,
	BigNum const *d);
unsigned bnModQ_32(BigNum const *src, unsigned d);
int bnExpMod_32(BigNum *dest, BigNum const *n,
	BigNum const *exp, BigNum const *mod);
int bnDoubleExpMod_32(BigNum *dest,
	BigNum const *n1, BigNum const *e1,
	BigNum const *n2, BigNum const *e2,
	BigNum const *mod);
int bnTwoExpMod_32(BigNum *n, BigNum const *exp,
	BigNum const *mod);
int bnGcd_32(BigNum *dest, BigNum const *a,
	BigNum const *b);
int bnInv_32(BigNum *dest, BigNum const *src,
	BigNum const *mod);
int bnLShift_32(BigNum *dest, unsigned amt);
void bnRShift_32(BigNum *dest, unsigned amt);
unsigned bnMakeOdd_32(BigNum *n);

