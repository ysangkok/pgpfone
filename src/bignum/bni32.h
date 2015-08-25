/*
 * $Id: bni32.h,v 1.2 1998/09/20 12:03:38 wprice Exp $
 *
 * WARNING: This file was automatically generated from bni16.h -- DO NOT EDIT!
 */

#ifndef Included_bni32_h
#define Included_bni32_h

#include "bni.h"

#ifndef BNWORD32
#error 32-bit bignum library requires a 32-bit data type
#endif


#ifndef bniCopy_32
void bniCopy_32(BNWORD32 *dest, BNWORD32 const *src, unsigned len);
#endif
#ifndef bniZero_32
void bniZero_32(BNWORD32 *num, unsigned len);
#endif
#ifndef bniNeg_32
void bniNeg_32(BNWORD32 *num, unsigned len);
#endif

#ifndef bniAdd1_32
BNWORD32 bniAdd1_32(BNWORD32 *num, unsigned len, BNWORD32 carry);
#endif
#ifndef bniSub1_32
BNWORD32 bniSub1_32(BNWORD32 *num, unsigned len, BNWORD32 borrow);
#endif

#ifndef bniAddN_32
BNWORD32 bniAddN_32(BNWORD32 *num1, BNWORD32 const *num2, unsigned len);
#endif
#ifndef bniSubN_32
BNWORD32 bniSubN_32(BNWORD32 *num1, BNWORD32 const *num2, unsigned len);
#endif

#ifndef bniCmp_32
int bniCmp_32(BNWORD32 const *num1, BNWORD32 const *num2, unsigned len);
#endif

#ifndef bniMulN1_32
void bniMulN1_32(BNWORD32 *out, BNWORD32 const *in, unsigned len, BNWORD32 k);
#endif
#ifndef bniMulAdd1_32
BNWORD32
bniMulAdd1_32(BNWORD32 *out, BNWORD32 const *in, unsigned len, BNWORD32 k);
#endif
#ifndef bniMulSub1_32
BNWORD32 bniMulSub1_32(BNWORD32 *out, BNWORD32 const *in, unsigned len,
			BNWORD32 k);
#endif

#ifndef bniLshift_32
BNWORD32 bniLshift_32(BNWORD32 *num, unsigned len, unsigned shift);
#endif
#ifndef bniDouble_32
BNWORD32 bniDouble_32(BNWORD32 *num, unsigned len);
#endif
#ifndef bniRshift_32
BNWORD32 bniRshift_32(BNWORD32 *num, unsigned len, unsigned shift);
#endif

#ifndef bniMul_32
void bniMul_32(BNWORD32 *prod, BNWORD32 const *num1, unsigned len1,
	BNWORD32 const *num2, unsigned len2);
#endif
#ifndef bniSquare_32
void bniSquare_32(BNWORD32 *prod, BNWORD32 const *num, unsigned len);
#endif

#ifndef bniNorm_32
unsigned bniNorm_32(BNWORD32 const *num, unsigned len);
#endif
#ifndef bniBits_32
unsigned bniBits_32(BNWORD32 const *num, unsigned len);
#endif

#ifndef bniExtractBigBytes_32
void bniExtractBigBytes_32(BNWORD32 const *bn, unsigned char *buf,
	unsigned lsbyte, unsigned buflen);
#endif
#ifndef bniInsertBigytes_32
void bniInsertBigBytes_32(BNWORD32 *n, unsigned char const *buf,
	unsigned lsbyte,  unsigned buflen);
#endif
#ifndef bniExtractLittleBytes_32
void bniExtractLittleBytes_32(BNWORD32 const *bn, unsigned char *buf,
	unsigned lsbyte, unsigned buflen);
#endif
#ifndef bniInsertLittleBytes_32
void bniInsertLittleBytes_32(BNWORD32 *n, unsigned char const *buf,
	unsigned lsbyte,  unsigned buflen);
#endif

#ifndef bniDiv21_32
BNWORD32 bniDiv21_32(BNWORD32 *q, BNWORD32 nh, BNWORD32 nl, BNWORD32 d);
#endif
#ifndef bniDiv1_32
BNWORD32 bniDiv1_32(BNWORD32 *q, BNWORD32 *rem,
	BNWORD32 const *n, unsigned len, BNWORD32 d);
#endif
#ifndef bniModQ_32
unsigned bniModQ_32(BNWORD32 const *n, unsigned len, unsigned d);
#endif
#ifndef bniDiv_32
BNWORD32
bniDiv_32(BNWORD32 *q, BNWORD32 *n, unsigned nlen, BNWORD32 *d, unsigned dlen);
#endif

#ifndef bniMontInv1_32
BNWORD32 bniMontInv1_32(BNWORD32 const x);
#endif
#ifndef bniMontReduce_32
void bniMontReduce_32(BNWORD32 *n, BNWORD32 const *mod, unsigned const mlen,
                BNWORD32 inv);
#endif
#ifndef bniToMont_32
void bniToMont_32(BNWORD32 *n, unsigned nlen, BNWORD32 *mod, unsigned mlen);
#endif
#ifndef bniFromMont_32
void bniFromMont_32(BNWORD32 *n, BNWORD32 *mod, unsigned len);
#endif

#ifndef bniExpMod_32
int bniExpMod_32(BNWORD32 *result, BNWORD32 const *n, unsigned nlen,
	BNWORD32 const *exp, unsigned elen, BNWORD32 *mod, unsigned mlen);
#endif
#ifndef bniDoubleExpMod_32
int bniDoubleExpMod_32(BNWORD32 *result,
	BNWORD32 const *n1, unsigned n1len, BNWORD32 const *e1, unsigned e1len,
	BNWORD32 const *n2, unsigned n2len, BNWORD32 const *e2, unsigned e2len,
	BNWORD32 *mod, unsigned mlen);
#endif
#ifndef bniTwoExpMod_32
int bniTwoExpMod_32(BNWORD32 *n, BNWORD32 const *exp, unsigned elen,
	BNWORD32 *mod, unsigned mlen);
#endif
#ifndef bniGcd_32
int bniGcd_32(BNWORD32 *a, unsigned alen, BNWORD32 *b, unsigned blen,
	unsigned *rlen);
#endif
#ifndef bniInv_32
int bniInv_32(BNWORD32 *a, unsigned alen, BNWORD32 const *mod, unsigned mlen);
#endif


#endif /* Included_bni32_h */
