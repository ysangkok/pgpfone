/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CWinFilePipe.h,v 1.11 1999/03/10 02:42:02 heller Exp $
____________________________________________________________________________*/
#ifndef CWINFILEPIPE_H
#define CWINFILEPIPE_H

#ifdef	PGP_WIN32
#include "LMutexSemaphore.h"
#include "LThread.h"
#endif
#include "PGPFone.h"

//#define DEBUGXFERLOG

#define STDPIPESIZE		(24*1024L)
#define STDPIPEEXTRA	1024L

class CPipe;

typedef struct XferFileSpec
{
	char path[MAX_PATH];
	char* root;
	char* filename;
} XferFileSpec;

typedef struct XferInfo
{
	CPipe *pipe;
	char filepath[MAX_PATH];	// always *full* path of file
	char* filename;
	ulong bytesTotal, bytesDone, cpsBase;
	ulong startTime, lastTime;
	short forceUpdate, setEOF;
	short retries, sendAs, resurrect;
	short fatalErr;
} XferInfo;

class CWinFileSendPipe	:	public LThread
{
public:
					CWinFileSendPipe(XferInfo *xi, Boolean recovery, void **outResult);
	virtual			~CWinFileSendPipe(){}
	void			*Run(void);
protected:
	long			ReadFork(HANDLE fileRef, long offset, long start, long end);
	XferInfo		*mXI;
	Boolean 		mRecovery;
};

class CWinFileReceivePipe	:	public LThread
{
public:
					CWinFileReceivePipe(XferInfo *xi, void **outResult);
	virtual			~CWinFileReceivePipe(){}
	void			*Run(void);
protected:
	void			WriteFork(HANDLE fileRef, long pos, long end);
	XferInfo		*mXI;
};

void CheckSendMethod(LThread *t, XferInfo *xFile);

#endif


