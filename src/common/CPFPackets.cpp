/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFPackets.cpp,v 1.6 1999/03/10 02:35:19 heller Exp $
____________________________________________________________________________*/

//////////////////////////////////////////////////////////////////////
//	This module implements an asynchronous reliable and unreliable
//	datagram service that routes packets among the various threads
//	above it and the transport layer below it.
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "CPFPackets.h"

#include "CControlThread.h"
#include "CCounterEncryptor.h"
#include "CPriorityQueue.h"
#include "CPacketWatch.h"
#include "CPFTransport.h"
#include "CPFWindow.h"
#include "CSoundInput.h"
#include "CSoundOutput.h"
#include "CRC.h"
#include "CStatusPane.h"
#include "PGPFoneUtils.h"
#ifdef PGPXFER
#include "CXferThread.h"
#endif
#ifdef PGP_MACINTOSH
#include "CPFTAppleTalk.h"
#include <UEnvironment.h>
#endif

#define BANDWIDTHIP				150000	// default bandwidth estimates
#define BANDWIDTHSERIAL9600		960
#define BANDWIDTHSERIAL14400	1370
#define BANDWIDTHSERIAL28800	2700

#define RELIABLESTART		_hpt_Control
#define LASTPACKETTYPE		_hpt_File

#define HPPPACKETDELIM		0x7E	// At start and end of each packet
#define HPPPACKETESCAPE		0x7D	// Prefix for escaped characters
#define HPPNEEDSESCAPE(c) ((((c) - HPPPACKETESCAPE) & 0xff) < 2)
#define HPPESCAPE(c)		((c) ^ 0x20)// transforms an escaped character
#define HPPUNESCAPE(c)		HPPESCAPE(c)// transform is self-inverse

HPPReliablePacket	*sControlReliables, *sFileReliables;
LMutexSemaphore		*sInOutMutex;

ulong sRTTSequence, sRTTBase, sIntervalms,		// milliseconds btwn snd sound pkts
		sRFileSequence, sSFileSequence, sRControlSequence, sSControlSequence,
		sLastRRR,								// last rcvd reception report time
		sLastSRR;								// last sent reception report time
long sBandwidthcps, sBandwidthpp, sACKWaitAverage, sACKWaitTime,
		sJitter,								// jitter for rcvd packets
		sJVariance,								// arrival variance
		sSkew,									// clock skew
		sMTU;									// Maximum Transfer Unit

#ifdef	PGP_MACINTOSH
#include <OpenTransport.h>
// On Macs with OT installed, we use a more precise measurement
// which gives us accurate milliseconds
OTTimeStamp sOTRTTBase;
#endif

CPFPacketsIn::CPFPacketsIn(Boolean packetMode,
				CPFWindow *window,
				CSoundOutput *soundOutput,
				CPFTransport *transport,
				CMessageQueue *controlQueue,
				CMessageQueue *outQueue,
				CMessageQueue *soundOut,
				void **outResult)
#ifndef PGP_WIN32
        : LThread(0, thread_DefaultStack, threadOption_UsePool, outResult)
#else
        : LThread(outResult)
#endif  // PGP_WIN32
{
	sInOutMutex				= new LMutexSemaphore;
	mPFWindow				= window;
	mTransport				= transport;
	mSoundOutput			= soundOutput;
	mControlQueue			= controlQueue;
	mSoundOutQueue			= soundOut;
	mXferQueue				= NIL;
	mOutQueue				= outQueue;
	mPacketMode				= packetMode;
	mDecryptor				= NIL;
	mNoCrypto				= FALSE;
	mInputHead		=
		mInputTail			= mInputBuffer;
	mAbort					= FALSE;
	mSequence		=
		sSControlSequence	=
		sRControlSequence	=
		sSFileSequence		=
		sRFileSequence		= 0;
	sBandwidthcps			= BANDWIDTHIP;
	sMTU					= 1040;
	if(gPGFOpts.popt.connection == _cme_Serial)
	{
		switch(gPGFOpts.sopt.maxBaud)
		{
			case 0:
				sBandwidthcps = BANDWIDTHSERIAL9600;
				sMTU = 256;
				break;
			case 1:
				sBandwidthcps = BANDWIDTHSERIAL14400;
				sMTU = 256;
				break;
			case 2:
				sBandwidthcps = BANDWIDTHSERIAL28800;
				sMTU = 512;
				break;
		}
	}
#ifdef PGP_MACINTOSH
	else if(gPGFOpts.popt.connection == _cme_AppleTalk)
	{
		sMTU = MAXPGPFDDPSIZE;
	}
#endif
	sIntervalms = 200;
	sJitter = 0;
	sJVariance = 0;
	sSkew = 0;
	
	mJitterCount = 0;
	sLastSRR = sLastRRR = 0;
	mLateTrack = 0;
	mSavedOOPkts = NIL;
	mNumSavedOOPkts = 0;
	sACKWaitTime = sACKWaitAverage = DEFAULTACKWAITTIME;
#ifdef PGP_WIN32
	//windows 95 bug so we do this all the time
	mAbortEvent_foo = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (mAbortEvent_foo == INVALID_HANDLE_VALUE)
		pgp_errstring("error creating abort event");
	mAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (mAbortEvent == INVALID_HANDLE_VALUE)
		pgp_errstring("error creating abort event");
#endif	// PGP_WIN32
}

CPFPacketsIn::~CPFPacketsIn()
{
	SavedOOPacket *soop, *lsoop;
	HPPReliablePacket *hppr;
		
	if(mDecryptor)
		delete mDecryptor;
	for(soop=mSavedOOPkts;soop;)
	{
		// Delete any remaining reliable out of order packets
		lsoop=soop;
		soop=soop->next;
		safe_free(lsoop->data);
		safe_free(lsoop);
	}
#ifdef PGP_WIN32
	if(mAbortEvent != INVALID_HANDLE_VALUE)
		CloseHandle(mAbortEvent);
	if(mAbortEvent_foo != INVALID_HANDLE_VALUE)
		CloseHandle(mAbortEvent_foo);
#endif	// PGP_WIN32
	for(hppr = sControlReliables;hppr;)
	{
		sControlReliables = sControlReliables->next;
		if(hppr->data)
			safe_free(hppr->data);
		safe_free(hppr);
		hppr = sControlReliables;
	}
	for(hppr = sFileReliables;hppr;)
	{
		sFileReliables = sFileReliables->next;
		if(hppr->data)
			safe_free(hppr->data);
		safe_free(hppr);
		hppr = sFileReliables;
	}
	delete sInOutMutex;
}

