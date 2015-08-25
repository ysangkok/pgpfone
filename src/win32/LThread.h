/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: LThread.h,v 1.4 1999/03/10 02:44:08 heller Exp $
____________________________________________________________________________*/
#ifndef LTHREAD_H
#define LTHREAD_H

#include "PGPFone.h"

#define THREAD_DEBUG		1

class LSemaphore;

class LThread
{
public:
						LThread(void **outResult);
	virtual				~LThread(void);
	virtual void		DeleteThread(void *inResult = NULL);
	static void			(Yield)(const LThread *yieldTo = NULL);
	static void			EnterCritical(void);
	static void			ExitCritical(void);
	static LThread		*GetCurrentThread(void);
	virtual void		Suspend(void);
	virtual void		Resume(void);
	virtual void		Sleep(long milliSeconds);
	virtual void		Wake(void);
	void				*GetResult(void) const;
//	virtual int			Run(void);
	virtual void		*Run(void);
	void				SetResult(void *inResult);
		
protected:

	HANDLE		mThreadHandle;	// handle to this thread for Win32 API calls
	DWORD		mThreadID;		// Win32 ID for this thread
	void		**mResult;		// the thread's return value
};

#endif

