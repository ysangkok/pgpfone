/*
 * bnsieve.h - Trial division for prime finding.
 *
 * This is generally not intended for direct use by a user of the library;
 * the bnprime.c and dhprime.c functions. are more likely to be used.
 * However, a special application may need these.
 *
 * $Id: bnsieve.h,v 1.1 1997/12/14 11:30:39 wprice Exp $
 */

#ifndef Included_bnsieve_h
#define Included_bnsieve_h

PGP_BEGIN_C_DECLARATIONS

#include "pgpBase.h"

#include "pgpOpaqueStructs.h"

/* Remove multiples of a single number from the sieve */
void sieveSingle(unsigned char *array, unsigned size, unsigned start,
	unsigned step);

/* Build a sieve starting at the number and incrementing by "step". */
int sieveBuild(unsigned char *array, unsigned size,
	BigNum const *bn, unsigned step, unsigned dbl);

/* Similar, but uses a >16-bit step size */
int sieveBuildBig(unsigned char *array, unsigned size,
	BigNum const *bn, BigNum const *step, unsigned dbl);

/* Return the next bit set in the sieve (or 0 on failure) */
unsigned sieveSearch(unsigned char const *array, unsigned size,
	unsigned start);

PGP_END_C_DECLARATIONS

#endif /* Included_bnsieve_h */
