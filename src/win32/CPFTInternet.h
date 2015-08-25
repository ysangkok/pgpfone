/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFTInternet.h,v 1.5 1999/03/10 02:36:18 heller Exp $
____________________________________________________________________________*/
#ifndef CPFTINTERNET_H
#define CPFTINTERNET_H

#include "CPFTransport.h"
#include <winsock.h>

#define CONTROLPORTNUMBER	17447	// PGPfone's conference control port
#define RTPPORTNUMBER		7448	// RTP's default port
#define RTCPPORTNUMBER		7449	// RTCP's default port
#define MAXPGPFUDPSIZE		600
#define MAXUDPRINGS			6		//note: this number and the next must
#define MAXUDPCALLTRIES		17		//be synchronized to avoid overringing.


class CPFWindow;
class LThread;
			    
class CPFTInternet		:	public CPFTransport
{
public:
					CPFTInternet();
					CPFTInternet(CPFWindow *cpfWindow, LThread *thread,
						short *result);
					~CPFTInternet();
	PGErr			Connect(ContactEntry *con, short *connectResult);
	PGErr			Disconnect();
	PGErr			Listen(Boolean answer);
	PGErr			Reset();
	PGErr			BindMediaStreams();
	PGErr			WriteBlock(void *buffer, long *count, short channel);
	PGErr			WriteBlockTo(void *buffer, long *count, struct sockaddr_in *addr);
	PGErr			WriteAsync(long count, short channel, AsyncTransport *async);
	long			Read(void *data, long max, short *channel);
	void			ReceiveUDPMsg(uchar *msg, long len);
	static void		CleanUp();
	short			SleepForData(HANDLE *abortEvent);
	
	LThread			*mThread;
private:
	SOCKET			mSocket, mMSocket, mMCSocket;
	struct sockaddr_in	mAddress, mRemoteAddress, mLastAddress,
						mMRemoteAddress, mMCRemoteAddress;
	CPFWindow		*mPFWindow;
};

#endif

