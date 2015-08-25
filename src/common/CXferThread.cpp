/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CXferThread.cpp,v 1.15 1999/03/10 02:41:57 heller Exp $
____________________________________________________________________________*/
#include <stdio.h>
#include <string.h>

#include "CXferThread.h"

#include "CPFPackets.h"
#include "CXferWindow.h"
#include "CControlThread.h"
#include "CPacketWatch.h"
#include "CPFWindow.h"
#include "CStatusPane.h"
#include "CPipe.h"
#include "fastpool.h"
#include "SHA.h"

#define noDEBUGXFERLOG

#ifdef	PGP_MACINTOSH
#include <UThread.h>
#include "PGPFMacUtils.h"
#include "CMacBinaryPipe.h"
#define CFileSendPipe		CMacBinarySendPipe
#define CFileReceivePipe	CMacBinaryReceivePipe
#elif	PGP_WIN32
#include "CWinFilePipe.h"
#define CFileSendPipe		CWinFileSendPipe
#define CFileReceivePipe	CWinFileReceivePipe
#endif


CXferThread::CXferThread(CPFWindow *window,
				CXferWindow *xferWindow,
				CPFPacketsOut *packetOut,
				CMessageQueue *inQueue,
				CMessageQueue *outQueue,
				void **outResult)
#ifdef	PGP_MACINTOSH
        : LThread(FALSE, thread_DefaultStack, threadOption_UsePool, outResult)
#else
        : LThread(outResult)
#endif  // PGP_WIN32
{
	pgpAssertAddrValid(window, CPFWindow);
	pgpAssertAddrValid(xferWindow, VoidAlign);
	pgpAssertAddrValid(packetOut, VoidAlign);
	pgpAssertAddrValid(inQueue, CMessageQueue);
	pgpAssertAddrValid(outQueue, CMessageQueue);
	mPFWindow = window;
	mXferWindow = xferWindow;
	mPacketThread = packetOut;
	mInQueue = inQueue;
	mOutQueue = outQueue;
	mNextStreamID = 0;
	mSending = NIL;
	mReceiving = NIL;
	mOverflowSends = NIL;
	mNumSending = mNumReceiving = mOpeningStream = 0;
	mSendMutex = new LMutexSemaphore;
}

CXferThread::~CXferThread()
{
	delete mSendMutex;
}

