/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: SHA.cpp,v 1.3 1999/03/10 03:03:17 heller Exp $
____________________________________________________________________________*/
#include <string.h>

#include "CHash.h"
#include "PGPFoneUtils.h"
#include "SHA.h"

#define f1(x,y,z)	( z ^ (x & (y ^ z) ) )			// Rounds  0-19
#define f2(x,y,z)	( x ^ y ^ z )					// Rounds 20-39
#define f3(x,y,z)	( (x & y) | (z & (x | y) ) )	// Rounds 40-59
#define f4(x,y,z)	( x ^ y ^ z )					// Rounds 60-79

// The SHS Mysterious Constants

#define K1	0x5A827999L		// Rounds  0-19
#define K2	0x6ED9EBA1L		// Rounds 20-39
#define K3	0x8F1BBCDCL		// Rounds 40-59
#define K4	0xCA62C1D6L		// Rounds 60-79

// SHS initial values

#define h0init	0x67452301L
#define h1init	0xEFCDAB89L
#define h2init	0x98BADCFEL
#define h3init	0x10325476L
#define h4init	0xC3D2E1F0L

// 32-bit rotate left - kludged with shifts

#define ROTL(n,X)  ( (X << n) | (X >> (32-n)) )

// The hash function is defined over an 80-word expanded input array W,
//  where the first 16 are copies of the input data, and the remaining 64
//  are defined by W[i] = W[i-16] ^ W[i-14] ^ W[i-8] ^ W[i-3].  This
//  implementation generates these values on the fly in a circular buffer.

// The new (corrected) SHA
#define expand(W,i) (W[i&15] ^= W[(i-14)&15] ^ W[(i-8)&15] ^ W[(i-3)&15], \
                     W[i&15] = ROTL(1, W[i&15]))

// The fundamental sub-round is
// a' = e + ROTL(5,a) + f(b, c, d) + k + data;
// b' = a;
// c' = ROTL(30,b);
// d' = c;
// e' = d;
// ... but this is implemented by unrolling the loop 5 times and renaming
// the variables (e,a,b,c,d) = (a',b',c',d',e') each iteration.

#define subRound(a, b, c, d, e, f, k, data) \
	( e += ROTL(5,a) + f(b, c, d) + k + data, b = ROTL(30, b) )

static void
byteReverse(ulong *buf, unsigned byteCount)
{
	static uchar	bytes[4];
	ulong	value;

	byteCount /= sizeof(ulong);
	while ( byteCount-- )
	{
		value = *buf;
		bytes[0] = (uchar)(value >> 24);
		bytes[1] = (uchar)(value >> 16);
		bytes[2] = (uchar)(value >> 8);
		bytes[3] = (uchar)(value);
		*buf++ = *(ulong*)bytes;
		
/*		value = ( value & 0xFF00FF00L ) >> 8  | \
			( value & 0x00FF00FFL ) << 8;
		*buf++ = value << 16  |  value >> 16 ; */
	}
}

/* The above code is replicated 20 times for each of the 4 functions,
   using the next 20 values from the W[] array each time. */

SHA::SHA(void)
{
	Init();
	mHashSize = 20;
}

void
SHA::Init(void)
{
	// Set the h-vars to their initial values
	mDigest[0] = h0init;
	mDigest[1] = h1init;
	mDigest[2] = h2init;
	mDigest[3] = h3init;
	mDigest[4] = h4init;

	// Initialise bit count
	mCountLo = mCountHi = 0;
}

