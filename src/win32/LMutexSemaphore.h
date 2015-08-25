/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: LMutexSemaphore.h,v 1.4 1999/03/10 02:44:02 heller Exp $
____________________________________________________________________________*/
#ifndef LMUTEXSEMAPHORE_H
#define LMUTEXSEMAPHORE_H

#include "LSemaphore.h"

class LMutexSemaphore;

class LMutexSemaphore : public LSemaphore 
{
public:
					LMutexSemaphore(void);
					LMutexSemaphore(LMutexSemaphore &mutex)
						{	mMutex = mutex.mMutex;	}
	virtual			~LMutexSemaphore(void);
	
	inline virtual void		Signal(void);
	inline virtual short	Wait(long milliSeconds = semaphore_WaitForever);
private:
	HANDLE		mMutex;		// mutex handle for Win32 API calls
};

class StMutex : public LMutexSemaphore	// stack-based mutex
{
public:
				StMutex(LMutexSemaphore &mutex);
	virtual		~StMutex(void)
					{	mMutex.Signal();	}
private:
	LMutexSemaphore	&mMutex;
};

#endif

