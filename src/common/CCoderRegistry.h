/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CCoderRegistry.h,v 1.5 1999/03/10 02:32:01 heller Exp $
____________________________________________________________________________*/
#ifndef CCODERREGISTRY_H
#define CCODERREGISTRY_H

#include "LThread.h"

#include "PGPFone.h"
#include "gsm.h"

class CControlThread;

#define MAXCODERTIMETHRESHOLD		600

typedef struct CoderEntry
{
	ulong	code;				//	4 character code to identify coder internally
	char	name[32];			//	C string name of coder including sample rate
	ulong	encTime, decTime;	//	milliseconds required to encode/decode 1 second of sample rate in
	short	reqSpeed;			// 0:9600, 1:14400, 2:28800, 3: > 28800
	short	reqcps;				// characters required per second
} CoderEntry;

class CCoderRegistry	:	public LThread
{
public:
					CCoderRegistry(CControlThread *controlThread, void **outResult);
					~CCoderRegistry();
	void			*Run(void);
private:
	CoderEntry		*mCoders;		// list of coders sorted by CPU time required
	short			mNumCoders;
	CControlThread	*mControlThread;
	short			*mSampleBuf;
	uchar			*mSampleCompBuf;
	
	gsm				mGSM;
};

#endif

