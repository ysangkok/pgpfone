/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPipe.h,v 1.6 1999/03/10 02:38:55 heller Exp $
____________________________________________________________________________*/
#ifndef CPIPE_H
#define CPIPE_H

#include "LThread.h"
#include "LMutexSemaphore.h"
#include "LSemaphore.h"

#ifdef PGP_WIN32
typedef long OSErr;
#endif

enum PipeStates {
					_pps_none,
					_pps_pushWait,
					_pps_pullWait,
					_pps_pushing,
					_pps_pulling,
					_pps_endWait,
					_pps_posChanged,
					_pps_aborted
				};

class CPipe
{
public:
					CPipe(long pipeSize, long pipeExtra);
					~CPipe();
	void			SetPusher(LThread *pusher);
	void			SetPuller(LThread *puller);
	void			ResetPipe(void);
	void			DoStartPipePush(long len, void **data);
	void			DoEndPipePush(long len);
	void			DoCancelPipePush(void);
	OSErr			DoEndPipe(long *pos);			// Called by push process
	void			DoStartPipePull(long *len, void **data, short flex);
	void			DoEndPipePull(long len);
	void			DoCancelPipePull(void);
	short			DoPipeEnded(void);
	void			DoAbortPipe(void);
	short			DoPipeAborted(void);
	void			DoAckEndPipe(void);				// Called by pull process
	void			DoSetPipePos(long pos);			// Called by pull process
	short			DoPipePosChanged(long *pos);	// Called by push process
	void			DoDisposePipe(void);
private:
	LMutexSemaphore	mSem;
	LSemaphore		mPushWait, mPullWait;
	LThread			*mPusher, *mPuller;
	long			mPushBytes, mPullBytes;
	long			mPushOffset, mPullOffset;
	short			mPushState, mPullState, mFlexPull;
	short			mIndex, mOutdex;
	long			mStart[2], mEnd[2];
	long			mPipeSize, mPipeExtra;
	long			mTotalBytes, mPipePos;
	char			*mData;
};


#endif

