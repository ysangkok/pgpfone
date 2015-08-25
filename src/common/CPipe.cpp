/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPipe.cpp,v 1.6 1999/03/10 02:38:54 heller Exp $
____________________________________________________________________________*/
#include "CPipe.h"
#include "PGPFoneUtils.h"

#include "LThread.h"

#define MAXPIPESIZE		32768L		// This is temporary
#define MINPIPEEXTRA	1024L

CPipe::CPipe(long pipeSize, long pipeExtra)
{
	mData=NIL;
	if(pipeSize>MAXPIPESIZE)
		pgp_errstring("CPipe: Pipe too big");
	if(pipeExtra<MINPIPEEXTRA)
		pipeExtra=MINPIPEEXTRA;
	if(mData=(char *)pgp_malloc(pipeSize+pipeExtra))
	{
		mPusher = mPuller = NIL;
		mPipeSize=pipeSize;
		mPipeExtra=pipeExtra;
		ResetPipe();
	}
	pgpAssert(mData);
}

void
CPipe::SetPusher(LThread *pusher)
{
	mPusher = pusher;
}

void
CPipe::SetPuller(LThread *puller)
{
	mPuller = puller;
}

CPipe::~CPipe()
{
	if(mData)
	{
		pgp_free(mData);
		mData=NIL;
	}
}

void
CPipe::ResetPipe(void)
{
	mSem.Wait();
	mPushBytes=mPullBytes=0;
	mPushOffset=mPullOffset=0;
	mPushState=mPullState=_pps_none;
	mIndex=mOutdex=0;
	mStart[0]=mEnd[0]=0;
	mStart[1]=mEnd[1]=mPipeSize;
	mTotalBytes=mPipePos=0;
	mSem.Signal();
}

void
CPipe::DoStartPipePush(long len, void **data)
{
	register short i, o;
	long offset;
	
	mSem.Wait();
again:
	offset=0;
	switch(mPushState)
	{
		case _pps_none:
			i=mIndex;
			o=mOutdex;
			offset=mEnd[i];
			mPushBytes=len;
			if(offset+len>mStart[!i])
			{
				if(i!=o || len>mStart[i])
				{
					mPushState=_pps_pushWait;
					mSem.Signal();
					mPushWait.Wait();
					mSem.Wait();
					if(mPushState == _pps_pushWait)
						mPushState = _pps_none;
					goto again;
				}
				else
					offset=0;
			}
			mPushState=_pps_pushing;
			mPushOffset=offset;
			break;
		case _pps_pushWait:
			pgp_errstring("DoStartPipePush: mPushState == _pps_pushWait");
			break;
		case _pps_pushing:
			pgp_errstring("DoStartPipePush: Currently pushing");
			break;
		case _pps_posChanged:
		case _pps_aborted:
			break;
		case _pps_pullWait:
		case _pps_pulling:
		case _pps_endWait:
		default:
			pgp_errstring("DoStartPipePush: mPushState invalid");
			break;
	}
	*data=mData+offset;
	mSem.Signal();
}


void
CPipe::DoEndPipePush(long len)
{
	register short i, o;
	
	mSem.Wait();
	switch(mPushState)
	{
		case _pps_none:
			pgp_errstring("DoEndPipePush: Didn't call DoStartPipePush");
			break;
		case _pps_pushing:
			i=mIndex;
			o=mOutdex;
			mPushState=_pps_none;
			if(len<0)
				len=mPushBytes;
			if(len>mPushBytes)
				pgp_errstring("DoEndPipePush: Attempt to resize to larger than original request");
			if(i==o && mPushOffset==0 && mEnd[i]!=0)
			{
				i=mIndex=!i;
				if(mStart[i]!=mPipeSize || mEnd[i]!=mPipeSize)
					pgp_errstring("DoEndPipePush: p->start/end[i]!=mPipeSize");
				mStart[i]=mEnd[i]=0;
			}
			if(mPushOffset!=mEnd[i])
				pgp_errstring("DoEndPipePush: mPushOffset!=mEnd[i]");
			mEnd[i]+=len;
			mTotalBytes+=len;
			if(mPullState==_pps_pullWait
					&& (mTotalBytes>=mPullBytes
						|| (mFlexPull && mTotalBytes>0 && i!=o)))
			{
				mPullState=_pps_none;
				mPullWait.Signal();
			}
			break;
		case _pps_posChanged:
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pulling:
		case _pps_endWait:
		default:
			pgp_errstring("DoEndPipePush: mPushState invalid");
			break;
	}
	mSem.Signal();
}