void *
CXferThread::Run(void)
{
	PFMessage *msg;
	Boolean done = false;
	long subLen;
	ulong salt[2];
	uchar *packet, ptype, streamID, *push, str[128];
	XSendFile *xsf, *walk;
	XRcvFile *xrf;

	while(!done)
	{
		// We track send requests from CXferWindow and handle all incoming data pkts here
		if(msg = mInQueue->Recv())
		{
			switch(msg->type)
			{
				case _mt_sendFile:
					mSendMutex->Wait();
					xsf = InitSend(msg->data);
					pgpAssertAddrValid(xsf, XSendFile);
					if(xsf)
					{
						if(!mOpeningStream && (mNumSending<MAXSNDFILES))
						{
							OpenStream(xsf);
#ifdef	DEBUGXFERLOG
							GetFileName(&xsf->xi, str);
							DebugLog("Send: OpenStream(_mt_sendFile): %s", str);
#endif
						}
						else
						{
							if(IsNull(mOverflowSends))
								mOverflowSends = xsf;
							else
							{
								pgpAssertAddrValid(mOverflowSends, XSendFile);
								for(walk=mOverflowSends;walk->next;)
								{
									if(IsntNull(walk->next))
									{
										pgpAssertAddrValid(walk->next, XSendFile);
									}
									walk=walk->next;
								}
								walk->next = xsf;
							}
						}
					}
					mSendMutex->Signal();
					break;
				case _mt_abortSend:
					mSendMutex->Wait();
					if(!msg->data)
						xsf = mSending;
					else
						for(xsf=mSending;xsf && xsf!=msg->data;)
						{
							if(IsntNull(xsf->next))
							{
								pgpAssertAddrValid(xsf->next, XSendFile);
							}
							xsf=xsf->next;
						}
					if(xsf)
					{
						pgpAssertAddrValid(xsf, XSendFile);
#ifdef	DEBUGXFERLOG
						DebugLog("(ID:%d)Sent: _xst_AbortStream - user aborted xfer",
									(int)xsf->localStreamID);
#endif
						if(xsf->xi.pipe)
						{
							xsf->xi.pipe->DoAbortPipe();
							if(!xsf->xi.fatalErr)
								xsf->xi.fatalErr=_pge_InternalAbort;
							xsf->xi.pipe->DoAckEndPipe();
						}
						AbortSend(xsf->localStreamID, &xsf->salt[0]);
						if(xsf == mSending)
							EndSend();
						else
							DisposeSend(xsf);
						if(!mOpeningStream)
							CheckForNewStreams();
					}
					mSendMutex->Signal();
					break;
				case _mt_abortReceive:
					if(IsNull(msg->data))
						xrf = mReceiving;
					else
						for(xrf=mReceiving;IsntNull(xrf) && xrf!=msg->data;)
						{
							if(IsntNull(xrf->next))
							{
								pgpAssertAddrValid(xrf->next, XRcvFile);
							}
							xrf=xrf->next;
						}
					if(xrf)
					{
						pgpAssertAddrValid(xrf, XRcvFile);
#ifdef	DEBUGXFERLOG
						DebugLog("(ID:%d)Sent: _xst_RemoteAbortStream - user aborted xfer",
									(int)xrf->remoteStreamID);
#endif
						AbortReceive(xrf->remoteStreamID, &xrf->salt[0]);
						if(xrf->xi.pipe)
						{
							xrf->xi.pipe->DoAbortPipe();
							mXferWindow->EndReceive();
						}
						DisposeRecv(xrf);
					}
					break;
				case _mt_filePacket:
					packet = (uchar *)msg->data;
					pgpAssertAddrValid(packet, uchar *);
					if(msg->len < 2)
						break;
					ptype = *packet++;
					streamID = *packet++;
					subLen = msg->len - 2;
					// Find the stream
					if((ptype == _xst_AckOpenStream) || (ptype == _xst_RemoteAbortStream))
					{
						mSendMutex->Wait();
						if(IsntNull(mSending))
						{
							pgpAssertAddrValid(mSending, XSendFile);
						}
						for(xsf=mSending;xsf;)
						{
							if(xsf->localStreamID == streamID)
							{
								switch(ptype)
								{
									case _xst_AckOpenStream:{
										ulong startPos;
										uchar hash[SHS_DIGESTSIZE];
										
										// Sanity checks and interpretations on packet
										// Packet format: StartPos(32)/Hash(>=16bytes) - if Start>0
										pgpAssert(!xsf->openAckd);
										xsf->openAckd = TRUE;
										BUFFER_TO_LONG(packet, startPos);	packet+=4;
#ifdef	DEBUGXFERLOG
										DebugLog("(ID:%d)Rcvd: _xst_AckOpenStream (pos:%ld)",
													(int)streamID, startPos);
#endif
										xsf->xi.bytesDone = startPos;
										xsf->xi.cpsBase = startPos;


										if((subLen >= 4) && (startPos<xsf->xi.bytesTotal) && !xsf->xi.pipe
											&& (!startPos ||
												((subLen >= 4+SHS_DIGESTSIZE) &&
												HashPartialFile(&xsf->xi, xsf->hash, &hash[0]) && 
												!memcmp(&hash, packet, SHS_DIGESTSIZE))))
										{
											// If a waiting file is at the head of the queue,
											// start sending it now.
											if(mSending == xsf)
											{
												StartSend();
											}
										}
										else
										{
											AbortSend(xsf->localStreamID, &xsf->salt[0]);
											DisposeSend(xsf);
											if(mSending && mSending->openAckd && !mSending->xi.pipe)
												StartSend();
#ifdef	DEBUGXFERLOG
											DebugLog("_xst_AckOpenStream: Error in file position or hash");
#endif
										}
										mOpeningStream = FALSE;
										if(mNumSending<MAXSNDFILES)
											CheckForNewStreams();
										break;}
									case _xst_RemoteAbortStream:
										pgp_memcpy( &salt[0], packet, XFERSALTCHECKSIZE );
										packet += XFERSALTCHECKSIZE;
										if((subLen >= XFERSALTCHECKSIZE) &&
											!memcmp(salt, &xsf->salt[0], XFERSALTCHECKSIZE))
										{
											if((mSending==xsf) && xsf->xi.pipe)
											{
												// Remote has aborted xfer in progress
#ifdef	DEBUGXFERLOG
												DebugLog("(ID:%d)_xst_RemoteAbortStream: "
															"Aborting current xfer.", (int)streamID);
#endif
												xsf->xi.pipe->DoAbortPipe();
												if(!xsf->xi.fatalErr)
													xsf->xi.fatalErr=_pge_InternalAbort;
												xsf->xi.pipe->DoAckEndPipe();
												EndSend();
											}
											else
											{
												if(!xsf->openAckd)
													mOpeningStream = FALSE;
												DisposeSend(xsf);
#ifdef	DEBUGXFERLOG
												DebugLog("(ID:%d)_xst_RemoteAbortStream: "
															"Aborting future xfer", (int)streamID);
#endif
											}
											if(!mOpeningStream)
												CheckForNewStreams();
										}
										break;
								}
								break;
							}
							if(IsntNull(xsf->next))
							{
								pgpAssertAddrValid(xsf->next, XSendFile);
							}
							xsf=xsf->next;
						}
						mSendMutex->Signal();
					}
					else if(ptype == _xst_OpenStream)
					{
						// Confirm that this stream does not exist
						for(xrf=mReceiving;xrf;xrf=xrf->next)
							if(xrf->remoteStreamID == streamID)
							{
#ifdef	DEBUGXFERLOG
								DebugLog("(ID:%d)Rcvd: _xst_OpenStream FAILED, dup stream",
											(int)streamID);
#endif
								break;
							}
						if(!xrf)
						{
							if(!(xrf=InitRecv(packet, streamID, subLen)))
							{
								pgp_memcpy( &salt[0], packet, XFERSALTCHECKSIZE );
								packet += XFERSALTCHECKSIZE;
								AbortReceive(streamID, &salt[0]);
#ifdef	DEBUGXFERLOG
								DebugLog("(ID:%d)Rcvd: _xst_OpenStream FAILED", (int)streamID);
#endif
							}
							else
							{
								AckOpenStream(xrf);
#ifdef	DEBUGXFERLOG
								DebugLog("(ID:%d)Rcvd: _xst_OpenStream, sent _xst_AckOpenStream", (int)streamID);
#endif
							}
						}
					 }
					 else for(xrf=mReceiving;xrf;xrf=xrf->next)
						if(xrf->remoteStreamID == streamID)
						{
							switch(ptype)
							{
								case _xst_AbortStream:
									// Packet format is simply the original salt
									pgp_memcpy( &salt[0], packet, XFERSALTCHECKSIZE );
									packet += XFERSALTCHECKSIZE;
									if((subLen >= XFERSALTCHECKSIZE) &&
										!memcmp(&salt[0], &xrf->salt[0], XFERSALTCHECKSIZE))
									{
#ifdef	DEBUGXFERLOG
										DebugLog("(ID:%d)Rcvd: _xst_AbortStream salt matched",
													(int)streamID);
#endif
										if(xrf->xi.pipe)
										{
											xrf->xi.pipe->DoAbortPipe();
											mXferWindow->EndReceive();
										}
										DisposeRecv(xrf);
									}
									else
									{
#ifdef	DEBUGXFERLOG
										DebugLog("(ID:%d)Rcvd: _xst_AbortStream FAILED SALT MATCH",
													(int)streamID);
#endif
									}
									break;
								case _xst_DataStream:
									if(!xrf->xi.pipe)
									{
										// This is the first data packet of a transfer
										// that has already been setup, so we need
										// to create the pipe.  Theoretically, we could
										// receive more than one file at a time.
										xrf->xi.startTime=xrf->xi.lastTime=pgp_getticks();
										xrf->xi.pipe = new CPipe(STDPIPESIZE, STDPIPEEXTRA);
										xrf->fileThreadResult = 0;
										xrf->fileThread = new CFileReceivePipe(&xrf->xi,
												(void **)&xrf->fileThreadResult);
										xrf->xi.pipe->SetPusher(this);
										xrf->xi.pipe->SetPuller(xrf->fileThread);
										xrf->fileThread->Resume();
										mXferWindow->StartReceive(&xrf->xi);
#ifdef	DEBUGXFERLOG
										DebugLog("(ID:%d)Rcvd: _xst_DataStream - creating pipe",
												(int)streamID);
#endif
									}
									// Push the data into the pipe.
									xrf->xi.bytesDone += subLen;
									xrf->xi.lastTime=pgp_getticks();
									xrf->hash->Update(packet, subLen);
									xrf->xi.pipe->DoStartPipePush(subLen, (void **)&push);
									pgp_memcpy(push, packet, subLen);
									xrf->xi.pipe->DoEndPipePush(-1L);
									if(xrf->xi.pipe->DoPipeAborted())
										mInQueue->Send(_mt_abortReceive);
#ifdef	DEBUGXFERLOG
									DebugLog("(ID:%d)Rcvd: _xst_DataStream - pushed %d bytes",
												(int)streamID, (int)subLen);
#endif
									break;
								case _xst_EndStream:{
									Boolean goodHash = FALSE;
									
									pgp_memcpy( &salt[0], packet, XFERSALTCHECKSIZE );
									packet += XFERSALTCHECKSIZE;
#ifdef	DEBUGXFERLOG
									DebugLog("(ID:%d)Rcvd: _xst_EndStream", (int)streamID );
#endif
									if((subLen >= XFERSALTCHECKSIZE + SHS_DIGESTSIZE) &&
										!memcmp(&salt[0], &xrf->salt[0], XFERSALTCHECKSIZE))
									{
										xrf->hash->Final(str);
										if(!memcmp(str, packet, SHS_DIGESTSIZE))
										{
											goodHash = TRUE;
											xrf->xi.pipe->DoEndPipe((long *)&xrf->xi.bytesDone);
											GetFileName(&xrf->xi, str);
											CStatusPane::GetStatusPane()->AddStatus(0,
												"Rcvd: %s (%ld K) - SHA matched.", 
												str, maxl(xrf->xi.bytesTotal / 1024,1));
										}
									}
									if(!goodHash)
									{
										xrf->xi.pipe->DoAbortPipe();
										CStatusPane::GetStatusPane()->AddStatus(0,
											"Receive aborted due to SHA mismatch!");
									}
									DisposeRecv(xrf);
									mXferWindow->EndReceive();
									break;}
							}
							break;
						}
					break;
				case _mt_abort:
					done = TRUE;
					break;
				default:
					pgp_errstring("Unknown XferThread command.");
					break;
			}
			mInQueue->Free(msg);
		}
	}
	mPacketThread->SetXferThread(NULL);
	while(mSending)
	{
		if(mSending->xi.pipe)
		{
			mSending->xi.pipe->DoAbortPipe();
			if(!mSending->xi.fatalErr)
				mSending->xi.fatalErr=_pge_InternalAbort;
			mSending->xi.pipe->DoAckEndPipe();
			mXferWindow->EndSend();
		}
		DisposeSend(mSending);
	}
	while(mOverflowSends)
	{
		xsf = mOverflowSends;
		delete xsf->hash;
		mOverflowSends = xsf->next;
		pgp_free(xsf);
	}
	while(mReceiving)
	{
		if(mReceiving->xi.pipe)
		{
			mReceiving->xi.pipe->DoAbortPipe();
			mXferWindow->EndReceive();
		}
		DisposeRecv(mReceiving);
	}
	return (void *)1L;
}

