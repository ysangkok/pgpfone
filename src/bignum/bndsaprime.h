/*
 * $Id: bndsaprime.h,v 1.1 1997/12/14 11:30:13 wprice Exp $
 */

PGP_BEGIN_C_DECLARATIONS

#include "pgpBigNumOpaqueTypes.h"


/* Generate a pair of DSS primes */
int dsaPrimeGen(BigNum *bnq, BigNum *bnp,
	int (*f)(void *arg, int c), void *arg);

PGP_END_C_DECLARATIONS