CPFPacketsOut::CPFPacketsOut(Boolean packetMode,
								CPFWindow *window,
								CPFTransport *transport,
								CSoundInput *soundIn,
								CMessageQueue *controlQueue,
								CMessageQueue *soundQueue,
								void **outResult)
#ifndef PGP_WIN32
        : LThread(0, thread_DefaultStack, threadOption_UsePool, outResult)
#else
        : LThread(outResult)
#endif  // PGP_WIN32
{
	short inx;
	
	mPFWindow = window;
	mTransport = transport;
	mSoundQueue = soundQueue;
	mControlQueue = controlQueue;
	mXferThread = NIL;
	mSoundInput = soundIn;
	mPacketMode = packetMode;
	mEncryptor = NIL;
	mAbort = FALSE;
	mSendingSound = FALSE;
	for(inx = 0;inx<NUMHPPASYNCS;inx++)
		mTransport->NewAsync(&mAsyncs[inx]);
	mNextAsync = 0;
	sControlReliables = sFileReliables = NIL;
	mSequence = 0;
	mReceiverWindow = MAXSAVEDOOPACKETS;
	sRTTSequence = 0;
	sRTTBase = pgp_getticks();
#ifdef PGP_WIN32
	mAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (mAbortEvent == INVALID_HANDLE_VALUE)
		pgp_errstring("error creating abort event");
#endif	// PGP_WIN32
}

CPFPacketsOut::~CPFPacketsOut()
{
	for(short inx = 0;inx<NUMHPPASYNCS;inx++)
		mTransport->DeleteAsync(&mAsyncs[inx]);
	if(mEncryptor)
		delete mEncryptor;
#ifdef PGP_WIN32
	if (mAbortEvent != INVALID_HANDLE_VALUE)
		CloseHandle(mAbortEvent);
#endif	// PGP_WIN32
}

// Package and send a packet.
// A packet is surrounded by HPPPACKETDELIM characters, and contains the
// following:
// Packet type (1 byte)
// Sequence Number (1 byte)
// Packet data (variable length)
// CRC (2 bytes, 4 bytes if reliable packet)
//
// If either of the bytes HPPPACKETDELIM or HPPPACKETESCAPE appear
// anywhere in the packet, they are escaped by replacing them
// with HPPPACKETESCAPE followed by an escaped form of the character.
// The escaping is done by XORing 0x20 with the character.

void
CPFPacketsOut::StreamSendPacket(enum HPPPacketType type, ulong seq, long len,
						uchar *data, Boolean lastPacket)
{
	HPPReliablePacket *hppr, *walk;
	uchar *buffer, *p, c, *r, fb;
	uchar iv[6], crcs[4];
	ushort crc = 0xffff;
	ulong crc32 = 0xffffffff;
	short inx;
	Boolean reliable;

	pgpAssert(len <= HPPMAXPKTLEN);
	r=data;
	fb = *data;
	if((type != _hpt_Control) && (type != _hpt_ACK))
	{
		mTXKeyChangeMutex.Wait();
		
		// Setup the counter for counter mode
		// Counter format:
		// Byte 1		Packet Type
		// Byte 2		Packet Counter, mod 2^40
		// Bytes 7-8	Cipher block counter
		
		iv[0] = (uchar)type;
		iv[1] = 0;
		LONG_TO_BUFFER(seq, &iv[2]);

		if(mEncryptor)
			mEncryptor->Encrypt(data, len, iv);
		mTXKeyChangeMutex.Signal();
	}
	mTransport->WaitAsync(&mAsyncs[mNextAsync]);
	mAsyncs[mNextAsync].buffer = p = buffer = (uchar *)safe_malloc(HPPBUFFERSIZE);
	if(type >= RELIABLESTART)
		reliable = TRUE;
	else
		reliable = FALSE;
	*p++ = HPPPACKETDELIM;	// Ensure packet starts with a delimiter
	// escape and CRC the type byte
	c = type;
	if(reliable)
		crc32 = updatecrc32(c, crc32);
	else
		crc = updatecrc16(c, crc);
	if(HPPNEEDSESCAPE(c))
	{
		*p++ = HPPPACKETESCAPE;
		c = HPPESCAPE(c);
	}
	*p++ = c;
	// escape and CRC the sequence number
	c = (uchar)seq;
	if(reliable)
		crc32 = updatecrc32(c, crc32);
	else
		crc = updatecrc16(c, crc);
	if(HPPNEEDSESCAPE(c))
	{
		*p++ = HPPPACKETESCAPE;
		c = HPPESCAPE(c);
	}
	*p++ = c;
	// escape and CRC the packet data
	for(;;)
	{
		c = *r++;
		len--;
		if(reliable)
			crc32 = updatecrc32(c, crc32);
		else
			crc = updatecrc16(c, crc);
		if(HPPNEEDSESCAPE(c))
		{
			*p++ = HPPPACKETESCAPE;
			c = HPPESCAPE(c);
		}
		*p++ = c;
		if(!len)
			break;
	}
	// Now escape the CRC
	if(reliable)
	{
		crc32 = ~crc32;
		for(inx=0;inx<4;inx++)
		{
			// CRC-32 is implemented as little-endian, so we have to make
			// sure that the bytes get inserted in little endian order
			// here.
			
			crcs[inx] = (unsigned char) crc32 & 0xff;
			crc32>>=8;
		}
		for(inx=0;inx<4;inx++)
		{
			c = crcs[inx];
			if(HPPNEEDSESCAPE(c))
			{
				*p++ = HPPPACKETESCAPE;
				c = HPPESCAPE(c);
			}
			*p++ = c;
		}
	}
	else
	{
		c = (crc >> 8) & 0xff;
		if(HPPNEEDSESCAPE(c))
		{
			*p++ = HPPPACKETESCAPE;
			c = HPPESCAPE(c);
		}
		*p++ = c;
		c = crc & 0xff;
		if(HPPNEEDSESCAPE(c))
		{
			*p++ = HPPPACKETESCAPE;
			c = HPPESCAPE(c);
		}
		*p++ = c;
	}

	// If no more packets are expected for a while, add a delimiter
	if(lastPacket)
		*p++ = HPPPACKETDELIM;

	// We're finished.  Send it.
	len = p-buffer;
	if(type >= RELIABLESTART)
	{
		hppr = (HPPReliablePacket *)safe_malloc(sizeof(HPPReliablePacket));
		memset(hppr, 0, sizeof(HPPReliablePacket));
		sInOutMutex->Wait();
		if(type == _hpt_File)
		{
			if(sFileReliables)
			{
				for(walk = sFileReliables;walk->next;walk=walk->next)
					;
				walk->next = hppr;
			}
			else
				sFileReliables = hppr;
		}
		else
		{
			if(sControlReliables)
			{
				for(walk = sControlReliables;walk->next;walk=walk->next)
					;
				walk->next = hppr;
			}
			else
				sControlReliables = hppr;
		}
		hppr->type = type;
		hppr->seq = seq;
		hppr->subType=fb;
		hppr->firstXmitTime = hppr->lastXmitTime = pgp_getticks();
		hppr->data = buffer;
		safe_increaseCount(buffer);
		hppr->len = (short) len;
		sInOutMutex->Signal();
	}
	// send the data to be encrypted or to the modem
	mTransport->WriteAsync(len, _pfc_Control, &mAsyncs[mNextAsync++]);
	mNextAsync %= NUMHPPASYNCS;
	if(gPacketWatch)
		gPacketWatch->SentPacket();
}