void
CPipe::DoCancelPipePush(void)
{
	mSem.Wait();
	switch(mPushState)
	{
		case _pps_none:
			pgp_errstring("DoCancelPipePush: Didn't call DoStartPipePush");
			break;
		case _pps_pushing:
			mPushState=_pps_none;
			break;
		case _pps_posChanged:
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pulling:
		case _pps_endWait:
		default:
			pgp_errstring("DoCancelPipePush: mPushState invalid");
			break;
	}
	mSem.Signal();
}

OSErr
CPipe::DoEndPipe(long *pos)	// Called by push process
{
	OSErr result;
	
	mSem.Wait();
again:
	result=0;
	switch(mPushState)
	{
		case _pps_none:
			mPushState=_pps_endWait;
			if(mPullState==_pps_pullWait)
			{
				mPullState=_pps_none;
				mPullWait.Signal();
			}
			if(mPushState==_pps_endWait)
			{
				mSem.Signal();
				mPushWait.Wait();
				mSem.Wait();
			}
			goto again;
			break;
		case _pps_pushing:
			pgp_errstring("DoEndPipe: Called while pushing");
			break;
		case _pps_posChanged:
			result=1;
			*pos=mPipePos;
			mPushState=_pps_none;
			break;
		case _pps_aborted:
		//	ResetPipe(pipe);
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pulling:
		case _pps_endWait:
		default:
			pgp_errstring("DoEndPipe: mPushState invalid");
			break;
	}
	mSem.Signal();
	return result;
}

void
CPipe::DoStartPipePull(long *len, void **data, short flex)
{
	register short i, o;
	long reqLen, offset, extraNeeded, temp;
	
	reqLen=*len;
	mSem.Wait();
again:
	offset=0;
	*len=0;
	switch(mPullState)
	{
		case _pps_none:
			i=mIndex;
			o=mOutdex;
			*len=reqLen;
			if(i!=o && mStart[o]>=mEnd[o])
			{
				mStart[o]=mEnd[o]=mPipeSize;
				o=mOutdex=i;
			}
			temp=mEnd[o]-mStart[o];
			if(flex && i!=o && *len>temp)
				*len=mEnd[o]-mStart[o];
			if(mPushState==_pps_endWait && reqLen>mTotalBytes)
			{
				*len=mTotalBytes;
			}
			mPullBytes=*len;
			mFlexPull=flex;
			if(mTotalBytes<*len)
			{
				mPullState=_pps_pullWait;
				mSem.Signal();
				mPullWait.Wait();
				mSem.Wait();
				if(mPullState == _pps_pullWait)
					mPullState = _pps_none;
				goto again;
			}
			offset=mStart[o];
			extraNeeded=(offset+*len)-mEnd[o];
			if(extraNeeded>0)
			{
				if(flex)
					pgp_errstring("DoStartPipePull: Have to memcpy, but flex!=0");
				if(mEnd[o]+extraNeeded>mPipeSize+mPipeExtra)
					pgp_errstring("DoStartPipePull: Not enough extra for BlockMove");
				if(i==o)
					pgp_errstring("DoStartPipePull: Have to BlockMove, but i==o");
#ifdef PGP_MACINTOSH
				BlockMoveData(mData+mStart[i], mData+mEnd[o], extraNeeded);
#elif PGP_WIN32
				memmove(mData+mEnd[o], mData+mStart[i], extraNeeded);
#endif
			}
			mPullState=_pps_pulling;
			mPullOffset=offset;
			break;
		case _pps_pullWait:
			pgp_errstring("DoStartPipePull: mPullState == _pps_pullWait");
			break;
		case _pps_pulling:
			pgp_errstring("DoStartPipePull: Currently pulling");
			break;
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pushing:
		case _pps_endWait:
		case _pps_posChanged:
		default:
			pgp_errstring("DoStartPipePull: mPullState invalid");
			break;
	}
	*data=mData+offset;
	mSem.Signal();
}

