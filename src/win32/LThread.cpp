/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: LThread.cpp,v 1.4 1999/03/10 02:44:07 heller Exp $
____________________________________________________________________________*/
#include <process.h>
#include "LThread.h"
#include "PGPFone.h"
#include "PGPFoneUtils.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// this is a helper function for the LThread constructor.  when creating the
//  thread we need to specify a starting address and there is no easy way to
//  specify the starting address of a object function (because of the implicit
//  passing of the this pointer as a parameter).  therefore, this function takes
//  a pointer to an LThread object and invokes its Run member function.

static
DWORD WINAPI ThreadFunction(LPVOID lpvThreadParam)
{
	void		*result;
	LThread		*pThread = (LThread*)lpvThreadParam;

	// run the thread
	result = pThread->Run();

	// store the return value
	pThread->SetResult(result);


	delete pThread;
 	_endthreadex(0);

	return 0;
}

LThread::LThread(void **outResult)
{
	HANDLE		hThread;

	mResult = outResult;

	// create the thread suspended.  the person who created this object will
	// then have to call resume thread to make it run again.
	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int (__stdcall *)(LPVOID))ThreadFunction,
							(void *)this, CREATE_SUSPENDED, (unsigned int *)&mThreadID);


	if (!hThread) 
	//if (!(hThread = CreateThread(NULL, 0, ThreadFunction, this, CREATE_SUSPENDED,
	//	&mThreadID)))
	{
		char	error[30];
		sprintf(error, "error creating thread %ld", GetLastError());
		pgp_errstring(error);
	}
	else if (mResult)
		*mResult = NULL;

	// initialize mThreadHandle so any thread can call thread functions for
	//  this thread using this object.
	// this is totaly dumb wkm
	// DuplicateHandle(GetCurrentProcess(), hThread,
	//				GetCurrentProcess(), &mThreadHandle,
	//				0, 0, DUPLICATE_SAME_ACCESS);
	mThreadHandle = hThread;
}

LThread::~LThread(void)
{
	// _endthreadex dosen't close the thread handle
	CloseHandle(mThreadHandle);
}

void
LThread::Wake(void)
{
	pgp_errstring("LThread::Wake didn't expect to be called");
}

LThread *
LThread::GetCurrentThread(void)
{
	pgp_errstring("LThread::GetCurrentThread didn't expect to be called");
	return NULL;
}

void
LThread::DeleteThread(void *inResult)
{
	SetResult(inResult);
	/* try to cleanly dispose of the thread, call the thread's destructor */
}

void *
LThread::Run(void)
{
	//	This is a pure virtual function in LThread, so you MUST override it in 
	//	your derived classes.  It is implemented here only as a debugging aid.
	pgp_errstring("Entering LThread::Run() -- this should never happpen");
	return (NULL);
}

void
(LThread::Yield)(const LThread *yieldTo)
{
	// in Win32 you cannot yield to another thread so we will just give
	// up our time-slice.
	::Sleep(0);
}

void
LThread::EnterCritical(void)
{
	// the Win32 API implements critical sections a little bit differently.
	// thus, the programmer really doesn't need to call this function.
	pgp_errstring("LThread::EnterCritical() someone used this function");
}

void
LThread::ExitCritical(void)
{
	// the Win32 API implements critical sections a little bit differently.
	// thus, the programmer really doesn't need to call this function.
	pgp_errstring("LThread::ExitCritical() someone used this function");
}

void
LThread::Suspend(void)
{
	// put this thread to sleep
	if (SuspendThread(mThreadHandle) == 0xFFFFFFFF)
		pgp_errstring("LThread::Suspend error suspending thread");
}

void
LThread::Resume(void)
{
	// wake this thread up again.  someone else will call this probably
	if (ResumeThread(mThreadHandle) == 0xFFFFFFFF)
		pgp_errstring("LThread::Resume error resuming thread");
}

void
LThread::Sleep(long milliSeconds)
{
	// put the call to sleep for milliSeconds time
	::Sleep(milliSeconds);
}

void *
LThread::GetResult(void) const			{	return (mResult);		}

void
LThread::SetResult(void *inResult)
{
	if (mResult)
		*mResult = inResult;
}

