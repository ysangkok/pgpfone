/*
 * $Id: bngermain.h,v 1.1 1997/12/14 11:30:14 wprice Exp $
 */

#ifndef Included_bngermain_h
#define Included_bngermain_h

#include "pgpBase.h"

PGP_BEGIN_C_DECLARATIONS

#include "pgpBigNumOpaqueStructs.h"

/* Generate a Sophie Germain prime */
int bnGermainPrimeGen(BigNum *bn, unsigned dbl,
	int (*f)(void *arg, int c), void *arg);

PGP_END_C_DECLARATIONS

#endif /* Included_bngermain_h */
