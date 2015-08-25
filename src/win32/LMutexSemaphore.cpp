/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: LMutexSemaphore.cpp,v 1.3 1999/03/10 02:44:01 heller Exp $
____________________________________________________________________________*/
#include "LMutexSemaphore.h"
#include "PGPFoneUtils.h"

LMutexSemaphore::LMutexSemaphore(void) 
{
	if (!(mMutex = CreateMutex(NULL, 0, NULL)))
		pgp_errstring("LMutexSemaphore::LMutexSemaphore - error creating mutex");
}

LMutexSemaphore::~LMutexSemaphore(void)
{
	CloseHandle(mMutex);
}

	short
LMutexSemaphore::Wait(long milliSeconds)
{
	ulong	result;

	switch (result = WaitForSingleObject(mMutex, INFINITE))
	{
		case WAIT_OBJECT_0:
			return 0;

		case WAIT_ABANDONED:
			return errSemaphoreTimedOut;	// we own it now, but it's unsignaled

		case WAIT_FAILED:
			pgp_errstring("LMutexSemaphore::Wait wait failed");

		default:
			char	str[100];
			sprintf(str, "LMutexSemaphore::Wait unknown return value %08X %p", result, this);
			pgp_errstring(str);
			return 0;
	}
}

	void
LMutexSemaphore::Signal(void)
{
	ReleaseMutex(mMutex);
}

StMutex::StMutex(LMutexSemaphore &mutex)
			: mMutex(mutex)
{
	mMutex.Wait();
}

