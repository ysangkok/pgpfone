/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFTSerial.cpp,v 1.5 1999/03/10 02:36:19 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "CPFTSerial.h"
#include "CPFWindow.h"
#include "CStatusPane.h"
#include "PGPFone.h"

#include <mmsystem.h>

#ifdef _DEBUG
#define MODEM_ERROR(s, e)				\
{										\
	char	str[100];					\
	sprintf(str, "%s(%d{l})", (s), (e));	\
	pgp_errstring(str);					\
}
#else
#define MODEM_ERROR(s, e)	(void)0		// do nothing
#endif	// _DEBUG

CPFTSerial::CPFTSerial(SerialOptions *serOpt, CPFWindow *cpfWindow,
	short *result)
	:	CPFTransport(cpfWindow)
{
	COMMTIMEOUTS	timeouts;

	mPortHandle = INVALID_HANDLE_VALUE;
	mRXevent = mTXevent = INVALID_HANDLE_VALUE;
	mDataEvent = INVALID_HANDLE_VALUE;

	*result = 0;
	mAbort = FALSE;

	pgp_memcpy(mPort, serOpt->port, 5);
	pgp_memcpy(&mSerOpts, serOpt, sizeof(SerialOptions));

	// open the port for reading and writing
	mPortHandle = CreateFile(mPort, GENERIC_READ|GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
	if(mPortHandle == INVALID_HANDLE_VALUE)
	{
		*result = -1;
		MODEM_ERROR("error opening port", GetLastError());
		goto error;
	}

	// setup the input and output buffers
	if(! SetupComm(mPortHandle, 4096, 4096))
	{
		*result = -2;
		MODEM_ERROR("error creating input/output buffers", GetLastError());
		goto error;
	}

	// setup the timeouts for reading and writing
	timeouts.ReadIntervalTimeout         = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier  = 0;
	timeouts.ReadTotalTimeoutConstant    = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant   = 0;

	if(! SetCommTimeouts(mPortHandle, &timeouts))
	{
		*result = -3;
		MODEM_ERROR("error setting port timeouts", GetLastError());
		goto error;
	}

	// create event objects for OVERLAPPED structures in reading and writing
	mRXevent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(mRXevent == INVALID_HANDLE_VALUE)
	{
		*result = -4;
		MODEM_ERROR("error creating event for reading", GetLastError());
		goto error;
	}
	mTXevent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(mTXevent == INVALID_HANDLE_VALUE)
	{
		*result = -5;
		MODEM_ERROR("error creating event for writing", GetLastError());
		goto error;
	}
	mDataEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(mDataEvent == INVALID_HANDLE_VALUE)
	{
		*result = -6;
		MODEM_ERROR("error creating mDataEvent", GetLastError());
		goto error;
	}

	SetState(_cs_none);
	mInBack = mInFront = 0;
	return;
error:
	PGFAlert("Could not open serial port!", 0);
}

CPFTSerial::~CPFTSerial()
{
	AbortSync();
	mTMutex->Wait();

	if(mState == _cs_connected)
		Disconnect();

	if(mPortHandle != INVALID_HANDLE_VALUE)
		CloseHandle(mPortHandle);
	if(mRXevent != INVALID_HANDLE_VALUE)
		CloseHandle(mRXevent);
	if(mTXevent != INVALID_HANDLE_VALUE)
		CloseHandle(mTXevent);
	if(mDataEvent != INVALID_HANDLE_VALUE)
		CloseHandle(mDataEvent);
}

PGErr
CPFTSerial::Connect(ContactEntry *con, short *connectResult)
{
	static char	s[256], t[256], number[64];
	short	reply = _cr_NoReply, x;
	short	result=0;
	char	*p;
	
	mTMutex->Wait();
	SetState(_cs_connecting);
	*connectResult = 0;

	if(con->modemInit[0])
	{
		for (short retries = 3; reply != _cr_OK && retries; retries--)
		{
			Write("%s\r", con->modemInit);
			reply = GetModemResult(1000*5);
		}
		if(reply != _cr_OK)
		{
			*connectResult = reply;
			result = _pge_ModemNotAvail;
			goto done;
		}
	}
	if(con->useDialInfo)
	{
		x=0;
		strcpy(t,con->phoneNumber);
		for (p = t; *p && (x < 63); p++)
			if((*p != ' ') && (*p != '-') && (*p != '(') && (*p != ')'))
				number[x++] = *p;
		number[x] = 0;
		if(number[0] != gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].areaCode[1] ||
			number[1] != gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].areaCode[2] ||
			number[2] != gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].areaCode[3])
			x = 2;
		else if(con->nonLocal)
			x = 1;
		else
			x = 0;
		strcpy(s, gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].catBefore[x]);
		strcat(s, number);
		strcat(s, gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].catAfter[x]);
	}
	else
		strcpy(s, con->phoneNumber);
	CStatusPane::GetStatusPane()->AddStatus(0, "Contacting %s", s);
	strcpy(t, "ATD ");
	t[3] = gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].usePulseDial ? 'P' : 'T';
	strcat(t, s);
	Write("%s\r", t);
	reply = GetModemResult(1000*80);
	if(mAbort)
		Write("\r");
	*connectResult = reply;
	if(reply != _cr_Connect)
	{
		if(gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].redialDelay)
			Sleep(gPGFOpts.dopt.dialInfo[gPGFOpts.dopt.curDialSet].redialDelay);
		CStatusPane::GetStatusPane()->AddStatus(0, "Connection failed.");
		SetState(_cs_none);
	}
	else
	{
		CStatusPane::GetStatusPane()->AddStatus(0, "Connected %s.", mConnectString);
		SetState(_cs_connected);
	}