void
CXferThread::CheckForNewStreams()
{
	XSendFile *xsf;
	
	if(mOverflowSends)
	{
		xsf = mOverflowSends;
		mOverflowSends = xsf->next;
		xsf->next = NIL;
		OpenStream(xsf);
#ifdef	DEBUGXFERLOG
		uchar str[128];
		GetFileName(&xsf->xi, str);
		DebugLog("Send: OpenStream: %s", str);
#endif
	}
}

XRcvFile *
CXferThread::InitRecv(uchar *p, uchar streamID, short len)
{
	StMutex mutex(*this);
	XRcvFile *nxr=NIL, *walk;
	uchar filename[MAXFILENAMELEN+1], *e, *dirspec;
	short nlen, subtype, slen, dirlen;

	// Format:
	// 8 byte salt
	// data type: 0 for files
	// 4 byte total file length
	// filename [max 256 chars], null terminated
	// subpackets*		(0 or more, in any order)
	//		subpacket type byte
	//		subpacket length byte
	//		subpacket [length]
	if(mNumReceiving >= MAXRCVFILES)
		goto error;
	if(len<XFERSALTCHECKSIZE+5)	//verify minimum length
		goto error;
	for(walk=mReceiving;walk;walk=walk->next)
		if(walk->remoteStreamID == streamID)
			goto error;
	nxr = (XRcvFile *)pgp_malloc(sizeof(XRcvFile));	pgpAssert(nxr);
	memset(nxr, 0, sizeof(XRcvFile));
	nxr->remoteStreamID = streamID;
	nxr->fileThreadResult = 1;
	e=p+len;
	pgp_memcpy( &nxr->salt[0], p, XFERSALTCHECKSIZE );
	p += XFERSALTCHECKSIZE;
	if(*p++ != 0)	// check data type specifier, we only handle files
		goto error;
	BUFFER_TO_LONG(p, nxr->xi.bytesTotal);	
	p+=sizeof(nxr->xi.bytesTotal);
	nlen = strlen((char *)p);
	if((len<nlen+14) || (nlen > MAXFILENAMELEN))	//verify name length is correct
		goto error;
	strcpy((char *)filename, (char *)p);	
	p+=nlen+1;
	dirlen = 0;	dirspec = NIL;
	// subtype interpreter
	slen = 0;
	for(;p<e;p+=slen)
	{
		subtype = *p++;
		if(p == e)
			goto error;
		slen = *p++;
		if(e-p < slen)
			goto error;
		switch(subtype)
		{
			case _ost_CanonDirectory:
				// cross platform canonical directory specifier
				dirspec = p;
				dirlen = slen;
				break;
			case _ost_CanonInfo:
				// cross platform canonical file info (creation/mod/etc..)
				break;
		}
	}
	
	if(PrepIncomingFile(nxr, filename, dirspec, dirlen))// platform dependent processing
	{
		if(mReceiving)
		{
			for(walk=mReceiving;walk->next;walk=walk->next);
			walk->next = nxr;
		}
		else
			mReceiving = nxr;
		mNumReceiving++;
		nxr->hash = new SHA();
		nxr->hash->Update((uchar *)&nxr->salt[0], XFERSALTCHECKSIZE);
		mXferWindow->QueueReceive(nxr);
		return nxr;
	}
error:
	if(nxr)
		pgp_free(nxr);
	return NIL;
}