void
SHA::Transform(void)
{
	register ulong A, B, C, D, E;

	// Set up first buffer
	A = mDigest[0];
	B = mDigest[1];
	C = mDigest[2];
	D = mDigest[3];
	E = mDigest[4];

	// Heavy mangling, in 4 sub-rounds of 20 interations each.
	subRound( A, B, C, D, E, f1, K1, mData[ 0] );
	subRound( E, A, B, C, D, f1, K1, mData[ 1] );
	subRound( D, E, A, B, C, f1, K1, mData[ 2] );
	subRound( C, D, E, A, B, f1, K1, mData[ 3] );
	subRound( B, C, D, E, A, f1, K1, mData[ 4] );
	subRound( A, B, C, D, E, f1, K1, mData[ 5] );
	subRound( E, A, B, C, D, f1, K1, mData[ 6] );
	subRound( D, E, A, B, C, f1, K1, mData[ 7] );
	subRound( C, D, E, A, B, f1, K1, mData[ 8] );
	subRound( B, C, D, E, A, f1, K1, mData[ 9] );
	subRound( A, B, C, D, E, f1, K1, mData[10] );
	subRound( E, A, B, C, D, f1, K1, mData[11] );
	subRound( D, E, A, B, C, f1, K1, mData[12] );
	subRound( C, D, E, A, B, f1, K1, mData[13] );
	subRound( B, C, D, E, A, f1, K1, mData[14] );
	subRound( A, B, C, D, E, f1, K1, mData[15] );
	subRound( E, A, B, C, D, f1, K1, expand(mData, 16) );
	subRound( D, E, A, B, C, f1, K1, expand(mData, 17) );
	subRound( C, D, E, A, B, f1, K1, expand(mData, 18) );
	subRound( B, C, D, E, A, f1, K1, expand(mData, 19) );

	subRound( A, B, C, D, E, f2, K2, expand(mData, 20) );
	subRound( E, A, B, C, D, f2, K2, expand(mData, 21) );
	subRound( D, E, A, B, C, f2, K2, expand(mData, 22) );
	subRound( C, D, E, A, B, f2, K2, expand(mData, 23) );
	subRound( B, C, D, E, A, f2, K2, expand(mData, 24) );
	subRound( A, B, C, D, E, f2, K2, expand(mData, 25) );
	subRound( E, A, B, C, D, f2, K2, expand(mData, 26) );
	subRound( D, E, A, B, C, f2, K2, expand(mData, 27) );
	subRound( C, D, E, A, B, f2, K2, expand(mData, 28) );
	subRound( B, C, D, E, A, f2, K2, expand(mData, 29) );
	subRound( A, B, C, D, E, f2, K2, expand(mData, 30) );
	subRound( E, A, B, C, D, f2, K2, expand(mData, 31) );
	subRound( D, E, A, B, C, f2, K2, expand(mData, 32) );
	subRound( C, D, E, A, B, f2, K2, expand(mData, 33) );
	subRound( B, C, D, E, A, f2, K2, expand(mData, 34) );
	subRound( A, B, C, D, E, f2, K2, expand(mData, 35) );
	subRound( E, A, B, C, D, f2, K2, expand(mData, 36) );
	subRound( D, E, A, B, C, f2, K2, expand(mData, 37) );
	subRound( C, D, E, A, B, f2, K2, expand(mData, 38) );
	subRound( B, C, D, E, A, f2, K2, expand(mData, 39) );

	subRound( A, B, C, D, E, f3, K3, expand(mData, 40) );
	subRound( E, A, B, C, D, f3, K3, expand(mData, 41) );
	subRound( D, E, A, B, C, f3, K3, expand(mData, 42) );
	subRound( C, D, E, A, B, f3, K3, expand(mData, 43) );
	subRound( B, C, D, E, A, f3, K3, expand(mData, 44) );
	subRound( A, B, C, D, E, f3, K3, expand(mData, 45) );
	subRound( E, A, B, C, D, f3, K3, expand(mData, 46) );
	subRound( D, E, A, B, C, f3, K3, expand(mData, 47) );
	subRound( C, D, E, A, B, f3, K3, expand(mData, 48) );
	subRound( B, C, D, E, A, f3, K3, expand(mData, 49) );
	subRound( A, B, C, D, E, f3, K3, expand(mData, 50) );
	subRound( E, A, B, C, D, f3, K3, expand(mData, 51) );
	subRound( D, E, A, B, C, f3, K3, expand(mData, 52) );
	subRound( C, D, E, A, B, f3, K3, expand(mData, 53) );
	subRound( B, C, D, E, A, f3, K3, expand(mData, 54) );
	subRound( A, B, C, D, E, f3, K3, expand(mData, 55) );
	subRound( E, A, B, C, D, f3, K3, expand(mData, 56) );
	subRound( D, E, A, B, C, f3, K3, expand(mData, 57) );
	subRound( C, D, E, A, B, f3, K3, expand(mData, 58) );
	subRound( B, C, D, E, A, f3, K3, expand(mData, 59) );

	subRound( A, B, C, D, E, f4, K4, expand(mData, 60) );
	subRound( E, A, B, C, D, f4, K4, expand(mData, 61) );
	subRound( D, E, A, B, C, f4, K4, expand(mData, 62) );
	subRound( C, D, E, A, B, f4, K4, expand(mData, 63) );
	subRound( B, C, D, E, A, f4, K4, expand(mData, 64) );
	subRound( A, B, C, D, E, f4, K4, expand(mData, 65) );
	subRound( E, A, B, C, D, f4, K4, expand(mData, 66) );
	subRound( D, E, A, B, C, f4, K4, expand(mData, 67) );
	subRound( C, D, E, A, B, f4, K4, expand(mData, 68) );
	subRound( B, C, D, E, A, f4, K4, expand(mData, 69) );
	subRound( A, B, C, D, E, f4, K4, expand(mData, 70) );
	subRound( E, A, B, C, D, f4, K4, expand(mData, 71) );
	subRound( D, E, A, B, C, f4, K4, expand(mData, 72) );
	subRound( C, D, E, A, B, f4, K4, expand(mData, 73) );
	subRound( B, C, D, E, A, f4, K4, expand(mData, 74) );
	subRound( A, B, C, D, E, f4, K4, expand(mData, 75) );
	subRound( E, A, B, C, D, f4, K4, expand(mData, 76) );
	subRound( D, E, A, B, C, f4, K4, expand(mData, 77) );
	subRound( C, D, E, A, B, f4, K4, expand(mData, 78) );
	subRound( B, C, D, E, A, f4, K4, expand(mData, 79) );

	// Build message digest
	mDigest[0] += A;
	mDigest[1] += B;
	mDigest[2] += C;
	mDigest[3] += D;
	mDigest[4] += E;
}

