/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CWinFilePipe.cpp,v 1.5 1999/03/10 02:41:57 heller Exp $
____________________________________________________________________________*/

#include "PGPFoneUtils.h"
#include "CWinFilePipe.h"
#include "CPipe.h"

#include <string.h>

#define READPICKUPSIZE 4096
#define WRITEPICKUPSIZE 8192


static void LogTransfer(XferInfo *xFile, short send, short good)
{

}

// The purpose of CheckSendMethod is really just to set the bytesTotal value
// and to make sure the file exists.

void CheckSendMethod(LThread *t, XferInfo *xFile)
{
	HANDLE f;

	f = CreateFile(xFile->filepath, 0 /* no actual open */, 0, NIL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NIL);
	if(f != INVALID_HANDLE_VALUE)
	{
		xFile->bytesTotal = GetFileSize(f, NIL);
		CloseHandle(f);
	}
	else
		xFile->fatalErr = -1;
}

CWinFileSendPipe::CWinFileSendPipe(XferInfo *xi, Boolean recovery, void **outResult)
        : LThread(outResult)
{
	mXI = xi;
	mRecovery = recovery;
}

CWinFileReceivePipe::CWinFileReceivePipe(XferInfo *xi, void **outResult)
        : LThread(outResult)
{
	mXI = xi;
}

void
CWinFileReceivePipe::WriteFork(HANDLE fileRef, long pos, long end)
{
	long count;
	char *data;
	
	while(!mXI->pipe->DoPipeEnded() && (pos<end || end<0))
	{
		if(end<0 || (count=end-pos)>WRITEPICKUPSIZE)
			count=WRITEPICKUPSIZE;
		mXI->pipe->DoStartPipePull(&count, (void **)&data, 1);
#ifdef DEBUGXFERLOG
		DebugLog("WriteFork: Wrote %d", count);
#endif
		if(!WriteFile(fileRef, data, count, (ulong *)&count, NIL))
			mXI->fatalErr=-1;
		else
			FlushFileBuffers(fileRef);
		mXI->pipe->DoEndPipePull(-1);
		pos+=count;
	}
}

void *
CWinFileReceivePipe::Run()
{
	long dataLen=0, startData=0;
	short good=0;
	HANDLE dataRef;
	
	dataRef = CreateFile(mXI->filepath, GENERIC_WRITE, 0, NIL, OPEN_ALWAYS,
							FILE_FLAG_WRITE_THROUGH, NIL);
	if(dataRef == INVALID_HANDLE_VALUE)
		goto error;
	if(!mXI->pipe->DoPipeEnded())
	{
		WriteFork(dataRef, startData, -1);
		if(mXI->bytesTotal && !mXI->fatalErr && mXI->setEOF)
			SetEndOfFile(dataRef);
		CloseHandle(dataRef);
		if(!mXI->pipe->DoPipeAborted())
		{
			good=1;
		}
		LogTransfer(mXI, 0, good);
	}
	mXI->pipe->DoAckEndPipe();
	return (void *)1L;
error:
	mXI->fatalErr=1;	// needs improved error checking
	mXI->pipe->DoAbortPipe();
	return (void *)1L;
}

long
CWinFileSendPipe::ReadFork(HANDLE fileRef, long offset, long start, long end)
{
	long count, pos=start, padRead;
	char *data;
	register short i;
	
	while(!mXI->pipe->DoPipeEnded() && pos<end)
	{
		if(mXI->pipe->DoPipePosChanged(&count))
		{
			pos=count-offset;
			if(pos<0 || pos>=end)
				return count;
		}
		if((count=end-pos)>READPICKUPSIZE)
			count=READPICKUPSIZE;
		padRead=count;
		mXI->pipe->DoStartPipePush(count, (void **)&data);
		if(!ReadFile(fileRef, data, count, (ulong *)&count, NIL))
			mXI->fatalErr = 1;
		if(padRead>count)
			for(i=count;i<padRead;i++)
				data[i]=0;
		pos+=padRead;
		mXI->pipe->DoEndPipePush(-1);
	}
	return -1;
}

void *
CWinFileSendPipe::Run()
{
	HANDLE fileRef;
	long pipePos=0;
	
	fileRef = CreateFile(mXI->filepath, GENERIC_READ, FILE_SHARE_READ, NIL, OPEN_EXISTING, 0, NIL);
	if(fileRef == INVALID_HANDLE_VALUE)
		goto error;
startSend:
	SetFilePointer(fileRef, pipePos, NIL, FILE_BEGIN); // ***** check this
	ReadFork(fileRef, 0, 0, mXI->bytesTotal);
	CloseHandle(fileRef);
	if(mXI->pipe->DoEndPipe(&pipePos))
		goto startSend;
	LogTransfer(mXI, 1, (mXI->fatalErr==0));
	return (void *)1L;
error:
	mXI->fatalErr=1;
	mXI->pipe->DoAbortPipe();
	return (void *)1L;
}