void
CXferThread::AckOpenStream(XRcvFile *xrf)
{
	uchar *p, *e, hashfinal[SHS_DIGESTSIZE];
	short hashLen;

	if(HashPartialFile(&xrf->xi, xrf->hash, hashfinal))
	{
		hashLen = SHS_DIGESTSIZE;
		xrf->xi.resurrect = 1;
	}
	else
		hashLen = 0;
	e = p = (uchar *)safe_malloc(6 + hashLen);	pgpAssert(p);
	*e++ = _xst_AckOpenStream;
	*e++ = xrf->remoteStreamID;
	LONG_TO_BUFFER(xrf->xi.bytesDone, e);	e+=sizeof(xrf->xi.bytesDone);
	if(hashLen)
	{
		pgp_memcpy(e, hashfinal, SHS_DIGESTSIZE);
		e+=SHS_DIGESTSIZE;
	}
	mOutQueue->Send(_mt_filePacket, p, e-p, 1);
}

XSendFile *
CXferThread::OpenStream(XSendFile *xsf)
{
	XSendFile *walk;
	uchar name[128], *p, *e;
	short nlen;
	
	if(xsf)
	{
		if(!mSending)
			mSending = xsf;
		else
		{
			for(walk=mSending;walk->next;walk=walk->next);	
			walk->next = xsf;
		}
		mNumSending++;
		mOpeningStream=TRUE;
		mXferWindow->QueueSend(xsf);
		xsf->localStreamID = mNextStreamID++;
		nlen = GetFileName(&xsf->xi, name);
		e = p = (uchar *)safe_malloc(16+nlen);	pgpAssert(p);
		*e++ = _xst_OpenStream;
		*e++ = xsf->localStreamID;
		pgp_memcpy( e, &xsf->salt[0], XFERSALTCHECKSIZE );	e += XFERSALTCHECKSIZE;
		*e++ = 0;	// Data type specifier - 0 is file, hardcoded for now
		LONG_TO_BUFFER(xsf->xi.bytesTotal, e);	e+=sizeof(xsf->xi.bytesTotal);
		strcpy((char *)e, (char *)name);	e+=nlen+1;
		if(xsf->path[0])
		{
			*e++ = _ost_CanonDirectory;
			
#ifdef PGP_MACINTOSH
			pstrcpy(e, xsf->path);
			e+=*e+1;
#elif PGP_WIN32
			nlen = strlen((char*)xsf->path);
			//*e++ = nlen;
			strcpy((char*)e, (char*)xsf->path);
			e+=nlen;
#endif
			
		}
		
		mOutQueue->Send(_mt_filePacket, p, e-p, 1);
	}
	return xsf;
}