void
SHA::Update(uchar *buf, short len)
{
	ulong t;

	// Update bitcount
	t = mCountLo;
	if ( ( mCountLo = t + len ) < t )
		mCountHi++;		// Carry from low to high

	t &= 0x3f;			// Bytes already in mData

	// Handle any leading odd-sized chunks
	if (t)
	{
		uchar *p = (uchar *)mData + t;

		t = 64-t;
		if (len < t)
		{
			pgp_memcpy(p, buf, len);
			return;
		}
		pgp_memcpy(p, buf, t);
		byteReverse(mData, SHS_BLOCKSIZE);
		Transform();
		buf += t;
		len -= t;
	}

	// Process data in SHS_BLOCKSIZE chunks
	while (len >= SHS_BLOCKSIZE)
	{
		pgp_memcpy(mData, buf, SHS_BLOCKSIZE);
		byteReverse(mData, SHS_BLOCKSIZE);
		Transform();
		buf += SHS_BLOCKSIZE;
		len -= SHS_BLOCKSIZE;
	}

	// Handle any remaining bytes of data.
	pgp_memcpy(mData, buf, len);
}

void
SHA::Final(uchar *hash)
{
	int len;
	uchar *p;

	// Compute number of bytes mod 64
	len = (int)mCountLo & 0x3F;

	// Set the first char of padding to 0x80.
	// This is safe since there is always at least one byte free
	p = (uchar*)mData + len;
	*p++ = 0x80;

	// Bytes of padding needed to make 64 bytes
	len = SHS_BLOCKSIZE - 1 - len;

	// Pad out to 56 mod 64
	if (len < 8)
	{
		// Two lots of padding:  Pad the first block to 64 bytes
		memset(p, 0, len);
		byteReverse(mData, SHS_BLOCKSIZE);
		Transform();

		// Now fill the next block with 56 bytes
		memset(mData, 0, SHS_BLOCKSIZE-8);
	}
	else
	{
		// Pad block to 56 bytes
		memset(p, 0, len-8);
	}
	byteReverse(mData, SHS_BLOCKSIZE-8);

	// Append length in *bits* and transform
	mData[14] = mCountHi << 3 | mCountLo >> 29;
	mData[15] = mCountLo << 3;

	Transform();

	// Store output hash in buffer
	byteReverse(mDigest, SHS_DIGESTSIZE);
	pgp_memcpy(hash, mDigest, SHS_DIGESTSIZE);
	memset(mData, 0, 16*sizeof(ulong));
	memset(mDigest, 0, 5*sizeof(ulong));
	mCountLo = mCountHi = 0;
}

SHA *
SHA::Clone()
{
	SHA *clone;
	short inx;
	
	clone = new SHA();
	for(inx=0;inx<16;inx++)
		clone->mData[inx] = mData[inx];
	for(inx=0;inx<5;inx++)
		clone->mDigest[inx] = mDigest[inx];
	clone->mCountHi = mCountHi;
	clone->mCountLo = mCountLo;
	return clone;
}

