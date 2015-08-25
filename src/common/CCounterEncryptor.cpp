/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CCounterEncryptor.cpp,v 1.3 1999/03/10 02:32:24 heller Exp $
____________________________________________________________________________*/
#include <string.h>
#include <stdio.h>

#include "CCounterEncryptor.h"
#include "CPFTransport.h"
#include "PGPFoneUtils.h"
#include "CStatusPane.h"

#include "blowfish.h"
#include "des3.h"
#include "cast5.h"

#define noDEBUGCOUNTER

CCounterEncryptor::CCounterEncryptor(uchar *key, uchar *alg)
{
	if(!memcmp(alg, sCryptorHash[_enc_cast], 4))
	{
		mKS1 = (uchar *)pgp_malloc(32 * sizeof(long));
		CAST5schedule((ulong *)mKS1, key);
		mAlgorithm = _enc_cast;
	}
	else if(!memcmp(alg, sCryptorHash[_enc_tripleDES], 4))
	{
		des3key(key, EN0, (void**)&mKS1);
		mAlgorithm = _enc_tripleDES;
	}
	else if(!memcmp(alg, sCryptorHash[_enc_blowfish], 4))
	{
		BlowfishInit(key, 24, (ulong**)&mKS1, (ulong***)&mKS2);
		mAlgorithm = _enc_blowfish;
	}
	else
		pgp_errstring("CEncryptionStream::CEncryptionStream() invalid algorithm");
	mDecrypting = FALSE;
}

CCounterEncryptor::~CCounterEncryptor(void)
{
	switch(mAlgorithm)
	{
		case _enc_tripleDES:
			DESDKeyDone((void*)mKS1);
			break;
		case _enc_blowfish:
			BlowfishDone((ulong*)mKS1, (ulong**)mKS2);
			break;
		case _enc_cast:
			memset(mKS1, 0, 32 * sizeof(long));
			pgp_free(mKS1);
			break;
	}
}

void
CCounterEncryptor::Decrypt(uchar *buffer, long length, uchar *iv)
{
	mDecrypting = TRUE;	// Recorded for debugging purposes only
	Encrypt(buffer, length, iv);
	mDecrypting = FALSE;
}

void
CCounterEncryptor::Encrypt(uchar *buffer, long length, uchar *iv)
{
	uchar *p = buffer, counter[8];
	ulong fb[2];
	ushort cbCount;
	long avail;
	short i, aligned;

	if((ulong)buffer % 4)
		aligned = 0;
	else
		aligned = 1;
	// Structure of the counter is as follows:
	// Byte 1		Packet Type
	// Byte 2-6		Packet Counter, mod 2^40
	// Byte 7-8		Cipher block counter
	for(i=0;i<6;i++)
		counter[i]=iv[i];
	for(cbCount = 0;length>0;cbCount++)
	{
		SHORT_TO_BUFFER(cbCount, &counter[6]);
#ifdef DEBUGCOUNTER
		char s[100], t[100];
		
		if(mDecrypting)
			strcpy(s, "D:");
		else
			strcpy(s, "E:");
		for(i=0;i<8;i++)
		{
			sprintf(t, "%02x", *(counter + i));
			strcat(s,t);
		}
#endif
		switch(mAlgorithm)
		{
			case _enc_tripleDES:
				D3des(counter, (uchar *)fb, mKS1);
				break;
			case _enc_blowfish:
				BlowfishEncipher((ulong *)&counter[0], (ulong *)&counter[4],
					&fb[0], &fb[1], (ulong*)mKS1, (ulong**)mKS2);
				break;
			case _enc_cast:
				CAST5encrypt(counter, (uchar *)fb, (ulong *)mKS1);
				break;
		}
#ifdef DEBUGCOUNTER
		strcat(s,":::>");
		for(i=0;i<8;i++)
		{
			sprintf(t, "%02x", *((uchar *)fb + i));
			strcat(s,t);
		}
		if(!cbCount)
			DebugLog(s);
#endif
		// Alignment check below makes sure p is long-aligned
		if(0 && (length >= 8) && aligned)
		{
			*(ulong *)p ^= fb[0];	p+=4;
			*(ulong *)p ^= fb[1];	p+=4;
			avail = 8;
		}
		else
		{
			avail = (length > 8) ? 8 : length;
			for(i=0;i<avail;i++)
				*p++ ^= *((uchar *)fb + i);
		}
		length -= avail;
	}
}