XSendFile *
CXferThread::InitSend(void *file)
{
	StMutex mutex(*this);
	XSendFile *nxs=NIL;

	if(nxs = (XSendFile *)pgp_malloc(sizeof(XSendFile)))
	{
		memset(nxs, 0, sizeof(XSendFile));
		GetFileSize(nxs, file);
		randPoolGetBytes((uchar *)&nxs->salt[0], XFERSALTCHECKSIZE);
		nxs->hash = new SHA();
		nxs->hash->Update((uchar *)&nxs->salt[0], XFERSALTCHECKSIZE);
		
		nxs->fileThreadResult = 1;
	}
#ifdef	DEBUGXFERLOG
	else
		DebugLog("_mt_sendFile: memory error!");
#endif
	return nxs;
}

void
CXferThread::StartSend()
{
	XSendFile *xsf;
	
	// First, create the file data pipe, we create it now
	// so that it is not allocated when the stream has only
	// been setup.
	pgpAssert(mSending);
	xsf = mSending;
	xsf->xi.startTime=xsf->xi.lastTime=pgp_getticks();
	xsf->xi.pipe = new CPipe(STDPIPESIZE, STDPIPEEXTRA);
	xsf->fileThread = new CFileSendPipe(&xsf->xi, FALSE,
						(void **)&xsf->fileThreadResult);
	// Create the send thread
	xsf->xi.pipe->SetPusher(xsf->fileThread);
	xsf->xi.pipe->SetPuller(mPacketThread);
	if(xsf->xi.bytesDone)	// If so, this is a resumed file transfer
		xsf->xi.pipe->DoSetPipePos(xsf->xi.bytesDone);
	xsf->fileThread->Resume();
	mXferWindow->StartSend(&xsf->xi);
}

void
CXferThread::EndSend()
{
	// The file transfer at the front of the queue is completed
	// so move onto the next file.
	mXferWindow->EndSend();
	if(mSending)
	{
		DisposeSend(mSending);
		if(mSending && mSending->openAckd && !mSending->xi.pipe)
			StartSend();
	}
}

void
CXferThread::DisposeSend(XSendFile *xsf)
{
	XSendFile *walk;
	
	while(!xsf->fileThreadResult)
		(Yield)();
	mXferWindow->DequeueSend(xsf);
	if(xsf == mSending)
		mSending = xsf->next;
	else
	{
		for(walk=mSending;walk->next!=xsf;walk=walk->next);
		if(walk->next == xsf)
			walk->next = xsf->next;
	}
	delete xsf->hash;
	if(xsf->xi.pipe)
		delete xsf->xi.pipe;
	mNumSending--;
	pgp_free(xsf);
}

void
CXferThread::DisposeRecv(XRcvFile *xrf)
{
	XRcvFile *lxrf, *wxrf;
	
	while(!xrf->fileThreadResult)
		(Yield)();
	mXferWindow->DequeueReceive(xrf);
	for(lxrf=NIL,wxrf=mReceiving;wxrf!=xrf;wxrf=wxrf->next)
		lxrf=wxrf;
	if(wxrf==xrf)
	{
		if(lxrf)
			lxrf->next = xrf->next;
		else
			mReceiving = xrf->next;
		delete xrf->hash;
		if(xrf->xi.pipe)
			delete xrf->xi.pipe;
		pgp_free(xrf);
	}
	mNumReceiving--;
}

short
CXferThread::GetFileName(XferInfo *xi, uchar *name)
{
#ifdef	PGP_MACINTOSH
	short len;
	FSSpec *fs;
	
	fs = (FSSpec *)&xi->file;
	len = fs->name[0];
	pstrcpy(name, fs->name);
	p2cstr(name);
	return len;
#elif	PGP_WIN32
	short len = 0;
	char* cp = NULL;
	
	cp = strrchr(xi->filepath, '\\');
	
	if(cp)
	{
		strcpy((char*)name, cp + 1);
		len = strlen((char*)name);
	}
	else
	{
		*name = 0x00;
	}
	
	return len;

#endif
}