// This is organized as a state machine, with GOTOs between states.
// If the mAbort flag is set, anything will jump to the abort state,
// which returns failure.  (Checked before each I/O and before returning.)
// Otherwise, all jumps are backwards, with the expected flow being
// downwards through the code to the bottom.  The states are:
//
// packetfind: Search for a packet delimiter in the input stream
// packetstart: Just past a packet delimiter, expecting a packet type
//		followed by a packet body.  If a packet delimiter is found in an
//		unexpected place, jump straight back to packetstart.
//		When an ending delimiter is found, check the length and CRC.
//		If they're good, return the packet.  Since the ending delimiter
//		can also be the starting delimiter of the next packet, the input
//		is backed up one character to make the next invocation work.
//		If the CRC is not valid, go to packetstart.

Boolean
CPFPacketsIn::StreamGetPacket(enum HPPPacketType *type, uchar **buffer, short *len,
								ulong *seq)
{
	ushort inlen, outlen, crc;
	uchar iv[6];
	uchar c, seqbyte, *p;
	ulong sequence, crc32;
	Boolean crypted=FALSE, reliable;

	*buffer = (uchar *)safe_malloc(HPPMAXPKTLEN+2);
	if(!*buffer)
	{
		*len = 0;
		return crypted;
	}
	// Skip bytes until the next packet header
packetfind:
	// Ensure that we have some data to examine
	if(mInputHead >= mInputTail && GetData() < 0)
		goto abort;
	// Scan for the start of a packet
	for (;;)
	{
		inlen = mInputTail - mInputHead;
		mInputHead = (uchar*)memchr(mInputHead, HPPPACKETDELIM, inlen);
		if(mInputHead)
			break;
		if(GetData() < 0)
			goto abort;
	}
	mInputHead++;	// Skip packet start delimiter
packetstart:
	// We have a valid packet start - get type
	if(mInputHead >= mInputTail && GetData() < 0)
		goto abort;
	c = *mInputHead++;
	if(HPPNEEDSESCAPE(c))
	{
		if(c == HPPPACKETDELIM)
			goto packetstart;	// Zero-length "packet" - totally ignore
		// Okay, so it starts with an escaped byte
		if(mInputHead >= mInputTail && GetData() < 0)
			goto abort;
		c = *mInputHead++;
		if(c == HPPPACKETDELIM)
		{
			if(gPacketWatch)
				gPacketWatch->GotPacket(FALSE);	// Bad packet
			goto packetstart;
		}
		c = HPPUNESCAPE(c);
	}
	*type = (enum HPPPacketType)c;
	if(*type >= RELIABLESTART)
		reliable = TRUE;
	else
		reliable = FALSE;
	// Start CRC computation
	if(reliable)
		crc32 = updatecrc32(c, 0xffffffff);
	else
		crc = updatecrc16(c, 0xffff);
	
	// Get Sequence number
	if(mInputHead >= mInputTail && GetData() < 0)
		goto abort;
	c = *mInputHead++;
	if(HPPNEEDSESCAPE(c))
	{
		if(c == HPPPACKETDELIM)
		{
			if(gPacketWatch)
				gPacketWatch->GotPacket(FALSE); // Bad packet
			goto packetstart;
		}
		// Okay, so it starts with an escaped byte
		if(mInputHead >= mInputTail && GetData() < 0)
			goto abort;
		c = *mInputHead++;
		if(c == HPPPACKETDELIM)
		{
			if(gPacketWatch)
				gPacketWatch->GotPacket(FALSE); // Bad packet
			goto packetstart;
		}
		c = HPPUNESCAPE(c);
	}
	if(reliable)
		crc32 = updatecrc32(c, crc32);
	else
		crc = updatecrc16(c, crc);
	seqbyte = c;
	
	// Get body of packet
	p = *buffer;
	outlen = 0;

	inlen = mInputTail-mInputHead;
	for (;;)
	{
		if(!inlen--)
		{
			if(GetData() < 0)
				goto abort;
			inlen = mInputTail-mInputHead-1;
		}
		c = *mInputHead++;
		if(HPPNEEDSESCAPE(c))
		{
			if(c == HPPPACKETDELIM)
				break;	// End of packet!
			if(!inlen--)
			{
				if(GetData() < 0)
					goto abort;
				inlen = mInputTail-mInputHead-1;
			}
			c = *mInputHead++;
			if(c == HPPPACKETDELIM)
			{
				if(gPacketWatch)
					gPacketWatch->GotPacket(FALSE);	// Bad packet
				goto packetstart;
			}
			c = HPPUNESCAPE(c);
		}
		if(++outlen > HPPMAXPKTLEN + 2)	// Packet too long - skip
			goto packetfind;
		*p++ = c;
		if(reliable)
			crc32 = updatecrc32(c, crc32);
		else
			crc = updatecrc16(c, crc);
	}
	// We now have a complete packet
	if(mAbort)
		goto abort;
	if((reliable && ((crc32 != 0xDEBB20E3) || (outlen < 4))) ||
		(!reliable && ((crc != 0) || (outlen < 2))))	// Bad CRC?
	{
		if(gPacketWatch)
			gPacketWatch->GotPacket(FALSE);	// Bad packet
		goto packetstart;
	}
	mInputHead--;		// Back up so next invocation can find packet delim
	
	*len = outlen - (reliable ? 4 : 2);      // Chop off CRC, leaving only user data
	// Find the last encountered sequence number
	if(*type >= RELIABLESTART)
	{
		if(*type == _hpt_File)
			sequence = sRFileSequence;
		else
			sequence = sRControlSequence;
	}
	else
		sequence = mSequence;
	
	// Find the full sequence number of the current packet
	sequence += (long)(signed char)(seqbyte-sequence);
	*seq = sequence;
	
	// Okay, now decrypt the thing
	if((*type != _hpt_ACK) && (*type != _hpt_Control))
	{
		// Create counter mode iv
		iv[0] = (uchar)*type;
		iv[1] = 0;
		LONG_TO_BUFFER(sequence, &iv[2]);
	
		mRXKeyChangeMutex.Wait();
		if(mDecryptor)
		{
			mDecryptor->Decrypt(*buffer, *len, iv);
			crypted = TRUE;
		}
		mRXKeyChangeMutex.Signal();
	}
	return crypted;
abort:
	safe_free(*buffer);
	*buffer = NIL;
	*len = 0;
	*type = (enum HPPPacketType)0;
	return crypted;
}