done:
	mTMutex->Signal();
	return result;
}

#define CheckModemResponse(strP, num)	\
		if(!strP ## Str || ch!=*strP ## Str) strP ## Str=NULL;	\
		else if(!*++strP ## Str) response=num;

short
CPFTSerial::GetModemResult(long timeout)
{
	char	*okStr = NULL, *noCarrierStr = NULL, *noAnswerStr = NULL, *busyStr = NULL;
	char	*noDialtoneStr = NULL, *connectStr = NULL, *errorStr = NULL;
	uchar	*p, ch, count, i;
	ulong	outOfTime;
	short	response = _cr_Timeout, state = 0, csLen;
	OVERLAPPED	overlapped;
	short handle_error_count = 0;
	HANDLE		handleArray[2] = {mAbortEvent, mDataEvent};
	DWORD		eventMask, result, timeToWait, error;

	memset(&overlapped, 0, sizeof(overlapped));

	ResetEvent(handleArray[1]);
	overlapped.hEvent = handleArray[1];
	
	outOfTime = pgp_getticks() + timeout;
	while ((response == _cr_Timeout || state == 1) && !mAbort &&
			(!timeout || pgp_getticks() < outOfTime))
	{
		// try and read some data from the modem
		if(Read(NIL, 0, NIL) > 0)
		{
			if(mInFront > mInBack)
				count = mInFront - mInBack;
			else
				count = -mInBack;
			p = mInBuffer + mInBack;

			if(!state)
			{
				for (i = 0; i < count && response == _cr_Timeout; i++, p++)
				{
					ch = (*p & 0x7F);
					if((ch == '\r') || (ch == '\n'))
					{
						okStr = "OK";
						noCarrierStr = "NO CARRIER";
						noAnswerStr = "NO ANSWER";
						busyStr = "BUSY";
						noDialtoneStr = "NO DIAL";
						connectStr = "CONNECT";
						errorStr = "ERROR";
					}
					else
					{
						CheckModemResponse(connect, 	_cr_Connect);
						CheckModemResponse(ok, 			_cr_OK);
						CheckModemResponse(noCarrier, 	_cr_NoCarrier);
						CheckModemResponse(noDialtone, 	_cr_NoDialTone);
						CheckModemResponse(noAnswer, 	_cr_NoAnswer);
						CheckModemResponse(busy, 		_cr_Busy);
						CheckModemResponse(error, 		_cr_Error);
					}
				}
				if(response == _cr_Connect)
				{
					state++;
					csLen = 0;
					goto getConSuffix;
				}
			}
			else
			{
				i = 0;
			getConSuffix:
				for(;i<count && *p != '\r';i++,p++)
					if(csLen<CONNECTSTRINGLEN-1)
						mConnectString[csLen++]=*p;
				if((*p=='\r') || (csLen==CONNECTSTRINGLEN-1))
				{
					mConnectString[csLen]=0;
					state++;
				}
			}
			mInBack += i;
		}
		else
		{
			// if we don't have any data in our buffer then we should setup to
			//	fall asleep until some comes in
			if(! WaitCommEvent(mPortHandle, &eventMask, &overlapped) &&
				(error = GetLastError()) != ERROR_IO_PENDING)
			{
				MODEM_ERROR("error setting up comm event", error);
				response = _pge_InternalAbort;
				break;
			}

			// make sure that some data didn't come in before WaitCommEvent()
			//	was called
			if(Read(NIL, 0, NIL) > 0)
			{
				// flush the pending wait and reset the event for another call
				SetCommMask(mPortHandle, EV_RXCHAR);
				ResetEvent(handleArray[1]);
				continue;
			}

			// figure out how long to wait.
			if(!timeout)
				timeToWait = INFINITE;
			else
				timeToWait = outOfTime - pgp_getticks();

WAIT_AGAIN:			// wait for the first event that gets triggered.
			result = WaitForMultipleObjects(2, handleArray, FALSE, timeToWait);
			if(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
				break;					// we have to abort or we timed out
			if(result != (WAIT_OBJECT_0+1))
			{
				int error = GetLastError();
				if(error ==  ERROR_INVALID_HANDLE && handle_error_count++ < 50)
					goto WAIT_AGAIN;
				MODEM_ERROR("unknown result waiting on event", error);
				response = _pge_InternalAbort;
				break;
			}
			ResetEvent(handleArray[1]);		// must have gotten data
		}
	}

	// flush any pending calls to WaitCommMask
	SetCommMask(mPortHandle, EV_RXCHAR);
	return response;
}

short
CPFTSerial::Disconnect()
{
	DCB		dcb;
	
	mTMutex->Wait();
	SetState(_cs_disconnecting);

	GetCommState(mPortHandle, &dcb);

	// drop DTR...
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	SetCommState(mPortHandle, &dcb);
	Sleep(250);

	// ...and pick it up again
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	SetCommState(mPortHandle, &dcb);
	Sleep(250);

	mPFWindow->GetStatusPane()->AddStatus(0, "Disconnected.");
	SetState(_cs_none);
	mTMutex->Signal();

	return 0;
}

short
CPFTSerial::WaitForRing()
{
	char	*ringStr = NULL;
	uchar	*p, ch, count, i;
	short	response = _cr_Timeout;
	OVERLAPPED	overlapped;
	int			handle_error_count = 0;

	HANDLE		handleArray[3] = {mAbortEvent, mAnswerEvent, mDataEvent};
	DWORD		eventMask, result, timeout, error;
	long		now;

	memset(&overlapped, 0, sizeof(overlapped));
	ResetEvent(handleArray[2]);
	overlapped.hEvent = handleArray[2];
	
	while (response == _cr_Timeout && !mAbort && !mAnswer)
	{
		// try to read some data
		if(Read(NIL, 0, NIL) > 0)
		{
			if(mInFront > mInBack)
				count = mInFront - mInBack;
			else
				count = -mInBack;
			p = mInBuffer + mInBack;

			for (i = 0; i < count && response == _cr_Timeout; i++)
			{
				ch = (*p & 0x7F);
				if(ch == '\r' || ch == '\n')
					ringStr = "RING";
				else
					CheckModemResponse(ring, _cr_Ring);
				p++;
			}
			mInBack += i;
		}
		else
		{
			// if we don't have any data in our buffer then we should setup to
			//	fall asleep until some comes in
			if(! WaitCommEvent(mPortHandle, &eventMask, &overlapped) &&
				(error = GetLastError()) != ERROR_IO_PENDING)
			{
				MODEM_ERROR("error setting up comm event", error);
				response = _pge_InternalAbort;
				break;
			}

			// make sure that some data didn't come in before WaitCommEvent()
			//	was called
			if(Read(NIL, 0, NIL) > 0)
			{
				// flush the pending wait and continue
				SetCommMask(mPortHandle, EV_RXCHAR);
				ResetEvent(handleArray[2]);
				continue;
			}

			// figure out how long to wait.  if we are waiting for a ring,
			//	then only fall asleep until the latest time the next ring
			//	could come in.
			if(mState == _cs_calldetected)
			{
				if(mLastRingTime + 8*1000 < (now = pgp_getticks()))
				{
					SetState(_cs_listening);
					continue;
				}
				timeout = mLastRingTime + 8*1000 - now;
			}
			else
				timeout = INFINITE;

WAIT_AGAIN:			// wait for the first event that gets triggered.
			result = WaitForMultipleObjects(3, handleArray, FALSE, timeout);
			if(result == WAIT_TIMEOUT)
			{
				// resume listening and flush the pending wait
				SetState(_cs_listening);
				SetCommMask(mPortHandle, EV_RXCHAR);
			}
			else if(result == WAIT_OBJECT_0)
				break;						// we have to abort
			else if(result == (WAIT_OBJECT_0+1))
			{
				//ResetEvent(mAnswerEvent);
				break;						// the phone has been answered
			}
			else if(result != (WAIT_OBJECT_0+2))
			{
				int error = GetLastError();
				if(error ==  ERROR_INVALID_HANDLE && handle_error_count++ < 50)
					goto WAIT_AGAIN;

				MODEM_ERROR("unknown error waiting on event", error);
				response = _pge_InternalAbort;
				break;
			}
			ResetEvent(handleArray[2]);		// must have gotten data
		}
	}

	// flush any pending calls to WaitCommEvent
	SetCommMask(mPortHandle, EV_RXCHAR);
	return response;
}

PGErr
CPFTSerial::Listen(Boolean answer)
{
	short	result = 0, reply = _cr_NoReply;

	mTMutex->Wait();
	if(answer)
		mAnswer = TRUE;
	else
		SetState(_cs_listening);
	while (!mAbort)
	{
		if((mAnswer || (WaitForRing() == _cr_Ring)) && !mAbort)
		{
			if(mAnswer)
			{
				SetState(_cs_connecting);
				Write("ATA\r");		/* answer the call, universal AT command */
				reply = GetModemResult(1000*60);
				if(reply == _cr_Connect)
				{
					SetState(_cs_connected);
					break;
				}
				else
					Write("\r");	/* abort the modem connecting */
			}
			else
			{
				if(mState != _cs_calldetected)
					SetState(_cs_calldetected);
				mLastRingTime = pgp_getticks();
				mPFWindow->GetStatusPane()->AddStatus(0, "...Ring...");
				if(gPGFOpts.popt.playRing)
					PlaySound("RING.WAV",NULL,SND_FILENAME+SND_SYNC);
			}
		}
		Sleep(0);
	}
	if(!mAnswer && mAbort)
		result = _pge_InternalAbort;
	mAnswer = FALSE;
	mTMutex->Signal();
	return result;
}

short
CPFTSerial::Reset()
{
	short	result=0;
	short	reply, inx;
	DCB		dcb;

	mTMutex->Wait();
	mAbort = FALSE;
	ResetEvent(mAbortEvent);

	// get the current settings for the port
	if(! GetCommState(mPortHandle, &dcb))
	{
		MODEM_ERROR("error getting modem settings", GetLastError());
		mTMutex->Signal();
		return -1;
	}

	// set the baud rate and other hardware parameters
	dcb.BaudRate        = mSerOpts.baud;
	dcb.fBinary         = 1;
	dcb.fParity         = 0;
	dcb.fOutxCtsFlow    = 1;
	dcb.fOutxDsrFlow    = 0;
	dcb.fDtrControl     = DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity = 0;
	dcb.fOutX           = 0;
	dcb.fInX            = 0;
	dcb.fErrorChar      = 0;
	dcb.fNull           = 0;
	dcb.fRtsControl     = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError   = 0;
	dcb.ByteSize        = 8;
	dcb.Parity          = NOPARITY;
	dcb.StopBits        = ONESTOPBIT;
	if(! SetCommState(mPortHandle, &dcb))
	{
		MODEM_ERROR("error setting modem settings", GetLastError());
		mTMutex->Signal();
		return -2;
	}

	SetState(_cs_initializing);
	Sleep(1000);	// sleep for a second to let the modem catch-up

	// setup a mask so we know when a character comes in
	if(! SetCommMask(mPortHandle, EV_RXCHAR))
	{
		MODEM_ERROR("error setting up comm mask", GetLastError());
		return -3;
	}

	mTMutex->Signal();
	reply = _cr_NoReply;
	for(inx = 0; inx < 3 && reply != _cr_OK && !mAbort; inx++)
	{
		if(Write("AT%s\r", gPGFOpts.sopt.modemInit) < 0)
		{
			MODEM_ERROR("error writing to the modem", GetLastError());
			return -4;
		}
		reply = GetModemResult(1000*5);
	}
	if(reply != _cr_OK)
		result = _pge_ModemNotAvail;
	if(mAbort)
		result = _pge_InternalAbort;

	return result;
}

long
CPFTSerial::Read(void *data, long len, short *channel)
{
	static OVERLAPPED	overlapped;
	ulong	read = 0;
	long	extra = 0;
	uchar	*buffer = (uchar*)data, l = 0, amount;
	DWORD	error;

	// setup an overlapped structure for reading
	overlapped.Offset = overlapped.OffsetHigh = 0;
	overlapped.hEvent = mRXevent;
	ResetEvent(mRXevent);

	// check to see if the caller just wants to know whether we have
	//  any data waiting.
	if(buffer && len)
	{
		// in an earlier check for waiting data, we may have read an
		//  extra character.  if so, then include it in the data we
		//  send back.
		if(mInFront != mInBack)
		{
			// check to see if we have enough data in our internal buffer
			if(len > (uchar)(mInFront - mInBack))
			{
				if(mInFront > mInBack)
				{
					pgp_memcpy(buffer, mInBuffer + mInBack, l = mInFront - mInBack);
					buffer += l;
					len    -= l;
				}
				else
				{
					pgp_memcpy(buffer, mInBuffer + mInBack, l = (uchar)(-mInBack));
					buffer += l;
					len    -= l;
					pgp_memcpy(buffer, mInBuffer, mInFront);
					buffer += mInFront;
					len    -= mInFront;
					l      += mInFront;
				}
				mInBack = mInFront = 0;
			}
			else
			{
				if((uchar)(-mInBack) > len)
				{
					pgp_memcpy(buffer, mInBuffer + mInBack, len);
					mInBack += (uchar)len;
					return len;
				}
				else
				{
					pgp_memcpy(buffer, mInBuffer + mInBack, l = (uchar)(-mInBack));
					mInBack += (uchar)len;
					buffer += l;
					len    -= l;
					pgp_memcpy(buffer, mInBuffer, len);
					return len + l;
				}
			}
		}

		// try and read the data from the modem.
		if(! ReadFile(mPortHandle, buffer, len, &read, &overlapped) &&
			(error = GetLastError()) != ERROR_IO_PENDING)
		{
			MODEM_ERROR("error reading from modem", error);
			return -2;
		}

		// wait for the read to complete
		GetOverlappedResult(mPortHandle, &overlapped, &read, TRUE);

		return l + read;
	}
	else if(buffer)
		// if they don't want any data we succeed automatically.
		return 0;
	else
	{
		// if we have any data in our internal buffer from an earlier read
		//  then return the amount of data in our internal buffer
		//  immediately.  otherwise, try to read some data into our
		//  internal buffer and return the number of bytes that we
		//  succeeded in reading.

		if(mInFront != mInBack)
			return (uchar)(mInFront - mInBack);

		mInFront = mInBack = 0;
		amount = 255;

		if(! ReadFile(mPortHandle, mInBuffer, amount, &read, &overlapped) &&
			(error = GetLastError()) != ERROR_IO_PENDING)
		{
			MODEM_ERROR("error reading from modem", error);
			return -1;
		}

		// wait for the read to complete.
		if(! GetOverlappedResult(mPortHandle, &overlapped, &read, TRUE))
		{
			MODEM_ERROR("error getting read result", GetLastError());
			return -2;
		}

		mInFront = (uchar)read;
		return read;
	}
}

short
CPFTSerial::WriteBlock(void *buffer, long *count, short channel)
{
	static OVERLAPPED	overlapped;
	ulong	written;
	DWORD	error;

	mTMutex->Wait();

	// setup the OVERLAPPED structure for writing
	overlapped.Offset = overlapped.OffsetHigh = 0;
	overlapped.hEvent = mTXevent;

	// we use an overlapped structure so that a read can occur at the same
	//  time as a write.
	if(! WriteFile(mPortHandle, buffer, *count, &written, &overlapped) &&
		(error = GetLastError()) != ERROR_IO_PENDING)
	{
		MODEM_ERROR("error writing to modem", error);
		mTMutex->Signal();
		return -1;
	}

	// wait for the write to complete
	if(! GetOverlappedResult(mPortHandle, &overlapped, &written, TRUE))
	{
		MODEM_ERROR("error getting write result", GetLastError());
		mTMutex->Signal();
		return -2;
	}

	*count = written;
	mTMutex->Signal();

	return 0;
}

/*void
CPFTSerial::WaitAsync(AsyncTransport *asyncTrans)
{
	return;

	WaitForSingleObject(asyncTrans->u.overlapped.hEvent, INFINITE);
	return;
}*/

PGErr
CPFTSerial::WriteAsync(long count, short channel, AsyncTransport *async)
{
	return WriteBlock(async->buffer, &count, _pfc_Control);

	BOOL	result;
	DWORD	error;

	async->u.overlapped.Internal = 0;
	async->u.overlapped.InternalHigh = 0;
	async->u.overlapped.Offset = 0;
	async->u.overlapped.OffsetHigh = 0;

	result = WriteFile(mPortHandle, async->buffer, count, &async->written,
		&async->u.overlapped);
	if(!result && (error = GetLastError()) != ERROR_IO_PENDING)
	{
		MODEM_ERROR("error writing to modem", error);
		return _pge_ModemNotAvail;
	}

	return _pge_NoErr;
}

short
CPFTSerial::SleepForData(HANDLE *abortEvent)
{
	OVERLAPPED	overlapped;
	DWORD		eventMask, result, error = 0;
	short retval = 0;
	short handle_error_count = 0;

	HANDLE		handleArray[2] = {*abortEvent, mDataEvent};
	memset (&overlapped,0, sizeof(OVERLAPPED));
	overlapped.hEvent = handleArray[1];

	if(mInFront == mInBack && Read(NIL, 0, NIL) == 0)
	{
		if(! WaitCommEvent(mPortHandle, &eventMask, &overlapped) &&
			(error = GetLastError()) != ERROR_IO_PENDING)
		{
			MODEM_ERROR("error setting up comm event", error);
			retval = -1;
			//goto EXIT_FUNC;
		}

		// make sure that some data didn't come in before WaitCommEvent() was
		//	called
		if(Read(NIL, 0, NIL) > 0)
		{
			// flush the pending wait and return
			SetCommMask(mPortHandle, EV_RXCHAR);
			retval = 0;
			goto CLOSE_EVENT;
		}
		// wait for the first event that gets triggered.
WAIT_AGAIN:	 // Windows 95 returns ERROR_INVALID_HANDLE	sometimes when there is nothing 
			// wrong with the handle. When that happens we ignor the error a few times and see
			// if things don't just work fine anyway.
		result = WaitForMultipleObjects(2, handleArray, FALSE, INFINITE);
		if(result == WAIT_OBJECT_0)
		{
			// flush the pending wait and return
			SetCommMask(mPortHandle, EV_RXCHAR);
			error = 1;
		}
		else if(result != (WAIT_OBJECT_0+1))
		{
			// flush the pending wait, report an error, and return
			DWORD error = GetLastError();
			if(error ==  ERROR_INVALID_HANDLE && handle_error_count++ < 50)
				goto WAIT_AGAIN;

			SetCommMask(mPortHandle, EV_RXCHAR);
			MODEM_ERROR("unknown result waiting on event", error);
			retval = -2;
		}
	}
CLOSE_EVENT:;
	return retval;								// there's data so we're done
}