Boolean
CXferThread::PrepIncomingFile(	XRcvFile *xrf, 		// new receive file struct
								uchar *p, 			// platform independant filename
								uchar *pathname, 	// platform independant directory
								short len)			// directory length
{
	// Copy name into platform dependent file structure
#ifdef	PGP_MACINTOSH
	HParamBlockRec hpb;
	CInfoPBRec cpb;
	Str255 path, thisDir;
	FSSpec *fs;
	uchar *e=pathname, *u;
	char s[128];
	short vRefNum, inx;
	OSErr err;
	long dirID;
	
	fs = &xrf->xi.file;
	// first parse the pathname
	if(gPGFOpts.fopt.volName[0])
	{
		vRefNum = pgp_GetVolRefNum(gPGFOpts.fopt.volName);
		dirID = gPGFOpts.fopt.dirID;
	}
	else
	{
		vRefNum = -1;	//default root directory
		dirID = 0;
	}
	path[0]=0;
	while(len>0)
	{
		len-=*e+1;
		pstrcpy(thisDir, e);
		// make sure the directory name is mac-friendly
		for(inx = 0,u=e+1;inx<*e;inx++,u++)
			if(*u == ':')
				*u = '-';	// no colons in dirnames
		if(*(e+1)=='.')
			*(e+1)='-';		// no periods in first letter on the mac
		e+=*e+1;
		pstrcat(path, "\p:");
		pstrcat(path, thisDir);
		cpb.dirInfo.ioCompletion = NIL;
		cpb.dirInfo.ioNamePtr = thisDir;
		cpb.dirInfo.ioVRefNum = vRefNum;
		cpb.dirInfo.ioFDirIndex = 0;
		cpb.dirInfo.ioDrDirID = dirID;
		err = PBGetCatInfoSync(&cpb);
		if(err)
		{
			// doesn't exist, so create it
			hpb.fileParam.ioCompletion = NIL;
			hpb.fileParam.ioNamePtr = thisDir;
			hpb.fileParam.ioVRefNum = vRefNum;
			hpb.fileParam.ioDirID = dirID;
			err = PBDirCreateSync(&hpb);
			dirID = hpb.fileParam.ioDirID;
		}
		else
			dirID = cpb.dirInfo.ioDrDirID;
	}
	
	pstrcpy(xrf->path, path);
	fs->vRefNum = vRefNum;
	fs->parID = dirID;

	len = strlen((char *)p);
	strncpy(s, (char *)p, 32);
	if(len > 31)
		s[31] = 0;
	p=(uchar *)s;
	for(;*p;p++)
		if(*p == ':')
			*p = '-';	// no colons in filenames
	if(s[0]=='.')
		s[0]='-';		// no periods in first letter on the mac
	c2pstr(s);
	pstrcpy(fs->name, (uchar *)s);
	hpb.ioParam.ioCompletion = NIL;
	hpb.ioParam.ioNamePtr = NIL;
	hpb.ioParam.ioVRefNum = fs->vRefNum;
	hpb.volumeParam.ioVolIndex = 0;
	if(!PBHGetVInfoSync(&hpb))
	{
		if(hpb.volumeParam.ioVFrBlk * hpb.volumeParam.ioVAlBlkSiz
				<= xrf->xi.bytesTotal)
		{
			CStatusPane::GetStatusPane()->AddStatus(0,
				"Not enough disk space to receive file!");
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
	
#elif	PGP_WIN32

	char savePath[MAX_PATH + 1] = {0x00};
	//char tempPath[MAX_PATH + 1] = {0x00};
	
	// get the default save directory
	if(gPGFOpts.fopt.recvDir[0])
	{
		strcpy(savePath, gPGFOpts.fopt.recvDir);
		
		if(!SetCurrentDirectory(savePath) )
		{
			// need to create it...
		}
	}
	else
	{
		char root[MAX_PATH];
		
		GetCurrentDirectory(sizeof(root), root);

		root[3] = 0x00;
		
		strcpy(savePath,root);  
	}
		
	// parse the platform independant directory
	char* cp = (char*)pathname;
	
	
	while(len > 0)
	{
		char* check = NULL;
		char thisDir[256];
		int dirLength;
		int i;
		
		len -= *cp + 1;
		
		check = cp + 1;
		
		// make sure the directory name is win-friendly
		// chars not allowed in win filenames ->  \ / : * ? " < > |
		
		for(i = 0; i < *cp ; i++, check++)
		{
			if(	*check == '\\' 	|| 
				*check == '/' 	|| 
				*check == ':' 	||
				*check == '*' 	||
				*check == '?' 	||
				*check == '\"' 	|| 
				*check == '<' 	||
				*check == '>' 	||
				*check == '|' 	)
			{
				*check = '-';	// no colons in dirnames
			}
		}
			
		dirLength = *cp;
				
		memcpy(thisDir, cp + 1, dirLength);
		*(thisDir + dirLength) = 0x00;
				
		strcat(savePath, "\\");
		strcat(savePath, thisDir);
		
		if( !SetCurrentDirectory(savePath) )
		{
			// doesn't exist, so create it
			CreateDirectory(savePath, NULL);
		}
		
		cp += *cp + 1;
	}
	
	// tuck away path
	strcpy((char*)xrf->path, savePath);
	
	// parse the platform independant filename
	char filename[256];
	
	len = strlen((char *)p);
	strncpy(filename, (char *)p, 255);
	
	
	cp = filename;
	
	for(;*cp;cp++)
	{
		if(	*cp == '\\' || 
			*cp == '/' 	|| 
			*cp == ':' 	||
			*cp == '*' 	||
			*cp == '?' 	||
			*cp == '\"' || 
			*cp == '<' 	||
			*cp == '>' 	||
			*cp == '|' 	)
		{
			*cp = '-';	// no colons in filenames
		}
	}
	
	if(filename[0] == '.')
	{
		filename[0] = '-';		// no periods in first letter on windows
	}
	
	strcat(savePath, "\\");
	strcat(savePath, filename);
		
	// tuck away path and filename
	strcpy((char*)xrf->xi.filepath, savePath);
	
	xrf->xi.filename = strrchr((char*)xrf->xi.filepath, '\\') + 1;
	
	// check the current disk for space
	
	DWORD sectorsPerCluster; 
	DWORD bytesPerSector;	
	DWORD numberOfFreeClusters;
	DWORD totalNumberOfClusters;	 

	GetDiskFreeSpace( 	NULL, // use current working root
						&sectorsPerCluster,
						&bytesPerSector,	
						&numberOfFreeClusters,
						&totalNumberOfClusters);
						
	DWORD freespace = sectorsPerCluster * bytesPerSector * numberOfFreeClusters;
	
	if(freespace <= xrf->xi.bytesTotal)
	{
		/*CStatusPane::GetStatusPane()->AddStatus(0,
			"Not enough disk space to receive file!");*/
			
		MessageBox(	NULL, 
					"Not enough disk space to receive file!", 
					filename, 
					MB_OK|MB_ICONERROR);
			
		return FALSE;
	}
	
	return TRUE;
	
#endif
}

void
CXferThread::GetFileSize(XSendFile *xsf, void *file)
{
#ifdef	PGP_MACINTOSH
	XferFileSpec *xspec = (XferFileSpec *)file;
	Str255 rootPath, filePath;
	uchar *p;
	short pathlen, dirlen;
	
	if((xspec->rootDir != xspec->fs.parID) || (xspec->rootVol != xspec->fs.vRefNum))
	{
		rootPath[0] = filePath[0] = 0;
		pgp_GetFullPathname(xspec->rootVol, xspec->rootDir, rootPath);
		pgp_GetFullPathname(xspec->fs.vRefNum, xspec->fs.parID, filePath);
		// cut off the root part of the directory and record the partial pathname
		pstrcpy(xsf->path, "\p:");	// add leading colon which will be replaced by sublen
		dirlen = pstrlen(rootPath);
		pathlen = pstrlen(filePath) - dirlen;
		memcpy(&xsf->path[2], &filePath[dirlen+1], pstrlen(filePath) - dirlen);
		xsf->path[0] = pathlen+1;
		pathlen = --xsf->path[0];	// cut off the trailing colon
		for(p=&xsf->path[pathlen],dirlen=0;p>xsf->path;--p)
		{
			if(*p == ':')
			{
				*p = dirlen;
				dirlen = 0;
			}
			else
				dirlen++;
		}
	}
	pgp_memcpy(&xsf->xi.file, &xspec->fs, sizeof(FSSpec));
	xsf->xi.sendAs = SendFileAs(&xsf->xi.file, mPFWindow->GetControlThread()->GetRemoteSystemType());
	CheckSendMethod(this, &xsf->xi);
#elif	PGP_WIN32
	XferFileSpec *xspec = (XferFileSpec *)file;
	//char* path = NULL;
	int dirlen = 0;
	int pathlen = 0;
	char* cp = NULL;
	
	strcpy(xsf->xi.filepath, xspec->path);
	xsf->xi.filename = strrchr(xsf->xi.filepath, '\\') + 1;
	
	*(xsf->path) = 0x00;
	
	if(xspec->root)
	{
		//placeholders for length bytes
		strcpy((char*)xsf->path,".\\");
		
		strcat((char*) (xsf->path + 2), xspec->root);
		
		*(strrchr((char*)xsf->path, '\\')) = 0x00;
		
		// remember total length of path...
		// add one for the extra 'total length byte' 
		pathlen = strlen((char*) (xsf->path + 2)) + 1;
		
		// record the length byte
		*(xsf->path) = pathlen;
													  
		for(cp = (char*)(xsf->path + pathlen), dirlen = 0;cp > (char*)xsf->path; --cp)
		{ 
			if(*cp == '\\')
			{
				*cp = dirlen;
				dirlen = 0;
			}
			else
			{
				dirlen++;
			}
		}
	}
	
	xsf->xi.sendAs = 1; // BINARY
	
	HANDLE handle;
	WIN32_FIND_DATA wfd;
	
	handle = FindFirstFile( xspec->path, &wfd);
	
	if( INVALID_HANDLE_VALUE != handle)
	{
		xsf->xi.bytesTotal = wfd.nFileSizeLow;
		FindClose(handle);
	}
	else
	{
		xsf->xi.bytesTotal = 0;
	}
	
#endif
}

void
CXferThread::AbortSend(uchar localStreamID, ulong *salt)
{
	uchar *p, *e;
	
	// Send an _xst_AbortStream to abort the receiver for this stream
	e = p = (uchar *)safe_malloc(10);	pgpAssert(p);
	*e++ = _xst_AbortStream;
	*e++ = localStreamID;
	pgp_memcpy( e, salt, XFERSALTCHECKSIZE );	e += XFERSALTCHECKSIZE;
	mOutQueue->Send(_mt_filePacket, p, e-p, 1);
}

void
CXferThread::AbortReceive(uchar remoteStreamID, ulong *salt)
{
	uchar *p, *e;
	
	// Send an _xst_RemoteAbortStream to abort the sender for this stream
	e = p = (uchar *)safe_malloc(10);	pgpAssert(p);
	*e++ = _xst_RemoteAbortStream;
	*e++ = remoteStreamID;
	pgp_memcpy( e, salt, XFERSALTCHECKSIZE );	e += XFERSALTCHECKSIZE;
	mOutQueue->Send(_mt_filePacket, p, e-p, 1);
}

CMessageQueue *
CXferThread::GetQueue()
{
	return mInQueue;
}

Boolean
CXferThread::HashPartialFile(XferInfo *xi, SHA *hash, uchar *hashfinal)
{
	LThread *fileThread;
	SHA *cloneHash;
	long fileThreadResult, pull, pullSize, bDone=0;
	Boolean done=FALSE, aborted=FALSE;
	uchar *data;
	
	// Given a file, this function loads it in through the normal pipe
	// mechanism and hashes it to a certain position in order to match
	// the contents on each side.  It is used by the sender to confirm
	// the hash received from the receiver, and by the receiver when it
	// believes it is receiving the completion of a previous partial file.
	
#ifdef PGP_MACINTOSH
	xi->sendAs = SendFileAs(&xi->file, mPFWindow->GetControlThread()->GetRemoteSystemType());
#endif
	xi->pipe = new CPipe(STDPIPESIZE, STDPIPEEXTRA);
	fileThread = new CFileSendPipe(xi, TRUE, (void **)&fileThreadResult);
	xi->pipe->SetPusher(fileThread);
	xi->pipe->SetPuller(this);
	fileThread->Resume();
	while(!done && !aborted)
	{
		pullSize = pull = 1024;
		if(xi->bytesDone)
			if(pull+bDone>xi->bytesDone)
			{
				pull = xi->bytesDone-bDone;
				done = TRUE;
			}
		xi->pipe->DoStartPipePull(&pull, (void **)&data, 0);
		if(pull<pullSize)
			done = TRUE;
		if(pull)
		{
			bDone += pull;
			hash->Update(data, pull);
		}
		xi->pipe->DoEndPipePull(-1);
		(Yield)();
		aborted = xi->pipe->DoPipeAborted();
	}
	if(!aborted)
		xi->pipe->DoAckEndPipe();
	delete xi->pipe;
	xi->pipe = NIL;
	if(!xi->bytesDone)
		xi->bytesDone = bDone;
	if(!aborted)
	{
		cloneHash = hash->Clone();
		cloneHash->Final(hashfinal);
		delete cloneHash;
		return TRUE;
	}
	else
		xi->fatalErr = 0;
	return FALSE;
}

long
CXferThread::GetNextSendPacket(long maxSize)
{
	Boolean done = FALSE, aborted = FALSE;
	long pullSize, pull, sentBytes=0;
	uchar *data, *p, *e, str[80];

	// Activating the following mutex causes a deadlock that is
	// difficult to eliminate.  Using these mutexes is probably
	// desirable however in the long run.  The deadlock scenario:
	//  CPFPacketsOut:	mOutQueue->mFreeSpace is full
	//					calls this func which grabs mSendMutex
	//	CXferThread:	already has mSendMutex
	//					Has called OpenStream to send a packet
	//
	//	This scenario also occurs with a number of the abort
	//	packets.
	//	Eliminating the mutex eliminates the deadlock and
	//	I don't believe it will result in actual problems.
	
	//mSendMutex->Wait();
	if(mSending && mSending->openAckd && mSending->xi.pipe)
	{
		pullSize = pull = minl(maxSize, MAXXFERPACKETSIZE);
		mSending->xi.pipe->DoStartPipePull(&pull, (void **)&data, 0);
		if(pull<pullSize)
			done=TRUE;
		if(pull)
		{
			mSending->xi.bytesDone += pull;
			mSending->hash->Update(data, pull);
			
			mSending->xi.lastTime=pgp_getticks();
			p=e=(uchar *)safe_malloc(pull+2);
			*e++ = _xst_DataStream;
			*e++ = mSending->localStreamID;
			pgp_memcpy(e, data, pull);	e+=pull;
			mPacketThread->Send(_mt_filePacket, p, e-p);
			sentBytes += e-p;
#ifdef	DEBUGXFERLOG
			DebugLog("Send: _xst_DataStream: len %ld", (long)(e-p-2));
#endif
			safe_free(p);
		}
		mSending->xi.pipe->DoEndPipePull(-1);
		aborted = mSending->xi.pipe->DoPipeAborted();
		if(done && !aborted)
		{
			p=e=(uchar *)safe_malloc(6 + SHS_DIGESTSIZE);
			*e++ = _xst_EndStream;
			*e++ = mSending->localStreamID;
			pgp_memcpy( e, &mSending->salt[0], XFERSALTCHECKSIZE );
			e += XFERSALTCHECKSIZE;
			mSending->hash->Final(e);			e+=SHS_DIGESTSIZE;
			mPacketThread->Send(_mt_filePacket, p, e-p);
			sentBytes += e-p;
			safe_free(p);
#ifdef	DEBUGXFERLOG
			DebugLog("Send: _xst_EndStream");
#endif
			mSending->xi.pipe->DoAckEndPipe();
			GetFileName(&mSending->xi, str);
			CStatusPane::GetStatusPane()->AddStatus(0,
				"Sent: %s (%ld K).", str, maxl(mSending->xi.bytesTotal / 1024,1L));
			EndSend();
			if(!mOpeningStream)
				CheckForNewStreams();
		}
		if(aborted)
			mInQueue->Send(_mt_abortSend);
	}
	//mSendMutex->Signal();
	return sentBytes;
}