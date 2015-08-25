/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CMessageQueue.cpp,v 1.4 1999/03/10 02:35:14 heller Exp $
____________________________________________________________________________*/
#include "CMessageQueue.h"
#include "CPriorityQueue.h"
#include "PGPFoneUtils.h"
#include "LMutexSemaphore.h"
#include "LSemaphore.h"

#if PGP_MACINTOSH
#include <UThread.h>
#endif

/*	This code is needed to provide an interrupt safe method
	of sending messages between processes which do not
	require allocation of system memory.  Standard
	producer-consumer functions are provided.
*/

CMessageQueue::CMessageQueue(short maxMessages) : LMutexSemaphore()
{
	mPrioritySem = NIL;
	mMaxMessages = maxMessages;
	mMsgs = (PFMessage *)pgp_malloc(mMaxMessages * sizeof(PFMessage));
	pgpAssert(mMsgs);
	for(short inx = 0;inx<mMaxMessages;inx++)
	{
		mMsgs[inx].next = inx + 1;
		mMsgs[inx].num = inx;
	}
	mMsgs[mMaxMessages -1].next = -1;
	mFreeHead = 0;
	mSendHead = mSendTail = -1;
#ifdef PGP_WIN32
  	mAvailable = new LSemaphore(0, maxMessages);
  	mFreeSpace = new LSemaphore(maxMessages, maxMessages);
#else
  	mAvailable = new LSemaphore();
  	mFreeSpace = new LSemaphore(maxMessages);
#endif
	pgpAssert(mAvailable);
	mSize = 0;
}

CMessageQueue::~CMessageQueue()
{
	FreeAll();
	pgp_free(mMsgs);
	delete mAvailable;
}

void
CMessageQueue::SetPrioritySem(LSemaphore *sem)
{
	mPrioritySem = sem;
}

/*	check to make sure that the type of message being sent
	does not already exist in the queue */

void
CMessageQueue::SendUnique(enum MsgType type, void *data, short len, short safe)
{
	StMutex	mutex(*this);
	short	sendMark;

	for(sendMark = mSendHead; sendMark != -1; sendMark=mMsgs[sendMark].next)
	{
		if(mMsgs[sendMark].type == type)
			return;
	}
	Send(type, data, len, safe);
}

/*	adds the message to the queue if there is space */

void
CMessageQueue::Send(enum MsgType type, void *data, short len, short safe)
{
	mFreeSpace->Wait();
	{
		StMutex	mutex(*this);
		short inx;
		
		if(mFreeHead >= 0)
		{
			inx = mFreeHead;
			mFreeHead = mMsgs[mFreeHead].next;
			if(mSendHead >= 0)
			{
				mMsgs[mSendTail].next = inx;
				mSendTail = inx;
			}
			else
				mSendHead = mSendTail = inx;
			mMsgs[inx].type = type;
			mMsgs[inx].data = data;
			mMsgs[inx].len = len;
			mMsgs[inx].safe = safe;
			mMsgs[inx].next = -1;
			mSize++;
			mAvailable->Signal();
			if(mPrioritySem)
				mPrioritySem->Signal();
		}
	}
}

PFMessage *
CMessageQueue::Recv(long milliseconds)
{
	PFMessage *msg;
	short err;
	
	err = mAvailable->Wait(milliseconds);
	if (err)
		return NIL;

	{
		StMutex	mutex(*this);

		if(mSendHead != -1)
		{
			msg = &mMsgs[mSendHead];
			if((mSendHead = msg->next) == -1)
				mSendTail = -1;
			mSize--;
		}
		else
			return NIL;
	}
	
	return msg;
}

PFMessage *
CMessageQueue::Peek()
{
	PFMessage *msg = NIL;
	StMutex	mutex(*this);

	if(mSize)
		msg = &mMsgs[mSendHead];
	return msg;
}

void
CMessageQueue::Free(PFMessage *msg)
{
	StMutex	mutex(*this);

	/* check all fields to verify validity of message */
	pgpAssert(msg && msg->num<mMaxMessages &&
				(msg->type>=_mt_quit) &&
				(msg->type<=LASTMSGTYPE) &&
				(msg->len < 2048) &&
				(msg->safe == 1 || msg->safe == 0) &&
				(msg->next >= -1) &&
				(msg->next <= mMaxMessages));
	if(msg->data)
		if(msg->safe)
			safe_free(msg->data);
		else
			pgp_free(msg->data);
	msg->next = mFreeHead;
	mFreeHead = msg->num;
	mFreeSpace->Signal();
}

void
CMessageQueue::FreeAll(void)
{
	StMutex	mutex(*this);
	PFMessage *msg;

	for(;mSize;mSize--)
	{
		pgpAssert(mSendHead != -1);
		msg = &mMsgs[mSendHead];
		if((mSendHead = msg->next) == -1)
			mSendTail = -1;
		if(msg->data)
			if(msg->safe)
				safe_free(msg->data);
			else
				pgp_free(msg->data);
		msg->next = mFreeHead;
		mFreeHead = msg->num;
	}
}

void
CMessageQueue::FreeType(enum MsgType type)
{
	StMutex	mutex(*this);	
	PFMessage *msg;
	short p, nextp, lastp;

	for(lastp=-1,p=mSendHead;p >= 0;p=nextp)
	{
		msg = &mMsgs[p];
		nextp = msg->next;
		if(msg->type == type)
		{
			if(msg->data)
				if (msg->safe)
					safe_free(msg->data);
				else
					pgp_free(msg->data);
			if(lastp >= 0)
				mMsgs[lastp].next = msg->next;
			if(p == mSendTail)
				mSendTail = lastp;
			if(p == mSendHead)
				mSendHead = nextp;
			msg->next = mFreeHead;
			mFreeHead = p;
			mSize--;
		}
		else
			lastp = p;
	}
}

short
CMessageQueue::GetSize(void)
{
	return mSize;
}

void *
CMessageQueue::Next(PFMessage *msg)
{
	if(msg->next == -1)
		return NIL;
	return &mMsgs[msg->next];
}