void
CPipe::DoEndPipePull(long len)
{
	register short i, o;
	long left;
	
	mSem.Wait();
	switch(mPullState)
	{
		case _pps_none:
			pgp_errstring("DoEndPipePull: Didn't call DoStartPipePull");
			break;
		case _pps_pulling:
			i=mIndex;
			o=mOutdex;
			mPullState=_pps_none;
			if(len<0)
				len=mPullBytes;
			if(len>mPullBytes)
				pgp_errstring("DoEndPipePull: Attempt to resize to larger than original request");
			left=len;
			if(left>0 && i!=o)
			{
				mStart[o]+=left;
				if(mStart[o]>=mEnd[o])
				{
					left=mStart[o]-mEnd[o];
					mStart[o]=mEnd[o]=mPipeSize;
					o=mOutdex=i;
				}
				else
					left=0;
			}
			mStart[o]+=left;
			mTotalBytes-=len;
			mPipePos+=len;
			if(mPushState==_pps_pushWait)
			{
				if(mEnd[i]+mPushBytes<=mStart[!i] || (i==o && mPushBytes<=mStart[i]))
				{
					mPushState=_pps_none;
					mPushWait.Signal();
				}
			}
			break;
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pushing:
		case _pps_endWait:
		case _pps_posChanged:
		default:
			pgp_errstring("DoEndPipePull: mPullState invalid");
			break;
	}
	mSem.Signal();
}

void
CPipe::DoCancelPipePull(void)
{
	mSem.Wait();
	switch(mPullState)
	{
		case _pps_none:
			pgp_errstring("DoCancelPipePull: Didn't call DoStartPipePull");
			break;
		case _pps_pulling:
			mPullState=_pps_none;
			break;
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pushing:
		case _pps_endWait:
		case _pps_posChanged:
		default:
			pgp_errstring("DoCancelPipePull: mPullState invalid");
			break;
	}
	mSem.Signal();
}

short CPipe::DoPipeEnded(void)
{
	short result;
	
	mSem.Wait();
	result=(mPushState==_pps_aborted || (mPushState==_pps_endWait && mTotalBytes==0));
	mSem.Signal();
	return result;
}

void
CPipe::DoAbortPipe(void)
{
	mSem.Wait();
	mPushState=mPullState=_pps_aborted;
	mPushWait.Signal();
	mPullWait.Signal();
	mSem.Signal();
}

short CPipe::DoPipeAborted(void)
{
	short result;
	
	mSem.Wait();
	result=(mPushState==_pps_aborted);
	mSem.Signal();
	return result;
}

void
CPipe::DoAckEndPipe(void)	// Called by pull process
{
	long len;
	char *data;
	
	do
	{
		len=1024;
		DoStartPipePull(&len, (void **)&data, 0);
		DoEndPipePull(len);
	} while(len>0);
	mSem.Wait();
	switch(mPullState)
	{
		case _pps_none:
			if(mPushState!=_pps_aborted)
			{
				if(mPushState!=_pps_endWait)
					pgp_errstring("DoAckEndPipe: mPushState!=_pps_endWait");
				mPushState=mPullState=_pps_aborted;
				mPushWait.Signal();
			}
			break;
		case _pps_pulling:
			pgp_errstring("DoAckEndPipe: Currently pulling");
			break;
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pushing:
		case _pps_endWait:
		case _pps_posChanged:
		default:
			pgp_errstring("DoAckEndPipe: mPullState invalid");
			break;
	}
	mSem.Signal();
}

void
CPipe::DoSetPipePos(long pos)	// Called by pull process
{
	mSem.Wait();
	switch(mPullState)
	{
		case _pps_none:
			if(mPipePos!=pos)
			{
				mPipePos=pos;
				mIndex=mOutdex=0;
				mStart[0]=mEnd[0]=0;
				mStart[1]=mEnd[1]=mPipeSize;
				mTotalBytes=0;
				if(mPushState==_pps_pushWait || mPushState==_pps_endWait)
					mPushWait.Signal();
				mPushState=_pps_posChanged;
			}
			break;
		case _pps_pulling:
			pgp_errstring("DoSetPipePos: Called while pulling");
			break;
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pushing:
		case _pps_endWait:
		case _pps_posChanged:
		default:
			pgp_errstring("DoSetPipePos: mPullState invalid");
			break;
	}
	mSem.Signal();
}

short
CPipe::DoPipePosChanged(long *pos)	// Called by push process
{
	short result;
	
	result=0;
	mSem.Wait();
	switch(mPushState)
	{
		case _pps_none:
			break;
		case _pps_pushing:
			pgp_errstring("DoPipePosChanged: Called while pushing");
			break;
		case _pps_posChanged:
			result=1;
			*pos=mPipePos;
			mPushState=_pps_none;
			break;
		case _pps_aborted:
			break;
		case _pps_pushWait:
		case _pps_pullWait:
		case _pps_pulling:
		case _pps_endWait:
		default:
			pgp_errstring("DoPipePosChanged: mPushState invalid");
			break;
	}
	mSem.Signal();
	return result;
}

