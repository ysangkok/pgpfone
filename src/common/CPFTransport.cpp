/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFTransport.cpp,v 1.3 1999/03/10 02:36:16 heller Exp $
____________________________________________________________________________*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "CPFTransport.h"
#include "CPFWindow.h"

CPFTransport::CPFTransport(CPFWindow *cpfWindow)
{
	mTMutex = new LMutexSemaphore;
	mAbort = mAnswer = FALSE;
	mPFWindow = cpfWindow;
	SetState(_cs_uninitialized);
#ifdef PGP_WIN32
	mAbortEvent = mAnswerEvent = INVALID_HANDLE_VALUE;

	// we want abort to be a manual-reset event.
	// Can't do this windows 95 bug
	mAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (mAbortEvent == INVALID_HANDLE_VALUE)
	{
		pgp_errstring("error creating abort event");
		return;
	}
	// we want answer to be an auto-reset event.
	mAnswerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (mAnswerEvent == INVALID_HANDLE_VALUE)
	{
		pgp_errstring("error creating answer event");
		return;
	}
#endif
}

CPFTransport::~CPFTransport()
{
	AbortSync();
	mTMutex->Wait(semaphore_WaitForever);
	mPFWindow->SetState(_cs_uninitialized);
	delete mTMutex;
#ifdef PGP_WIN32
	if(mAbortEvent != INVALID_HANDLE_VALUE)
		CloseHandle(mAbortEvent);
	if(mAnswerEvent != INVALID_HANDLE_VALUE)
		CloseHandle(mAnswerEvent);
#endif
}

void
CPFTransport::AbortSync()
{
	mAbort = TRUE;
#ifdef PGP_WIN32
	if(mAbortEvent != INVALID_HANDLE_VALUE)
		SetEvent(mAbortEvent);
#endif
}

void
CPFTransport::Answer()
{
	mAnswer = TRUE;
#ifdef PGP_WIN32
	if(mAnswerEvent != INVALID_HANDLE_VALUE)
		SetEvent(mAnswerEvent);
#endif
}

PGErr
CPFTransport::Flush()
{
	// We provide this function here because many subclasses
	// probably will do nothing here, so it is not necessary
	// to override it.
	return 0;
}

PGErr
CPFTransport::BindMediaStreams()
{
	// We provide this function here because many subclasses
	// probably will do nothing here, so it is not necessary
	// to override it.
	return 0;
}

void
CPFTransport::SetState(enum CSTypes newState)
{
	mState=newState;
	mPFWindow->SetState(mState);
}

enum CSTypes
CPFTransport::GetState()
{
	return mState;
}

/* remember not to use Write to write data		*/
/* it follows the characteristics of a printf	*/
/* and will stop at nulls, etc..				*/

PGErr
CPFTransport::Write(char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	long len;
	
	va_start(ap, fmt);
	len = vsprintf(buf, fmt, ap);
	va_end(ap);
	pgpAssert(len <= 1024);
	WriteBlock(buf, &len, _pfc_Control);
	return _pge_NoErr;
}

/*	The following 2 functions manage AsyncTransport structs.
	These allow us to handle an arbitrary number of outstanding
	asynchronous (i.e. non-blocking) calls.  This feature helps
	keep throughput near maximum on a given transport.  If
	a transport layer does not support this feature, it doesn't
	really matter because it can just decide to block.
	WriteAsync does block when it is sent an AsyncTransport which
	is currently in use.  To initialize an AsyncTransport, call
	NewAsync, and delete it with DeleteAsync.	*/

Boolean
CPFTransport::NewAsync(AsyncTransport *asyncTrans)
{
	memset(asyncTrans, 0, sizeof(AsyncTransport));
	asyncTrans->mutex = new LMutexSemaphore();
	if(asyncTrans->mutex)
	{
#ifdef __MC68K__
		asyncTrans->savedA5 = (long)LMGetCurrentA5();
#endif
#ifdef PGP_WIN32
		memset(&asyncTrans->u.overlapped, 0, sizeof(OVERLAPPED));
		// WriteFileEx won't use the hEvent field
		asyncTrans->u.overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
#endif
		return TRUE;
	}
	return FALSE;
}

void
CPFTransport::DeleteAsync(AsyncTransport *asyncTrans)
{
	if(asyncTrans)
	{
#ifdef PGP_WIN32
		CloseHandle(asyncTrans->u.overlapped.hEvent);
#endif
		delete asyncTrans->mutex;
		if(asyncTrans->buffer)
			safe_free(asyncTrans->buffer);
		asyncTrans->buffer = NIL;
	}
}

void
CPFTransport::WaitAsync(AsyncTransport *asyncTrans)
{
	if(asyncTrans->buffer)
		safe_free(asyncTrans->buffer);
	asyncTrans->buffer = NIL;
}

