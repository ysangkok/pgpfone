/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPriorityQueue.h,v 1.5 1999/03/10 02:39:17 heller Exp $
____________________________________________________________________________*/
#ifndef CPRIORITYQUEUE_H
#define CPRIORITYQUEUE_H

#include "LSemaphore.h"
#include "LMutexSemaphore.h"

class CMessageQueue;
					
class CPriorityQueue
{
public:
					CPriorityQueue(short numPriorities);
					~CPriorityQueue();
			
	CMessageQueue	*Recv(long milliseconds = semaphore_WaitForever);
	void			AddQueue(short priority, CMessageQueue *queue);
	
protected:
	short			mNumQueues;
	CMessageQueue	**mQueues;
	LSemaphore		*mAvailable;	// increments when packet is avail in any q
};

#endif

