/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: LSemaphore.cpp,v 1.3 1999/03/10 02:44:04 heller Exp $
____________________________________________________________________________*/
#include "LSemaphore.h"
#include "PGPFoneUtils.h"

LSemaphore::LSemaphore(long initialCount, long maxCount)
{
	pgpAssert(initialCount >= 0);
	pgpAssert(maxCount >= 0);

	if (!(mSemaphore = CreateSemaphore(NULL, initialCount, maxCount, NULL)))
		pgp_errstring("Error creating semaphore");
}

LSemaphore::~LSemaphore()
{
	CloseHandle(mSemaphore);
}

short
LSemaphore::Wait(long milliSeconds)
{
	switch (WaitForSingleObject(mSemaphore, milliSeconds))
	{
		case WAIT_OBJECT_0:
			return 0;		// no error

		case WAIT_TIMEOUT: 
			return errSemaphoreTimedOut;

		default:
			pgp_errstring("LSemaphore::Wait() unknown return value");
			return 0;		// this isn't right, but it won't get here
	}
}

void	LSemaphore::Signal(void)
{
	ReleaseSemaphore(mSemaphore, 1, NULL);
}

