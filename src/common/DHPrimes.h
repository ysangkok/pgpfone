/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: DHPrimes.h,v 1.4 1999/03/10 02:42:08 heller Exp $
____________________________________________________________________________*/
#ifndef DHPRIMES_H
#define DHPRIMES_H

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

#include "PGPFone.h"

#define NUM_DH_PRIMES	6
#define _dhp_768_BIT	0x0001
#define _dhp_1024_BIT	0x0002
#define _dhp_1536_BIT	0x0004
#define _dhp_2048_BIT	0x0008
#define _dhp_3072_BIT	0x0010
#define _dhp_4096_BIT	0x0020

typedef struct DHPrime
{
	short	expSize;	// number of bits in exponent
	short	length;		// byte length of the prime
	uchar	prime[512];	// the prime
} DHPrime;

extern DHPrime	DH_768bitPrime;
extern DHPrime	DH_1024bitPrime;
extern DHPrime	DH_1280bitPrime;
extern DHPrime	DH_1536bitPrime;
extern DHPrime	DH_2048bitPrime;
extern DHPrime	DH_3072bitPrime;
extern DHPrime	DH_4096bitPrime;

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif


