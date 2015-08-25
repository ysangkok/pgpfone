/*____________________________________________________________________________
	Copyright (C) 1997 Network Associates Inc. and affiliated companies.
	All rights reserved.

	$Id: CPriorityQueue.cpp,v 1.3 1999/03/10 02:39:13 heller Exp $
____________________________________________________________________________*/
#include "CPriorityQueue.h"
#include "CMessageQueue.h"
#include "PGPFoneUtils.h"
#ifdef _WIN32
#include "LMutexSemaphore.h"
#include "LSemaphore.h"
#endif

CPriorityQueue::CPriorityQueue(short numQueues)
{
	short inx;
	
	mNumQueues = numQueues;
	mQueues = (CMessageQueue **)pgp_malloc(numQueues * sizeof(CMessageQueue *));
	pgpAssert(mQueues);
	for(inx = 0;inx<numQueues;inx++)
		mQueues[inx] = NIL;
#ifdef _WIN32
  	mAvailable = new LSemaphore(0, 64);
#else
  	mAvailable = new LSemaphore();
#endif
}

CPriorityQueue::~CPriorityQueue()
{
	delete mAvailable;
	pgp_free(mQueues);
}

void
CPriorityQueue::AddQueue(short priority, CMessageQueue *queue)
{
	mQueues[priority] = queue;
	queue->SetPrioritySem(mAvailable);
}

CMessageQueue *
CPriorityQueue::Recv(long milliseconds)
{
	short inx, err;
	
	err = mAvailable->Wait(milliseconds);
	if(err)
		return NIL;
	for(inx=0;inx<mNumQueues;inx++)
	{
		if(mQueues[inx] && mQueues[inx]->Peek())
			return mQueues[inx];
	}
	return NIL;
}

