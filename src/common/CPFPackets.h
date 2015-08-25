/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFPackets.h,v 1.5 1999/03/10 02:35:21 heller Exp $
____________________________________________________________________________*/
#ifndef CPFPACKETS_H
#define CPFPACKETS_H

#include "LMutexSemaphore.h"
#include "LThread.h"

#include "PGPFoneUtils.h"
#include "CPFTransport.h"
#include "CMessageQueue.h"

#define HPPMAXPKTLEN		1024	// Max packet data len excluding headers
#define HPPBUFFERSIZE		(2*HPPMAXPKTLEN+5)
#define NUMHPPASYNCS		2
#define DEFAULTACKWAITTIME	2000	// Milliseconds to wait for an ACK
#define MINACKTIMEOUT		30000	// No response in 30 seconds = bye bye
#define HPPMAXRETRANSMIT	10		// Maximum retransmissions
#define HPPRTTINTERVAL		12000	// Send RoundTripTimer packet every 12 secs.
									// RTT interval must be greater than ACK wait
#ifdef	PGPXFER
#define MAXRELIABLEPKTS		32
#define MAXSAVEDOOPACKETS	31
#else
#define MAXRELIABLEPKTS		6
#define MAXSAVEDOOPACKETS	5
#endif


enum HPPPacketType	{
						// Unreliable datagrams
						_hpt_Setup=0, _hpt_ACK, _hpt_Voice, _hpt_RTT, _hpt_ReceptionReport,
						// Reliable datagrams
						_hpt_Control=128,
						_hpt_ControlEncrypt,
						_hpt_File
					};

class CPFWindow;
class CSoundInput;
class CSoundOutput;
class CXferThread;
class CPacketWatch;
class CMessageQueue;
class CEncryptionStream;
struct AsyncTransport;
struct PFMessage;

typedef struct HPPReliablePacket
{
	HPPReliablePacket *next;
	enum HPPPacketType type;
	ulong seq, lastXmitTime, firstXmitTime;
	short retransmits, len;
	uchar subType;
	uchar *data;
} HPPReliablePacket;

typedef struct SavedOOPacket
{
	// Designed for saving packets received out of order
	// Only *reliable* packets are saved.
	SavedOOPacket *next;
	enum HPPPacketType type;
	uchar *data;
	ulong seq;
	short len;
} SavedOOPacket;

class CPFPacketsIn	:	public LThread
{
public:
					CPFPacketsIn(Boolean packetMode,
								CPFWindow *window,
								CSoundOutput *soundOutput,
								CPFTransport *transport,
								CMessageQueue *controlQueue,
								CMessageQueue *outQueue,
								CMessageQueue *soundOut,
								void **outResult);
					~CPFPacketsIn();
	void			*Run(void);
	void			AbortSync();
	void			ChangeRXKey(uchar *keyMaterial, uchar *firstEA,
						uchar *secondEA, uchar doubleEncrypt);
	void			SetXferQueue(CMessageQueue *xferQueue);
	void			SetNoCryptoMode();
private:
	Boolean			StreamGetPacket(enum HPPPacketType *type, uchar **buffer, short *len,
								ulong *seq);
	Boolean			GetPacket(enum HPPPacketType *type, uchar **buffer, short *len,
								ulong *seq);
	short			GetData(void);
	void			DeliverPacket(enum HPPPacketType packetType, uchar *packet,
								short packetLen);
	void			AckPacket(enum HPPPacketType packetType, ulong seq);
	void			SendRR();
	
	Boolean					mAbort;
	Boolean					mPacketMode;// skip HDLC encoding if TRUE
	Boolean					mNoCrypto;	// negotiated connection uses no crypto
	ulong					mSequence;
	
	CMessageQueue			*mControlQueue, *mSoundOutQueue, *mXferQueue;

	uchar					mInputBuffer[HPPBUFFERSIZE];
	uchar					*mInputHead, *mInputTail;
	CMessageQueue			*mOutQueue;
	CEncryptionStream		*mDecryptor;
	LMutexSemaphore			mRXKeyChangeMutex;
	CPFTransport			*mTransport;
	CSoundOutput			*mSoundOutput;
	CPFWindow				*mPFWindow;
	SavedOOPacket			*mSavedOOPkts;
	short					mNumSavedOOPkts, mJitterCount, mLateTrack;
#ifdef PGP_WIN32
	HANDLE					mAbortEvent;
	HANDLE					mAbortEvent_foo;
#endif	// PGP_WIN32
};

class CPFPacketsOut	:	public LThread
{
public:
					CPFPacketsOut(Boolean packetMode,
									CPFWindow *window,
									CPFTransport *transport,
									CSoundInput *soundIn,
									CMessageQueue *controlQueue,
									CMessageQueue *soundQueue,
									void **outResult);
					~CPFPacketsOut();
	void			*Run(void);
	void			ChangeTXKey(uchar *keyMaterial, uchar *firstEA,
						uchar *secondEA, uchar doubleEncrypt);
	void			SetSound(ulong intervalms);
	void			SetSoundStatus(Boolean status);
	void			SetXferThread(CXferThread *xferThread);
	void			Send(enum MsgType type, uchar *data, long len);
private:
	void			StreamSendPacket(enum HPPPacketType type, ulong seq, long len,
									uchar *data, Boolean lastPacket);
	void			SendPacket(enum HPPPacketType type, ulong seq, long len,
									uchar *data);
	void			SendMediaPacket(ulong seq, long len, uchar *data);
	void			ServiceControl(long *sentBytes);
	
	Boolean					mAbort, mSendingSound;
	Boolean					mPacketMode;// skip HDLC encoding if TRUE
	ulong					mSequence,
							mReceiverWindow,
							mLastBWIncrease;
	
	AsyncTransport			mAsyncs[NUMHPPASYNCS];
	short					mNextAsync;
	CMessageQueue			*mSoundQueue, *mControlQueue;
	CXferThread				*mXferThread;
	CEncryptionStream		*mEncryptor;
	LMutexSemaphore			mTXKeyChangeMutex;
	CPFTransport			*mTransport;
	CSoundInput				*mSoundInput;
	CPFWindow				*mPFWindow;
#ifdef PGP_WIN32
	HANDLE					mAbortEvent;
	HANDLE					mAbortEvent_foo;
#endif	// PGP_WIN32
};

#endif