short
CPFPacketsIn::GetData(void)
{
	long read;
	short channel;

	do {
		if(mAbort)
			return -1;
#ifdef PGP_WIN32
		// sleeps until a character arrives or our abort event is triggered
		if (!ResetEvent(mAbortEvent))
		{
			DWORD error = GetLastError();
			char	str[100];					

			sprintf(str, "Abort event error %ld",  (error));
			pgp_errstring(str);				

		}
		short result = mTransport->SleepForData(&mAbortEvent);
		if (result != 0)
		{
			ResetEvent(mAbortEvent);
			return -1;	// error (< 0) or aborted (> 0)
		}
		ResetEvent(mAbortEvent);
#else
		(Yield)();
#endif	// PGP_WIN32
		read = mTransport->Read(mInputBuffer, HPPBUFFERSIZE, &channel);
	} while (!read);
	mInputHead = mInputBuffer;
	mInputTail = mInputBuffer + read;
	return 0;
}

//	Datagram style packet format
//	
//	Type		:	1 byte
//	Sequence	:	2 bytes
//	Data		:	<=	HPPMAXPKTLEN(1024)
//  CRC     	:   4 bytes (reliable packets only)
//
//	SendPacket is designed to skip bit stuffing in favor of the
//	above format for use over networks and other channels where
//	bit stuffing is pointless or redundant.

void
CPFPacketsOut::SendPacket(enum HPPPacketType type, ulong seq, long len, uchar *data)
{
	HPPReliablePacket *hppr, *walk;
	uchar *sendbuf, *p, fb;
	ulong crc32, timestamp;
	uchar iv[6];
	ushort seqshort;
	
	pgpAssert(len <= HPPMAXPKTLEN);
	mTransport->WaitAsync(&mAsyncs[mNextAsync]);
	mAsyncs[mNextAsync].buffer = sendbuf = (uchar *)safe_malloc(1024+7);
	p = sendbuf;
	*p++ = type;
	seqshort = (ushort)seq;
	SHORT_TO_BUFFER(seqshort, p);	p+= 2;
	fb = *data;
	if(type == _hpt_Voice)
	{
		timestamp = pgp_milliseconds();
		LONG_TO_BUFFER(timestamp, p);	p+=4;
		pgp_memcpy(p, data, len);		p+=len;
		len += 4;
	}
	else
	{
		pgp_memcpy(p, data, len);	p+=len;
	}
	if((type != _hpt_ACK) && (type != _hpt_Control))
	{
		mTXKeyChangeMutex.Wait();
		// Setup the counter for counter mode
		// Counter format:
		// Byte 1		Packet Type
		// Byte 2		Packet Counter, mod 2^40
		// Bytes 7-8	Cipher block counter
		
		iv[0] = (uchar)type;
		iv[1] = 0;
		LONG_TO_BUFFER(seq, &iv[2]);
		if(mEncryptor)
			mEncryptor->Encrypt(sendbuf+3, len, iv);
		mTXKeyChangeMutex.Signal();
	}
	len+=3;
	if(type >= RELIABLESTART)
	{
		crc32 = CalcCRC32(sendbuf, len);
		LONG_TO_BUFFER(crc32, p);	p+=4;
		len+=4;
		hppr = (HPPReliablePacket *)safe_malloc(sizeof(HPPReliablePacket));
		memset(hppr, 0, sizeof(HPPReliablePacket));
		sInOutMutex->Wait();
		if(type == _hpt_File)
		{
			if(sFileReliables)
			{
				for(walk = sFileReliables;walk->next;walk=walk->next)
					;
				walk->next = hppr;
			}
			else
				sFileReliables = hppr;
		}
		else
		{
			if(sControlReliables)
			{
				for(walk = sControlReliables;walk->next;walk=walk->next)
					;
				walk->next = hppr;
			}
			else
				sControlReliables = hppr;
		}
		hppr->type = type;
		hppr->seq = seq;
		hppr->subType=fb;
		hppr->firstXmitTime = hppr->lastXmitTime = pgp_getticks();
		hppr->data = sendbuf;
		safe_increaseCount(sendbuf);
		hppr->len = (short) len;
		sInOutMutex->Signal();
	}
	mTransport->WriteAsync(len, (type==_hpt_Voice)? _pfc_Media : _pfc_Control, &mAsyncs[mNextAsync++]);
	mNextAsync %= NUMHPPASYNCS;
	if(gPacketWatch)
		gPacketWatch->SentPacket();
}

//	The GetPacket routine assumes that all Read calls return an HPP
//	packet.  A lower layer is expected to be doing the packetization.
//	This is the case under Internet UDP and AppleTalk DDP.

