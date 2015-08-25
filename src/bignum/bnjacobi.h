/*
 * For a small (usually prime, but not necessarily) prime p,
 * Return Jacobi(p,bn), which is -1, 0 or +1.
 * bn must be odd.
 *
 * $Id: bnjacobi.h,v 1.1 1997/12/14 11:30:33 wprice Exp $
 */

#ifndef Included_bnjacobi_h
#define Included_bnjacobi_h

#include "pgpBase.h"

PGP_BEGIN_C_DECLARATIONS

#include "pgpBigNumOpaqueStructs.h"

int bnJacobiQ(unsigned p, BigNum const *bn);

PGP_END_C_DECLARATIONS

#endif /* Included_bnjacobi_h */

