/*
 * $Id: bnprint.h,v 1.1 1997/12/14 11:30:38 wprice Exp $
 */

#ifndef Included_bnprint_h
#define Included_bnprint_h

#include "pgpBase.h"

PGP_BEGIN_C_DECLARATIONS

#include <stdio.h>
#include "pgpOpaqueStructs.h"

/* Print in base 16 */
int bnPrint(FILE *f, char const *prefix, BigNum const *bn,
	char const *suffix);

/* Print in base 10 */
int bnPrint10(FILE *f, char const *prefix, BigNum const *bn,
	char const *suffix);

PGP_END_C_DECLARATIONS

#endif /* Included_bnprint_h */