Boolean
CPFPacketsIn::GetPacket(enum HPPPacketType *type, uchar **buffer, short *len, ulong *seq)
{
	long read, dlen, baavg, baskew;
	ulong crc32, pkcrc32, sequence, timestamp, timestampnow;
	uchar iv[6];
	ushort seqshort;
	short channel;
	Boolean goodcrc, crypted=FALSE;
	
	*buffer = NIL;
	while(!mAbort)
	{
		read = mTransport->Read(mInputBuffer, HPPBUFFERSIZE, &channel);
		if(read)
		{
			goodcrc = FALSE;
			if(mInputBuffer[0] >= RELIABLESTART)
			{
				// we only CRC control packets in packet mode
				if(read >= 6)
				{
					crc32 = CalcCRC32(mInputBuffer, read - 4);
					BUFFER_TO_LONG(mInputBuffer + read - 4, pkcrc32);
					if(crc32 == pkcrc32)
						goodcrc = TRUE;
					read -=4;
				}
			}
			else
				goodcrc = TRUE;
			dlen = read - 3;        // Allow for type and sequence bytes
			if(goodcrc && (mInputBuffer[0] <= LASTPACKETTYPE) &&
				(dlen <= HPPMAXPKTLEN))
			{
				*type = (enum HPPPacketType)mInputBuffer[0];
				if((*buffer = (uchar*)safe_malloc(dlen)) != NULL)
				{
					BUFFER_TO_SHORT(&mInputBuffer[1], seqshort);
					
					// Find the last encountered sequence number
					if(*type >= RELIABLESTART)
					{
						if(*type == _hpt_File)
							sequence = sRFileSequence;
						else
							sequence = sRControlSequence;
					}
					else
						sequence = mSequence;
					
					// Find the full sequence number of the current packet
					sequence += (long)(signed short)(seqshort - sequence);
					*seq = sequence;
					if((*type != _hpt_ACK) && (*type != _hpt_Control))
					{
						iv[0] = (uchar)*type;
						iv[1] = 0;	// +++++ should be overflow bits
						LONG_TO_BUFFER(sequence, &iv[2]);
						mRXKeyChangeMutex.Wait();
						if(mDecryptor)
						{
							mDecryptor->Decrypt(mInputBuffer + 3, dlen, iv);
							crypted = TRUE;
						}
						mRXKeyChangeMutex.Signal();
					}
					if((*type == _hpt_Voice) && (dlen > 5))
					{
						dlen-=4;
						*len = (short) dlen;
						pgp_memcpy(*buffer, mInputBuffer + 7, dlen);
						BUFFER_TO_LONG(mInputBuffer + 3, timestamp);
						timestampnow = pgp_milliseconds();
						baskew = timestampnow-timestamp;
						if(mJitterCount > 3)
							sSkew += (baskew-sSkew)/8;
						else
							sSkew = baskew;
						baavg = labs(baskew-sSkew);
						sJVariance += (baavg-sJVariance)/16;
						sJitter = sJVariance * 2;		// old: sJVariance * 2;
						// max = sJitter + sSkew;
						// if baskew > max, it's late!
						if(baskew > sJitter + sSkew)
						{
							if(++mLateTrack > 2)
							{
								//if(sLastSRR + 5000 < pgp_getticks())
								//	SendRR();
								mLateTrack = 0;
							}
						}
						else
							mLateTrack = 0;
						if(mJitterCount<10)
							mJitterCount++;
						else
						{
							mSoundOutput->SetJitter(sJitter);
							gPacketWatch->SetJitter(sJitter);
						}
					}
					else
					{
						*len = (short) dlen;
						pgp_memcpy(*buffer, mInputBuffer + 3, dlen);
					}
					return crypted;
				}
			}
			else
				gPacketWatch->GotPacket(FALSE); // Log bad packet
		}
		(Yield)();
	}
	return crypted;
}

void
CPFPacketsIn::DeliverPacket(enum HPPPacketType packetType, uchar *packet, short packetLen)
{
	HPPReliablePacket *hppr, *lhpr;
	enum HPPPacketType ackType;
	ulong sq, rttVal;
	ushort seqshort;
	uchar seqbyte;

	switch(packetType)
	{
		case _hpt_ACK:
			ackType = (enum HPPPacketType)((ulong)*packet);
			sInOutMutex->Wait();
			if(ackType == _hpt_File)
			{
				hppr = sFileReliables;
				sq = sSFileSequence;
			}
			else
			{
				hppr = sControlReliables;
				sq = sSControlSequence;
			}
			if(mPacketMode)
			{
				BUFFER_TO_SHORT(packet+1, seqshort);
				//DebugLog("ACK at %ld, on %d", sq, seqshort);
				//CStatusPane::GetStatusPane()->AddStatus(0, "Rcvd ACK at %ld, on %ld", sq, seqshort);
				sq += (long)(signed short)(seqshort - sq);
			}
			else
			{
				seqbyte = *(packet+1);
				sq += (long)(signed char)(seqbyte-sq);
			}
			for(lhpr=NIL;hppr;hppr=hppr->next)
			{
				if((hppr->type == ackType) && (hppr->seq == sq))
				{
					// send ack notification back to control,
					// and include the first byte of the
					// packet so control can identify it.
					if(ackType != _hpt_File)
						mControlQueue->Send(_mt_ack, (void *)hppr->subType);
					if(lhpr)
						lhpr->next = hppr->next;
					else
					{
						if(ackType == _hpt_File)
							sFileReliables = hppr->next;
						else
							sControlReliables = hppr->next;
					}
					if(!hppr->retransmits)
					{
						// Update ACK timeout low-pass filter
						
						sInOutMutex->Wait();
						sACKWaitAverage = (ulong)((0.9 * (float)sACKWaitAverage) +
											(0.1 * (float)(pgp_getticks() - hppr->firstXmitTime)));
						sACKWaitTime = sACKWaitAverage * 2;
						sInOutMutex->Signal();
					}
					else
					{
					}
					safe_free(hppr->data);
					safe_free(hppr);
				}
				lhpr = hppr;
			}
			sInOutMutex->Signal();
			safe_free(packet);
			break;
		case _hpt_Voice:
			mSoundOutQueue->Send(_mt_voicePacket, packet, packetLen, TRUE);
			break;
		case _hpt_Control:
			if(mDecryptor)
			{
				safe_free(packet);
				if(gPacketWatch)
					gPacketWatch->GotPacket(FALSE);
				break;	//ignore unencrypted pkts on encrypted link
			}
		case _hpt_ControlEncrypt:
			mControlQueue->Send(_mt_controlPacket, packet, packetLen, TRUE);
			break;
		case _hpt_RTT:
			BUFFER_TO_LONG(&packet[1], rttVal);
			if(packet[0] == mPFWindow->GetControlThread()->IsOriginator())
			{
				// Make sure this wasn't some RTT packet floating
				// around the network we sent a long time ago
				if(rttVal == sRTTSequence)
				{
#ifdef	PGP_MACINTOSH
					if(UEnvironment::HasFeature(env_HasOpenTransport))
						gPacketWatch->SetRTTime(OTElapsedMilliseconds(&sOTRTTBase));
					else
						gPacketWatch->SetRTTime(pgp_getticks() - sRTTBase);
#else
					gPacketWatch->SetRTTime(pgp_getticks() - sRTTBase);
#endif
				}
				safe_free(packet);
			}
			else
				mOutQueue->Send(_mt_rtt, packet, packetLen, TRUE);
			break;
		case _hpt_ReceptionReport:
#ifdef BETA
			//CStatusPane::GetStatusPane()->AddStatus(0,
			//	"Sound is late on the remote side, adjusting bandwidth.");
#endif
			/*sInOutMutex->Wait();
			sBandwidthcps = sBandwidthcps / 5 * 4;
			sBandwidthpp = sBandwidthcps / (1000 / sIntervalms);
			gPacketWatch->SetBandwidth(sBandwidthcps);
			sLastRRR = pgp_getticks();
			sInOutMutex->Signal();*/
			safe_free(packet);
			break;
		case _hpt_File:
			if(mXferQueue)
				mXferQueue->Send(_mt_filePacket, packet, packetLen, TRUE);
			else
				safe_free(packet);
			break;
		default:
			safe_free(packet);
			break;
	}
}

