/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: blowfish.cpp,v 1.4.10.1 1999/07/09 22:42:35 heller Exp $
____________________________________________________________________________*/
#ifdef PGP_WIN32
#include "StdAfx.h"
#endif
#include "blowfish.h"

#include "pi.inc"
#include "PGPFoneUtils.h"

#include <string.h>

#define N	16

static void
coreEncrypt(ulong *sxl, ulong *sxr, ulong *dxl, ulong *dxr, ulong *P, ulong **S)
{
	ulong	Xl = *sxl, Xr = *sxr, *subkey = P;
	ulong	f;
	int		i;

	(void) dxl;
	(void) dxr;
	
	for (i = 0; i < N; i += 2)
	{
		Xl ^= P[i];
/*		Xl ^= *subkey++;	*/
		f   = S[0][(uchar)(Xl >> 24)];
		f  += S[1][(uchar)(Xl >> 16)];
		f  ^= S[2][(uchar)(Xl >>  8)];
		f  += S[3][(uchar)(Xl)];
		Xr ^= f ^ P[i+1];
/*		Xr ^= f ^ *subkey++;	*/
		f   = S[0][(uchar)(Xr >> 24)];
		f  += S[1][(uchar)(Xr >> 16)];
		f  ^= S[2][(uchar)(Xr >>  8)];
		f  += S[3][(uchar)(Xr)];
		Xl ^= f;
	}

	/* Do a logical swap of Xl and Xr here (just rename them) */
	*sxr = Xl ^= P[N];
	*sxl = Xr ^= P[N + 1];
/*	*sxr = Xl ^= *subkey++;
	*sxl = Xr ^= *subkey++;	*/
}

void BlowfishEncipher(ulong *sxl, ulong *sxr, ulong *dxl, ulong *dxr, ulong *P, ulong **S)
{
	ulong	Xl, Xr;

	/* treat *sxl and *sxr as if they were big-endian ulongs */
#if PGP_WIN32
	Xl =	((ulong)((uchar*)sxl)[0] << 24) |
			((ulong)((uchar*)sxl)[1] << 16) |
			((ulong)((uchar*)sxl)[2] <<  8) |
			((ulong)((uchar*)sxl)[3]);
	Xr =	((ulong)((uchar*)sxr)[0] << 24) |
			((ulong)((uchar*)sxr)[1] << 16) |
			((ulong)((uchar*)sxr)[2] <<  8) |
			((ulong)((uchar*)sxr)[3]);
#else
	Xl = *sxl;
	Xr = *sxr;
#endif

	coreEncrypt(&Xl, &Xr, &Xl, &Xr, P, S);

	/* make sure that we create big-endian ulongs */
#if PGP_WIN32
	((uchar*)dxl)[0] = (uchar)(Xl >> 24);
	((uchar*)dxl)[1] = (uchar)(Xl >> 16);
	((uchar*)dxl)[2] = (uchar)(Xl >>  8);
	((uchar*)dxl)[3] = (uchar)(Xl);
	((uchar*)dxr)[0] = (uchar)(Xr >> 24);
	((uchar*)dxr)[1] = (uchar)(Xr >> 16);
	((uchar*)dxr)[2] = (uchar)(Xr >>  8);
	((uchar*)dxr)[3] = (uchar)(Xr);
#else
	*dxl = Xl;
	*dxr = Xr;
#endif
}

/* the following BlowfishDecipher code may not work on every system (i.e.,
	in an endian-independent manner).  however, we don't need the decrypt
	function because we are using Cipher FeedBack Mode (CFB) which only
	uses the encyrpt function. */

void BlowfishDecipher(ulong *sxl, ulong *sxr, ulong *dxl, ulong *dxr, 
	ulong *P, ulong **S)
{
	ulong	Xl, Xr;
	ulong	f;
	int		i;

	/* treat *sxl and *sxr as if they were little-endian ulongs */
	Xl =	((ulong)((uchar*)sxl)[0] << 24) |
			((ulong)((uchar*)sxl)[1] << 16) |
			((ulong)((uchar*)sxl)[2] <<  8) |
			((ulong)((uchar*)sxl)[3]);
	Xr =	((ulong)((uchar*)sxr)[0] << 24) |
			((ulong)((uchar*)sxr)[1] << 16) |
			((ulong)((uchar*)sxr)[2] <<  8) |
			((ulong)((uchar*)sxr)[3]);

	for (i = N; i > 0; i -= 2)
	{
		Xl ^= P[i+1];
		f  = S[0][(uchar)(Xl >> 24)];
		f += S[1][(uchar)(Xl >> 16)];
		f ^= S[2][(uchar)(Xl >>  8)];
		f += S[3][(uchar)(Xl)];
		Xr ^= f ^ P[i];
		f  = S[0][(uchar)(Xr >> 24)];
		f += S[1][(uchar)(Xr >> 16)];
		f ^= S[2][(uchar)(Xr >>  8)];
		f += S[3][(uchar)(Xr)];
		Xl ^= f;
	}

	/* make sure that we create little-endian ulongs */
	Xl ^= P[1];
	Xr ^= P[0];

	((uchar*)dxr)[0] = (uchar)(Xl >> 24);
	((uchar*)dxr)[1] = (uchar)(Xl >> 16);
	((uchar*)dxr)[2] = (uchar)(Xl >>  8);
	((uchar*)dxr)[3] = (uchar)(Xl);
	((uchar*)dxl)[0] = (uchar)(Xr >> 24);
	((uchar*)dxl)[1] = (uchar)(Xr >> 16);
	((uchar*)dxl)[2] = (uchar)(Xr >>  8);
	((uchar*)dxl)[3] = (uchar)(Xr);
}

