/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFTSerial.h,v 1.5 1999/03/10 02:36:21 heller Exp $
____________________________________________________________________________*/
#ifndef CPFTSERIAL_H
#define CPFTSERIAL_H

#include "CPFTransport.h"

#define MAXSERPORTS			16
#define CONNECTSTRINGLEN	64

typedef struct SerialPort
{
	uchar name[32], inName[32], outName[32];
	short inUse;
} SerialPort;

class CPFWindow;

class CPFTSerial : public CPFTransport
{
public:
					CPFTSerial();
					CPFTSerial(SerialOptions *serOpt, CPFWindow *cpfWindow, short *result);
	virtual			~CPFTSerial();
	PGErr			Connect(ContactEntry *con, short *connectResult);
	PGErr			Disconnect();
	PGErr			Listen(Boolean answer);
	PGErr			Reset();
	PGErr			WriteBlock(void *buffer, long *count, short channel);
	PGErr			WriteAsync(long count, short channel, AsyncTransport *async);
	long			Read(void *data, long len, short *channel);
	//void			WaitAsync(AsyncTransport *asyncTrans);
	short			GetModemResult(long timeout);
	short			WaitForRing();
	short			SleepForData(HANDLE *abortEvent);
private:
	SerialOptions	mSerOpts;
	HANDLE			mPortHandle;
	char			mPort[5];
	uchar			mInFront, mInBack;	// indices into mInBuffer
	uchar			mInBuffer[513];		// internal buffer for reading data
	HANDLE			mRXevent, mTXevent;	// event objects for reading and writing
	char			mConnectString[CONNECTSTRINGLEN];
	long			mLastRingTime;
	HANDLE			mDataEvent;
};

#endif