void
CPFPacketsIn::SendRR()
{
	/*
	+++++
	uchar *p, *e;
	
	p = e = (uchar *)safe_malloc(1);
	*e++ = 0;
	mOutQueue->Send(_mt_rr, p, 1, TRUE);*/
}

void
CPFPacketsIn::AckPacket(enum HPPPacketType packetType, ulong seq)
{
	uchar *p, *ackpkt, seqbyte;
	ushort seqshort;
	
	if(mPacketMode)
	{
		// We use 16 bit sequence numbers in packet mode (over IP and AppleTalk)
		p = ackpkt = (uchar *)safe_malloc(3);
		*p++ = packetType;
		seqshort = (unsigned short) seq & 0xFFFF;
		SHORT_TO_BUFFER(seqshort, p);
		mOutQueue->Send(_mt_ack, ackpkt, 3, TRUE);
	}
	else
	{
		// We use 8 bit sequence numbers in serial mode
		p = ackpkt = (uchar *)safe_malloc(2);
		*p++ = packetType;
		seqbyte = (unsigned char) seq & 0xFF;
		*p++ = seqbyte;
		mOutQueue->Send(_mt_ack, ackpkt, 2, TRUE);
	}
}

void *
CPFPacketsIn::Run(void)
{
	enum HPPPacketType packetType;
	uchar *packet;
	ulong seq, reliableSeq;
	short packetLen;
	Boolean crypted;
	SavedOOPacket *soop, *lsoop;
	Boolean found;						

	while(!mAbort)
	{
		do
		{
			// make sure we haven't received any packets we were waiting for
			// that will allow us to forward some saved packets
			found = FALSE;
			for(lsoop=NIL,soop=mSavedOOPkts;soop;soop=soop->next)
			{
				if(soop->seq == ((soop->type == _hpt_File)?sRFileSequence:sRControlSequence))
				{
#ifdef BETA
					//CStatusPane::GetStatusPane()->AddStatus(0, "Forwarded OO pkt(%ld).", soop->seq);
#endif
					DeliverPacket(soop->type, soop->data, soop->len);
					mNumSavedOOPkts--;
					if(lsoop)
						lsoop->next = soop->next;
					else
						mSavedOOPkts = soop->next;
					safe_free(soop);
					if(packetType == _hpt_File)
						sRFileSequence++;
					else
						sRControlSequence++;
					found = TRUE;
					break;
				}
				lsoop=soop;
			}
		} while(found);
		if(mPacketMode)
			crypted=GetPacket(&packetType, &packet, &packetLen, &seq);
		else
			crypted=StreamGetPacket(&packetType, &packet, &packetLen, &seq);
		if(!packet && mAbort)
			break;
#ifdef BETA
		//CStatusPane::GetStatusPane()->AddStatus(0, "Rcv(%d, %ld).", (short)packetType, seq);
#endif
		if(packetType >= RELIABLESTART)
		{
			if(packetType == _hpt_File)
				reliableSeq = sRFileSequence;
			else
				reliableSeq = sRControlSequence;
			if(seq <= reliableSeq)
			{
				if(!mNoCrypto && (packetType >= _hpt_ControlEncrypt) && !crypted)
				{
					// We aren't ready for encrypted packets yet because the
					// decryptor hasn't been set.  We will drop this packet
					// and expect it to be retransmitted later when we should
					// hopefully be ready.  This packet will not be ACKd.
					safe_free(packet);
					if(gPacketWatch)
						gPacketWatch->GotPacket(FALSE);
#ifdef BETA
					//CStatusPane::GetStatusPane()->AddStatus(0, "Out of band encrypted packet(%ld).", seq);
#endif
				}
				else
				{
					AckPacket(packetType, seq);
					if(seq < reliableSeq)
					{
						safe_free(packet);// ignore it, already processed
						if(gPacketWatch)
							gPacketWatch->GotPacket(FALSE);
#ifdef BETA
						//DebugLog("RXDUPE: %ld", seq);
						//CStatusPane::GetStatusPane()->AddStatus(0, "Duplicate packet(%ld).", seq);
#endif
					}
					else    // seq == reliableSeq, next expected
					{
						if(packetType == _hpt_File)
							sRFileSequence++;
						else
							sRControlSequence++;
						if(gPacketWatch)
							gPacketWatch->GotPacket(TRUE);
						DeliverPacket(packetType, packet, packetLen);
#ifdef BETA
						//CStatusPane::GetStatusPane()->AddStatus(0, "Packet(%ld) OK.", seq);
#endif
					}
				}
			}
			else
			{
				// We received a packet out of order, record it
				// so that we can automatically forward it later
				// when we receive the appropriate fill-in packets.
				if(mNumSavedOOPkts<MAXSAVEDOOPACKETS)
				{
					AckPacket(packetType, seq);
					soop = (SavedOOPacket *)safe_malloc(sizeof(SavedOOPacket));pgpAssert(soop);
					soop->type = packetType;
					soop->data = packet;
					soop->seq = seq;
					soop->len = packetLen;
					soop->next = mSavedOOPkts;
					mSavedOOPkts = soop;
					mNumSavedOOPkts++;
					if(gPacketWatch)
						gPacketWatch->GotPacket(TRUE);
#ifdef BETA
					//CStatusPane::GetStatusPane()->AddStatus(0, "Rcvd OO packet(%ld), expected(%ld).",
					//	seq, reliableSeq);
#endif
				}
				else
				{
					// Too many saved packets, let some of them time out
					// so we receive the ones we lost.
					safe_free(packet);
					if(gPacketWatch)
						gPacketWatch->GotPacket(FALSE);
#ifdef BETA
					//DebugLog("OO exhausted, wasting:%ld", seq);
					//CStatusPane::GetStatusPane()->AddStatus(0, "OO packets exhausted, wasted:(%ld).", seq);
#endif
				}
			}
		}
		else
		{
			if(packetType == _hpt_Setup)
			{
				// We received a stray setup packet from the machine
				// we're connected to.  We ignore setup packets from
				// the remote during a call.  Setup packets from a
				// different machine are handled by the Transport layer.
				safe_free(packet);
				continue;
			}
			else if(seq == mSequence)
			{
				gPacketWatch->GotPacket(TRUE);
				mSequence++;
			}
			else if(seq < mSequence)
			{
				// we already got this or it was out of order
				safe_free(packet);
				if(gPacketWatch)
					gPacketWatch->GotPacket(FALSE);
				continue;
			}
			else // seq > mSequence
			{
				//we lost packets, who cares, automatically resync
				mSequence = ++seq;
				if(gPacketWatch)
				{
					gPacketWatch->GotPacket(FALSE);	//we're really talking about the lost packet(s) here
					gPacketWatch->GotPacket(TRUE);	//but this packet is good
				}
			}
			DeliverPacket(packetType, packet, packetLen);
		}
		(Yield)();
	}
	return (void *)1L;
}

