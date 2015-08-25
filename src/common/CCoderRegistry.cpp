/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CCoderRegistry.cpp,v 1.4.10.1 1999/07/09 00:12:11 heller Exp $
____________________________________________________________________________*/
#ifdef PGP_WIN32
#include "StdAfx.h"
#endif
#include <stdlib.h>

#include "CCoderRegistry.h"
#include "CControlThread.h"
#include "fastpool.h"

#define CODERTESTBUFSIZE	15360L
CoderEntry initCoderList[] = {
			'GS4L',"GSM Lite 4410 hz", 0, 0, 0, 800,
			'GS6L',"GSM Lite 6000 hz", 0, 0, 1, 1100,
			'GS7L',"GSM Lite 7350 hz", 0, 0, 1, 1333,
			'GL80',"GSM Lite 8000 hz", 0, 0, 1, 1450,
			'GS1L',"GSM Lite 11025 hz",0, 0, 2, 2000,
			'GSM4',"GSM 4410 hz", 0, 0, 0, 910,
			'GSM6',"GSM 6000 hz", 0, 0, 1, 1250,
			'GSM7',"GSM 7350 hz", 0, 0, 1, 1516,
			'GS80',"GSM 8000 hz", 0, 0, 2, 1650,
			'GSM1',"GSM 11025 hz", 0, 0, 2, 2274,
			'ADP8',"ADPCM 8000 hz", 0, 0, 3, 4000,
			0,0,0,0,0,0,
			};
			
/*	The purpose of this module is more to determine whether or not certain
	coders are capable of running at all on the system.  It is not so much
	to simply time the coders in relation to one another since that information
	has already been essentially codified into the initialized table above.
*/

CCoderRegistry::CCoderRegistry(CControlThread *controlThread, void **outResult)
#ifdef PGP_WIN32
		: LThread(outResult)
#else
		: LThread(FALSE, thread_DefaultStack, threadOption_UsePool, outResult)
#endif	// PGP_WIN32
{
	for(mNumCoders=0;initCoderList[++mNumCoders].code;);
	mCoders = (CoderEntry *)pgp_malloc(sizeof(CoderEntry) * mNumCoders);
	pgp_memcpy(mCoders, &initCoderList, sizeof(CoderEntry) * mNumCoders);
	mControlThread = controlThread;
	mSampleBuf = (short *)pgp_malloc(CODERTESTBUFSIZE);
	mSampleCompBuf = (uchar *)pgp_malloc(CODERTESTBUFSIZE);
	mControlThread->SetCoderList(mCoders, mNumCoders);
}

CCoderRegistry::~CCoderRegistry()
{
	pgp_free(mSampleBuf);
	pgp_free(mSampleCompBuf);
}

void *
CCoderRegistry::Run(void)
{
	short cdr, inx;
	long blocks;
	short *s;
	uchar *d;
	short *sb, *se;
	ulong start;
	
#ifdef	PGP_WIN32
	SetThreadPriority(mThreadHandle, THREAD_PRIORITY_HIGHEST);
#endif
	sb = (short *)mSampleBuf;
	se = (short *)(((char *)mSampleBuf) + CODERTESTBUFSIZE);
	for(inx=0;sb<se;sb++,inx++)
		*sb = inx;	/* just setting it to non-zero data */
	for(cdr = mNumCoders-1;cdr>=0;cdr--)
	{
		(Yield)();
		switch(mCoders[cdr].code)
		{
			case 'GS7L':	/* for GSM we only test 7350 hz versions and figure out the others from that */
				blocks = 7350 / 160;
				mGSM = gsm_create();
				inx = 1;
				gsm_option(mGSM, GSM_OPT_LTP_CUT, &inx);
				start = pgp_getticks();
				for(inx = 0, s = mSampleBuf, d = mSampleCompBuf; inx<blocks ; inx++)
				{
					gsm_encode(mGSM, s, d);
					s+=160;
					d+=29;
				}
				mCoders[cdr].encTime = pgp_getticks() - start;	/* in milliseconds */
				mCoders[cdr - 2].encTime = mCoders[cdr].encTime / 5 * 3;	/* GS4L */
				mCoders[cdr - 1].encTime = mCoders[cdr].encTime / 5 * 4;	/* GS6L */
				mCoders[cdr + 1].encTime = mCoders[cdr].encTime / 5 * 6;	/* GL80 */
				mCoders[cdr + 2].encTime = mCoders[cdr].encTime / 2 * 3;	/* GS1L */
				(Yield)();
				start = pgp_getticks();
				for(inx = 0, s = mSampleBuf, d = mSampleCompBuf; inx<blocks ; inx++)
				{
					gsm_decode(mGSM, d, s);
					s+=160;
					d+=29;
				}
				mCoders[cdr].decTime = pgp_getticks() - start;	/* in milliseconds */
				mCoders[cdr - 2].decTime = mCoders[cdr].decTime / 5 * 3;	/* GS4L */
				mCoders[cdr - 1].decTime = mCoders[cdr].decTime / 5 * 4;	/* GS6L */
				mCoders[cdr + 1].decTime = mCoders[cdr].decTime / 5 * 6;	/* GL80 */
				mCoders[cdr + 2].decTime = mCoders[cdr].decTime / 2 * 3;	/* GS1L */
				gsm_destroy(mGSM);
				break;
			case 'GSM7':
				blocks = 7350 / 160;
				mGSM = gsm_create();
				start = pgp_getticks();
				for(inx = 0, s = mSampleBuf, d = mSampleCompBuf; inx<blocks ; inx++)
				{
					gsm_encode(mGSM, s, d);
					s+=160;
					d+=33;
				}
				mCoders[cdr].encTime = pgp_getticks() - start;	/* in milliseconds */
				mCoders[cdr - 2].encTime = mCoders[cdr].encTime / 5 * 3;	/* GSM4 */
				mCoders[cdr - 1].encTime = mCoders[cdr].encTime / 5 * 4;	/* GSM6 */
				mCoders[cdr + 1].encTime = mCoders[cdr].encTime / 5 * 6;	/* GS80 */
				mCoders[cdr + 2].encTime = mCoders[cdr].encTime / 2 * 3;	/* GSM1 */
				(Yield)();
				start = pgp_getticks();
				for(inx = 0, s = mSampleBuf, d = mSampleCompBuf; inx<blocks ; inx++)
				{
					gsm_decode(mGSM, d, s);
					s+=160;
					d+=33;
				}
				mCoders[cdr].decTime = pgp_getticks() - start;	/* in milliseconds */
				mCoders[cdr - 2].decTime = mCoders[cdr].decTime / 5 * 3;	/* GSM4 */
				mCoders[cdr - 1].decTime = mCoders[cdr].decTime / 5 * 4;	/* GSM5 */
				mCoders[cdr + 1].decTime = mCoders[cdr].decTime / 5 * 6;	/* GSM5 */
				mCoders[cdr + 2].decTime = mCoders[cdr].decTime / 2 * 3;	/* GSM1 */
				gsm_destroy(mGSM);
				break;
		}
	}
	//sort the list (for now it is already sorted, but we should do this eventually)
	mControlThread->SetCoderList(mCoders, mNumCoders);
	return (void *)1L;
}

