/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CXferThread.h,v 1.7 1999/03/10 02:42:00 heller Exp $
____________________________________________________________________________*/
#ifndef CXFERTHREAD_H
#define CXFERTHREAD_H

#include "LMutexSemaphore.h"
#include "LThread.h"

#ifdef	PGP_MACINTOSH
#include "CMacBinaryPipe.h"
#elif	PGP_WIN32
#include "CWinFilePipe.h"
#endif

#include "PGPFoneUtils.h"

class CPFWindow;
class CPFPacketsOut;
class CMessageQueue;
class CXferWindow;
class CPipe;
class SHA;

#define MAXRCVFILES			128	// Limited by memory and # of available stream IDs
#define MAXSNDFILES			128

#define MAXXFERPACKETSIZE	960
#define MINXFERPACKETSIZE	128	// Do *NOT* lower this - WP

#define XFERSALTCHECKSIZE	8
#define MAXFILENAMELEN		256

enum XferSubtypes	{	_xst_OpenStream, _xst_AckOpenStream,
						_xst_AbortStream, _xst_RemoteAbortStream,
						_xst_DataStream, _xst_EndStream	};
						
enum OpenStreamSubTypes	{	_ost_CanonDirectory, _ost_CanonInfo	};

typedef struct XSendFile
{
	XSendFile *next;
	XferInfo xi;
	uchar path[256];
	uchar localStreamID;
	ulong salt[2];
	Boolean openAckd;
	SHA *hash;
	LThread *fileThread;
	long fileThreadResult;
} XSendFile;

typedef struct XRcvFile
{
	XRcvFile *next;
	XferInfo xi;
	uchar path[256];
	uchar remoteStreamID;
	ulong salt[2];
	SHA *hash;
	LThread *fileThread;
	long fileThreadResult;
} XRcvFile;

class CXferThread	:	public LThread, public LMutexSemaphore
{
public:
					CXferThread(CPFWindow *window,
								CXferWindow *xferWindow,
								CPFPacketsOut *packetOut,
								CMessageQueue *inQueue,
								CMessageQueue *outQueue,
								void **outResult);
					~CXferThread();
	void			*Run(void);
	CMessageQueue	*GetQueue();
	long			GetNextSendPacket(long maxSize);
protected:
	XSendFile		*OpenStream(XSendFile *xsf);
	void			AckOpenStream(XRcvFile *xrf);
	void			StartSend();
	void			EndSend();
	short			GetFileName(XferInfo *xi, uchar *name);
	Boolean			PrepIncomingFile(XRcvFile *xrf, uchar *p, uchar *pathname, short len);
	void			GetFileSize(XSendFile *xsf, void *file);
	XSendFile		*InitSend(void *file);
	XRcvFile		*InitRecv(uchar *p, uchar streamID, short len);
	void			DisposeSend(XSendFile *xsf);
	void			DisposeRecv(XRcvFile *xrf);
	void			AbortSend(uchar localStreamID, ulong *salt);
	void			AbortReceive(uchar remoteStreamID, ulong *salt);
	Boolean			HashPartialFile(XferInfo *xi, SHA *hash, uchar *hashfinal);
	void			CheckForNewStreams();
	
private:
	CMessageQueue			*mOutQueue, *mInQueue;
	CPFWindow				*mPFWindow;
	CXferWindow				*mXferWindow;
	CPFPacketsOut			*mPacketThread;
	uchar					mNextStreamID;
	short					mNumSending, mNumReceiving, mOpeningStream;
	XSendFile				*mSending, *mOverflowSends;
	XRcvFile				*mReceiving;
	LMutexSemaphore			*mSendMutex;
};

#endif