void
CPFPacketsOut::SetSound(ulong intervalms)
{
	sInOutMutex->Wait();
	sIntervalms		= intervalms + 5;	// pad it a bit so we don't miss a lot
	sBandwidthpp	= sBandwidthcps / (1000 / sIntervalms);
	sInOutMutex->Signal();
}

void
CPFPacketsOut::SetSoundStatus(Boolean status)
{
	mSendingSound = status;
}

void
CPFPacketsOut::Send(enum MsgType type, uchar *data, long len)
{
	Boolean isLast;
	
	isLast = TRUE;	//mOutQueue->Peek() ? FALSE : TRUE;
	switch(type)
	{
		case _mt_controlPacket:
			if(mPacketMode)	// are we on a packet switched network?
				SendPacket(mEncryptor ? _hpt_ControlEncrypt : _hpt_Control,
							sSControlSequence++,len,data);
			else			// or are we on a circuit network
				StreamSendPacket(mEncryptor ? _hpt_ControlEncrypt : _hpt_Control,
							sSControlSequence++,len,data,isLast);
			break;
		case _mt_voicePacket:
			if(mPacketMode)
				SendPacket(_hpt_Voice, mSequence++,len,data);
			else
				StreamSendPacket(_hpt_Voice,mSequence++,len,data,isLast);
			break;
		case _mt_abort:
			mAbort = TRUE;
#ifdef PGP_WIN32
			if(mAbortEvent != INVALID_HANDLE_VALUE)
				SetEvent(mAbortEvent);
#endif
			break;
		case _mt_ack:
			if(mPacketMode)
				SendPacket(_hpt_ACK,mSequence++,len,data);
			else
				StreamSendPacket(_hpt_ACK,mSequence++,len,data,isLast);
			break;
		case _mt_rr:
			sLastSRR = pgp_getticks();
			if(mPacketMode)
				SendPacket(_hpt_ReceptionReport,mSequence++,len,data);
			else
				StreamSendPacket(_hpt_ReceptionReport,mSequence++,len,data,isLast);
			break;
		case _mt_rtt:
			if(mPacketMode)
				SendPacket(_hpt_RTT,mSequence++,len,data);
			else
				StreamSendPacket(_hpt_RTT,mSequence++,len,data,isLast);
			break;
		case _mt_filePacket:
			if(mPacketMode)
				SendPacket(_hpt_File,sSFileSequence++,len,data);
			else
				StreamSendPacket(_hpt_File,sSFileSequence++,len,data,isLast);
			break;
		default:
			pgp_errstring("Unknown outgoing HPP packet.");
			break;
	}
}

void
CPFPacketsOut::ServiceControl(long *sentBytes)
{
	PFMessage *msg;
	HPPReliablePacket *hppr;
	long now = pgp_getticks(), b;
	
	// Now look for reliable control packets that must be retransmitted
	sInOutMutex->Wait();
	for(hppr=sControlReliables;hppr;hppr=hppr->next)
	{
		b = now - hppr->lastXmitTime;
		if(b >= sACKWaitTime)
		{
			if(hppr->retransmits < HPPMAXRETRANSMIT)
			{
				mTransport->WaitAsync(&mAsyncs[mNextAsync]);
				mAsyncs[mNextAsync].buffer = hppr->data;
				safe_increaseCount(hppr->data);
				mTransport->WriteAsync(hppr->len, _pfc_Control,
							&mAsyncs[mNextAsync++]);
				mNextAsync %= NUMHPPASYNCS;
				hppr->retransmits++;
				hppr->lastXmitTime = pgp_getticks();
				*sentBytes += hppr->len;
#ifdef BETA
				// Debugging code to show retransmissions
				//CStatusPane::GetStatusPane()->AddStatus(0, "(C)Retransmission %ld.",
				//	(long)hppr->retransmits);
#endif
				if(gPacketWatch)
					gPacketWatch->SentPacket();
			}
			else
			{
				// The remote side has not responded to our reliable packet.
				// Reliable packets are not optional.  We tried to send it many
				// times over a 30 second period.
				// Something is wrong, so abort the protocol.
				// In most cases, this just means the remote went down.
				
				if(now - hppr->firstXmitTime >= MINACKTIMEOUT)
					mPFWindow->GetControlThread()->ProtocolViolation(_pv_noRemote);
				else
				{
					sACKWaitTime += 1000;
					if(sACKWaitTime > DEFAULTACKWAITTIME)
						sACKWaitTime = DEFAULTACKWAITTIME;
					hppr->retransmits--;
				}
			}
		}
	}
	sInOutMutex->Signal();
	// Now look for new control packets.  We always send all available
	// control packets
	while((msg = mControlQueue->Recv(0)) != NULL)
	{
		*sentBytes += msg->len;
		Send(msg->type, (uchar *)msg->data, msg->len);
		mControlQueue->Free(msg);
	}
}

