/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFTransport.h,v 1.5 1999/03/10 02:36:18 heller Exp $
____________________________________________________________________________*/
#ifndef CPFTRANSPORT_H
#define CPFTRANSPORT_H

#include "LMutexSemaphore.h"

#include "PGPFoneUtils.h"

enum ConnReply	{	_cr_NoReply = 0, _cr_Timeout, _cr_Connect, _cr_OK,
					_cr_NoCarrier, _cr_NoDialTone, _cr_NoAnswer,
					_cr_Busy, _cr_Error, _cr_Ring	};
					
enum CSTypes	{	_cs_uninitialized, _cs_none, _cs_connected,
					_cs_listening, _cs_connecting, _cs_disconnecting,
					_cs_calldetected, _cs_initializing	};

enum CPFTIPPacketTypes	{	_ptip_Call = 1,			//Call request
							_ptip_Busy,				//Response to Call request when busy
							_ptip_Probe,			//Probe to find PGPfones
							_ptip_ProbeResponse,	//Immediate response to Probe and Call
							_ptip_Message,			//Text based message
							_ptip_MessageAck,		//Response to successful receipt of msg
							_ptip_Accept			//Accept incoming call
						};
enum CPFTChannels		{	_pfc_Control, _pfc_Media, _pfc_MediaControl	};
					
typedef struct AsyncTransport
{
	union {
#ifndef PGP_WIN32
		ParamBlockRec	serial;	// one day we will probably use some cool C++
#else
		OVERLAPPED		overlapped;	// Win32 structure for asynchronous I/O
#endif
	} u;
	LMutexSemaphore	*mutex;
	void			*buffer;
#ifdef __MC68K__
	long savedA5;
#endif
#ifdef PGP_WIN32
	DWORD	written;
#endif
} AsyncTransport;

class CPFWindow;

class CPFTransport
{
public:
					CPFTransport(CPFWindow *cpfWindow);
	virtual			~CPFTransport();
	virtual PGErr	Connect(ContactEntry *con, short *connectResult) = 0;
	virtual PGErr	Disconnect() = 0;
	virtual PGErr	Listen(Boolean answer) = 0;
	virtual PGErr	Reset() = 0;
	virtual PGErr	Flush();
	virtual	PGErr	BindMediaStreams();
	virtual long	Read(void *data, long max, short *channel) = 0;
	virtual PGErr	WriteBlock(void *buffer, long *count, short channel) = 0;
	virtual PGErr	WriteAsync(long count, short channel, AsyncTransport *async) = 0;
	PGErr			Write(char *fmt, ...);
	enum CSTypes	GetState();
	virtual void	Answer(void);
	virtual void	AbortSync();
	Boolean			NewAsync(AsyncTransport *asyncTrans);
	void			DeleteAsync(AsyncTransport *asyncTrans);
	virtual void	WaitAsync(AsyncTransport *asyncTrans);
#ifdef PGP_WIN32
	virtual short	SleepForData(HANDLE *abortEvent) = 0;
#endif
protected:
	void			SetState(enum CSTypes newState);

	enum CSTypes	mState;
	Boolean			mAbort, mAnswer;
		
	CPFWindow		*mPFWindow;
	LMutexSemaphore *mTMutex;
	
#ifdef	PGP_WIN32
	HANDLE			mAbortEvent;		// event for waking thread to abort
	HANDLE			mAnswerEvent;		// event for waking thread to answer
#endif
};


#endif

