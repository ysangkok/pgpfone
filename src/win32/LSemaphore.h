/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: LSemaphore.h,v 1.4 1999/03/10 02:44:05 heller Exp $
____________________________________________________________________________*/
#ifndef LSEMAPHORE_H
#define LSEMAPHORE_H

#include "PGPfone.h"

#define semaphore_WaitForever	-1
#define semaphore_NoWait		0

// error codes
enum {
	errSemaphoreDestroyed = 28020, 
	errSemaphoreTimedOut,
	errSemaphoreNotOwner,
	errSemaphoreAlreadyReset
};

class LSemaphore
{
public:
	// constructors / destructors
					LSemaphore(long initialCount = 0, long maxCount = 0x7FFFFFFF);
	virtual			~LSemaphore();
	
	virtual void	Signal(void);
	virtual short	Wait(long milliSeconds = semaphore_WaitForever);
private:
	HANDLE		mSemaphore;	// semaphore handle for Win32 API calls
};

#endif

