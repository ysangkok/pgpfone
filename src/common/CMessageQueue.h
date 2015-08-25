/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CMessageQueue.h,v 1.5 1999/03/10 02:35:16 heller Exp $
____________________________________________________________________________*/
#ifndef CMESSAGEQUEUE_H
#define CMESSAGEQUEUE_H

#define DEFAULTMAXMSGS	128

#include "LSemaphore.h"
#include "LMutexSemaphore.h"

enum MsgType	{	_mt_quit, _mt_abort, _mt_call, _mt_waitCall,
					_mt_voicePacket, _mt_controlPacket, _mt_ack, _mt_rtt,
					_mt_rr, _mt_talk, _mt_listen, _mt_cloakMsg,
					_mt_changeKey, _mt_resetTransport, _mt_changeDecoder,
					_mt_changeEncoder, _mt_answer, _mt_remoteAbort,
					// File transfer messages
					_mt_filePacket, _mt_sendFile,
					_mt_abortReceive, _mt_abortSend,
					//
					_mt_changeDuplex };
#define LASTMSGTYPE	_mt_changeDuplex
					
class CMessageQueue;
class CPriorityQueue;

typedef struct PFMessage
{
	short num;			// index of message in array
	enum MsgType type;
	void *data;
	short len;			//	length of data
	short safe;			//	whether data is allocated by system or by SafeAlloc
	short next;			//	index of next free message or next send message
} PFMessage;

class CMessageQueue : private LMutexSemaphore 
{
public:
				CMessageQueue(short maxMessages = DEFAULTMAXMSGS);
				~CMessageQueue();
			
	void		Send(enum MsgType type, void *data = NULL, short len = 0, short safe = 0);
	void		SendUnique(enum MsgType type, void *data = NULL, short len = 0, short safe = 0);
	PFMessage	*Recv(long milliseconds = semaphore_WaitForever);
	PFMessage	*Peek(void);
	void		Free(PFMessage *msg);
	void		FreeAll(void);
	void		FreeType(enum MsgType type);
	short		GetSize(void);
	void 		*Next(PFMessage *msg);
	void		SetPrioritySem(LSemaphore *sem);
	
protected:
	short			mMaxMessages, mSize;	/* maximum, current	*/
	short			mFreeHead;				/* index of first free element */
	short			mSendHead, mSendTail;	/* InUse list head and tail */
	PFMessage		*mMsgs;					/* pointer to array of mMaxMessages messages */
	LSemaphore		*mAvailable;			/* will block threads when queue is empty	*/
	LSemaphore		*mFreeSpace;			/* will block threads when queue is full	*/
	LSemaphore		*mPrioritySem;
};

#endif