void
BlowfishInit(uchar *key, short keybytes, ulong **newP, ulong ***newS)
{
	short	i, j, k;
	ulong	data1, data2;
	ulong	*P = *newP = (ulong*)pgp_malloc(sizeof(ulong)*(N+2));
	ulong	**S = *newS = (ulong**)pgp_malloc(sizeof(ulong*)*4);
	uchar	*pi;

	/* allocate memory for the rest of the key schedule */
	for (i = 0; i < 4; i++)
		S[i] = (ulong*)pgp_malloc(sizeof(ulong)*256);

	pi = PiArray;
	for (i = 0; i < N + 2; ++i)
	{
		P[i] =	(ulong)((unsigned)pi[0] << 8 | pi[1]) << 16 |
				((unsigned)pi[2] << 8 | pi[3]);
		pi += 4;
	}

	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 256; ++j)
		{
			S[i][j] =	(ulong)((unsigned)pi[0] << 8 | pi[1]) << 16 |
						((unsigned)pi[2] << 8 | pi[3]);
			pi += 4;
		}
	}

	if (keybytes)
	{
		j = 0;
		data1 = 0;
		for (i = 0; i < N + 2; ++i)
		{
			for (k = 0; k < 4; ++k)
			{
				data1 = (data1 << 8) | key[j];
				if (++j == keybytes)
					j = 0;
			}
			P[i] ^= data1;
		}
	}

	data1 = 0x00000000;
	data2 = 0x00000000;
	for (i = 0; i < N + 2; i += 2)
	{
		coreEncrypt(&data1, &data2, &data1, &data2, P, S);

		P[i] = data1;
		P[i + 1] = data2;
	}

	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 256; j += 2)
		{
			coreEncrypt(&data1, &data2, &data1, &data2, P, S);

			S[i][j] = data1;
			S[i][j + 1] = data2;
		}
	}
}

void
BlowfishDone(ulong *P, ulong **S)
{
	memset(P, 0, sizeof(ulong)*(N+2));
	pgp_free(P);
	memset(S[0], 0, sizeof(ulong)*256);
	pgp_free(S[0]);
	memset(S[1], 0, sizeof(ulong)*256);
	pgp_free(S[1]);
	memset(S[2], 0, sizeof(ulong)*256);
	pgp_free(S[2]);
	memset(S[3], 0, sizeof(ulong)*256);
	pgp_free(S[3]);
	pgp_free(S);
}

#if 0

/* this code is for testing that the above routines encrypt data correctly
	according to a set of test vectors provided by Bruce Schneier	*/

ulong	GetTickCount()	{	return 0;	}

main()
{
	uchar	test[8] = {0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};
	char	key [] = "Who is John Galt?";
	ulong	A, B, C, D;
	ulong	**S, *P;
	int		i;

	BlowfishInit(key, strlen(key), &P, &S);
	BlowfishEncipher((ulong*)test, (ulong*)(test+4), &A, &B, P, S);
	BlowfishDecipher(&A, &B, &C, &D, P, S);

	printf("0x%02X%02X%02X%02X ",	(uchar)(A >> 24), (uchar)(A >> 16),
									(uchar)(A >>  8), (uchar)(A));
	printf("0x%02X%02X%02X%02X\n",	(uchar)(B >> 24), (uchar)(B >> 16),
									(uchar)(B >>  8), (uchar)(B));
	printf("0x%02X%02X%02X%02X ",	(uchar)(C >> 24), (uchar)(C >> 16),
									(uchar)(C >>  8), (uchar)(C));
	printf("0x%02X%02X%02X%02X\n",	(uchar)(D >> 24), (uchar)(D >> 16),
									(uchar)(D >>  8), (uchar)(D));
}
#endif