void *
CPFPacketsOut::Run(void)
{
	PFMessage *msg;
	uchar rttData[16];
	long waittime, lastInterval, sentBytes;
	Boolean sentSound;
#ifdef PGPXFER
	long b;
	HPPReliablePacket *hppr;
	Boolean windowok;
	PFMessage *peekmsg;
#endif

	lastInterval = pgp_getticks();
	while(!mAbort)
	{
		sentBytes = 0;
		// First wait for a sound packet to send, always send all sound packets first
		sentSound = FALSE;
		msg = NIL;
		while(mSendingSound && (!sentSound || msg) && !mAbort)
		{
			waittime = msg? 0 : (sIntervalms - (pgp_getticks() - lastInterval));
			if(waittime<0)
				waittime=0;
			msg = mSoundQueue->Recv(waittime);
			if(msg)
			{
				lastInterval = pgp_getticks();
				sentBytes += msg->len;
				Send(msg->type, (uchar *)msg->data, msg->len);
				mSoundQueue->Free(msg);
				sentSound = TRUE;
			}
			else
				ServiceControl(&sentBytes);
			(Yield)();
		}
		if(mAbort)
			continue;
		if(!sentSound)
			lastInterval = pgp_getticks();
		ServiceControl(&sentBytes);
#ifdef	PGPXFER
		// Now with any remaining bandwidth we may send file packets
		// First we handle any necessary retransmission of file packets
		// up to the bandwidth
		if(mXferThread)
		{
			sInOutMutex->Wait();
			for(hppr=sFileReliables;hppr;hppr=hppr->next)
			{
				b = lastInterval - hppr->lastXmitTime;
				if(b >= sACKWaitTime)
				{
					if(sentBytes + hppr->len > sBandwidthpp)
						continue;
					if(hppr->retransmits < HPPMAXRETRANSMIT)
					{
						mTransport->WaitAsync(&mAsyncs[mNextAsync]);
						mAsyncs[mNextAsync].buffer = hppr->data;
						safe_increaseCount(hppr->data);
						mTransport->WriteAsync(hppr->len, _pfc_Control,
									&mAsyncs[mNextAsync++]);
						mNextAsync %= NUMHPPASYNCS;
						hppr->retransmits++;
						hppr->lastXmitTime = pgp_getticks();
						sentBytes += hppr->len;
						//DebugLog("ReTX: %ld", hppr->seq);
#ifdef BETA
						// Debugging code to show retransmissions
						//CStatusPane::GetStatusPane()->AddStatus(0, "(F)Retransmission %ld.",
						//	(long)hppr->retransmits);
#endif
						if(gPacketWatch)
							gPacketWatch->SentPacket();
					}
					else
					{
						// See comment above
						
						if(lastInterval - hppr->firstXmitTime >= MINACKTIMEOUT)
						{
							//HPPReliablePacket *bhppr;
							
							//for(bhppr=sFileReliables;bhppr;bhppr=bhppr->next)
							//	DebugLog("Fatal No Response: seq (%ld), current (%ld)", bhppr->seq, sSFileSequence);
							mPFWindow->GetControlThread()->ProtocolViolation(_pv_noRemote);
						}
						else
						{
							sACKWaitTime += 1000;
							if(sACKWaitTime > DEFAULTACKWAITTIME)
								sACKWaitTime = DEFAULTACKWAITTIME;
							hppr->retransmits--;
						}
					}
				}
			}
			sInOutMutex->Signal();
			b=1;
			windowok = TRUE;
			while(mXferThread && !(peekmsg=mSoundQueue->Peek()) && b &&
					((sBandwidthpp - sentBytes) >= MINXFERPACKETSIZE) &&
					(!sFileReliables || (sSFileSequence - sFileReliables->seq < mReceiverWindow)))
			{
				b = mXferThread->GetNextSendPacket(minl(sBandwidthpp - sentBytes, sMTU-9));
				sentBytes += b;
				(Yield)();
			}
			if(b && !peekmsg && (pgp_getticks() > mLastBWIncrease+5000) &&
				(pgp_getticks() > sLastRRR+6500))
			{
				/*+++++++
				sInOutMutex->Wait();
				mLastBWIncrease = pgp_getticks();
				sBandwidthcps = sBandwidthcps / 10 * 11;	// add 10%
				sBandwidthpp = sBandwidthcps / (1000 / sIntervalms);
				gPacketWatch->SetBandwidth(sBandwidthcps);
				sInOutMutex->Signal();*/
			}
		}
#endif	//PGPXFER
		// And finally we'll send an RTT packet if necessary
		if(lastInterval - sRTTBase >= HPPRTTINTERVAL)
		{
			// HPPRTTINTERVAL has passed since the last time we sent a
			// round trip timing packet to measure the channel delay.
			// We will send one now.
			
			sRTTSequence++;
			sRTTBase = lastInterval;
#ifdef	PGP_MACINTOSH
			if(UEnvironment::HasFeature(env_HasOpenTransport))
				OTGetTimeStamp(&sOTRTTBase);
#endif
			rttData[0] = (uchar) mPFWindow->GetControlThread()->IsOriginator();
			LONG_TO_BUFFER(sRTTSequence, &rttData[1]);
			if(mPacketMode)
				SendPacket(_hpt_RTT,mSequence++,5,rttData);
			else
				StreamSendPacket(_hpt_RTT,mSequence++,5,rttData,TRUE);
			//CStatusPane::GetStatusPane()->AddStatus(0, "Sent RTT");
		}
		(Yield)();
	}
	return (void *)1L;
}

void
CPFPacketsIn::AbortSync()
{
	mAbort = TRUE;
#ifdef	PGP_MACINTOSH
	mTransport->AbortSync();
#elif	PGP_WIN32
	if(mAbortEvent != INVALID_HANDLE_VALUE)
		SetEvent(mAbortEvent);
#endif	// PGP_WIN32
}

void
CPFPacketsIn::ChangeRXKey(uchar *keyMaterial, uchar *firstEA, uchar *,
					uchar )
{
	// we have a mutex surrounding this operation because it will be executed
	// by the control thread, not the HPP-in thread.  The HPP-in thread
	// may be in the middle of a read when control calls this.
	mRXKeyChangeMutex.Wait();
	if(mDecryptor)
		delete mDecryptor;
	mDecryptor = NIL;
	if(memcmp(firstEA, sCryptorHash[_enc_none], 4))
		mDecryptor = new CCounterEncryptor(keyMaterial, firstEA);
	mRXKeyChangeMutex.Signal();
}

void
CPFPacketsOut::ChangeTXKey(uchar *keyMaterial, uchar *firstEA, uchar *,
					uchar )
{
	mTXKeyChangeMutex.Wait();
	if(mEncryptor)
		delete mEncryptor;
	mEncryptor = NIL;
	if(memcmp(firstEA, sCryptorHash[_enc_none], 4))
		mEncryptor = new CCounterEncryptor(keyMaterial, firstEA);
	mTXKeyChangeMutex.Signal();
}

void
CPFPacketsIn::SetXferQueue(CMessageQueue *xferQueue)
{
	mXferQueue = xferQueue;
}

void
CPFPacketsOut::SetXferThread(CXferThread *xferThread)
{
	mXferThread = xferThread;
}

void
CPFPacketsIn::SetNoCryptoMode()
{
	mNoCrypto = TRUE;
}

