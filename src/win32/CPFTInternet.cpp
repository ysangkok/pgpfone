/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFTInternet.cpp,v 1.4 1999/03/10 02:36:16 heller Exp $
____________________________________________________________________________*/
#include "CPFTInternet.h"

#include "CPFWindow.h"
#include "CControlThread.h"
#include "CPFPackets.h"
#include "CStatusPane.h"
#include "CPGPFone.h"
#include "LThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>

extern BOOLEAN gHasWinsock;

CPFTInternet::CPFTInternet(CPFWindow *cpfWindow, LThread *thread, short *result)
					:	CPFTransport(cpfWindow)
{
	*result = 0;
	mSocket = mMSocket = mMCSocket = INVALID_SOCKET;
	mPFWindow = cpfWindow;
	mPFWindow->SetLocalAddress("");
	if(!gHasWinsock)
		*result = _pge_PortNotAvail;
	else
	{
		mSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(mSocket != INVALID_SOCKET)
		{
			in_addr iaLocal;
			char szName[256];

			mLastAddress.sin_addr.s_addr = mRemoteAddress.sin_addr.s_addr = 0;
			mThread = thread;
			iaLocal.s_addr = INADDR_ANY;  // Initialize local addr to 0
			if(gethostname(szName, 256) != SOCKET_ERROR)
			{
				hostent* pHE = gethostbyname(szName);

				if(pHE != NULL)
				{
					// local ip address found
					iaLocal.s_addr = ((in_addr*)(pHE->h_addr_list[0]))->s_addr;
					mPFWindow->SetLocalAddress(inet_ntoa(iaLocal));
				}
			}
		}
		else
			*result = _pge_PortNotAvail;
	}
	if(*result)
		PGFAlert("Internet services could not be opened!", 0);
}

CPFTInternet::~CPFTInternet()
{
	AbortSync();
	mTMutex->Wait(semaphore_WaitForever);
	if(mState == _cs_connected)
		Disconnect();
	if(mSocket != INVALID_SOCKET)
		closesocket(mSocket);
	if(mMSocket != INVALID_SOCKET)
		closesocket(mMSocket);
	if(mMCSocket != INVALID_SOCKET)
		closesocket(mMCSocket);
}

PGErr
CPFTInternet::Connect(ContactEntry *con, short *connectResult)
{
	char buf[MAXPGPFUDPSIZE];
	Boolean found = FALSE;
	long len;
	uchar *p;
	short channel;
	ulong nextTry;
	char remoteDotName[32];
	struct hostent *ihinfo;
	struct in_addr inadr;
	int err;
	
	mTMutex->Wait(semaphore_WaitForever);
	SetState(_cs_connecting);
	*connectResult = _cr_Error;
	if(con->internetAddress[0])
	{
		CStatusPane::GetStatusPane()->AddStatus(0, "Contacting...");
		memset(&mAddress, 0, sizeof(mAddress));
		mAddress.sin_family = PF_INET;
		mAddress.sin_port = htons(CONTROLPORTNUMBER);
		err = bind(mSocket, (struct sockaddr *)&mAddress, sizeof(mAddress));
		if(err)
		{
			err = WSAGetLastError();
			pgpAssertNoErr(err);
		}
		if(!err && !mAbort)
		{
			memset(&mRemoteAddress, 0, sizeof(mRemoteAddress));
			mRemoteAddress.sin_family = PF_INET;
			mRemoteAddress.sin_port = htons(CONTROLPORTNUMBER);
			if(con->internetAddress[0]<'0' || con->internetAddress[0]>'9')
			{
				//Resolve address by name
				if(ihinfo = gethostbyname(con->internetAddress))
				{
					pgp_memcpy(&mRemoteAddress.sin_addr.s_addr, *ihinfo->h_addr_list, 4);
					pgp_memcpy(&inadr, *ihinfo->h_addr_list, 4);
					strcpy(remoteDotName, inet_ntoa(inadr));
					CStatusPane::GetStatusPane()->AddStatus(0, "Host identified as %s...",
						remoteDotName);
				}
				else
				{
					CStatusPane::GetStatusPane()->AddStatus(0, "Name lookup failed.");
					err = -1;
				}
			}
			else
			{
				//Convert numeric address to network order long
				if((mRemoteAddress.sin_addr.s_addr = inet_addr(con->internetAddress))
					== INADDR_NONE)
					err = -1;
			}
			if(!err && !mAbort)
			{
				for(short tries=MAXUDPCALLTRIES;(*connectResult == _cr_Error) && tries>0 && !mAbort;
					tries--)
				{
					buf[0] = _hpt_Setup;
					buf[1] = _ptip_Call;	len = 2;
					if(gPGFOpts.popt.idUnencrypted && gPGFOpts.popt.idOutgoing &&
						gPGFOpts.popt.identity[0])
					{
						len+= (buf[2] = strlen(gPGFOpts.popt.identity)) + 1;
						strcpy(&buf[3], gPGFOpts.popt.identity);
					}
					WriteBlock(buf, &len, _pfc_Control);
					nextTry = pgp_getticks() + 3000;
					while(!mAbort && nextTry > pgp_getticks())
					{
						if(len = Read(buf, MAXPGPFUDPSIZE, &channel))
							if(mLastAddress.sin_addr.s_addr ==
									mRemoteAddress.sin_addr.s_addr)
							{
								p = (uchar *)buf;
								if(*p++ == _hpt_Setup)
									switch(*p++)
									{
										case _ptip_Accept:
											*connectResult = _cr_Connect;
											break;
										case _ptip_Busy:
											CStatusPane::GetStatusPane()->AddStatus(0,
													"Busy.");
											if(gPGFOpts.popt.playRing)
												PlaySound("BUSY.WAV",NULL,
													SND_FILENAME+SND_SYNC);
											*connectResult = _cr_Busy;
											break;
										case _ptip_ProbeResponse:
											if(!found)
											{
												found = TRUE;
												CStatusPane::GetStatusPane()->AddStatus(0,
													"Remote contacted, ringing.");
												if(gPGFOpts.popt.playRing)
													PlaySound("BRING.WAV",NULL,
														SND_FILENAME+SND_SYNC);
											}
											break;
										case _ptip_Message:
											ReceiveUDPMsg(p, len -2);
											break;
									}
									break;
							}
						::Sleep(0);
					}
				}
			}
			if(*connectResult != _cr_Connect)
			{
				//WinSock 1.1 apparently didn't bother to include an unbind
				//call, so we have to hack it by killing the socket and recreating.
				closesocket(mSocket);
				mSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			}
		}
	}
	else
	{
		PGFAlert("No Internet address has been specified.",0);
		err = _pge_NoSrvrName;
	}
	if(*connectResult != _cr_Connect)
	{
		CStatusPane::GetStatusPane()->AddStatus(0, "Connection failed.");
		SetState(_cs_none);
	}
	else
	{
		CStatusPane::GetStatusPane()->AddStatus(0, "Connected.");
		SetState(_cs_connected);
	}
	mTMutex->Signal();
	return err;
}

PGErr
CPFTInternet::Disconnect()
{
	mTMutex->Wait(semaphore_WaitForever);
	if(mState == _cs_connected)
	{
		SetState(_cs_disconnecting);
		//WinSock 1.1 apparently didn't bother to include an unbind
		//call, so we have to hack it by killing the socket and recreating.
		closesocket(mSocket);
		if(mMSocket != INVALID_SOCKET)
			closesocket(mMSocket);
		if(mMCSocket != INVALID_SOCKET)
			closesocket(mMCSocket);
		mMSocket = mMCSocket = INVALID_SOCKET;
		mSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	SetState(_cs_none);
	mTMutex->Signal();
	return 0;
}

PGErr
CPFTInternet::Listen(Boolean answer)
{
	short result=0, reply=_cr_NoReply, rings, slen, err, channel;
	long resplen;
	ulong nextRingTime, len;
	uchar *p, buf[MAXPGPFUDPSIZE];
	char remoteDotName[32], remoteName[64], responsepkt[128];
	struct in_addr inadr;
	struct sockaddr_in savedAddress;
	Boolean sendResp, goodaddr, noted;

	mTMutex->Wait(semaphore_WaitForever);
	memset(&mAddress, 0, sizeof(mAddress));
	mAddress.sin_family = PF_INET;
	mAddress.sin_port = htons(CONTROLPORTNUMBER);
	err = bind(mSocket, (struct sockaddr FAR *) &mAddress, sizeof(mAddress));
	if(err)
	{
		err = WSAGetLastError();
		pgpAssertNoErr(err);
	}
	if(!err && !mAbort)
	{
		SetState(_cs_listening);
		do
		{
			if(mAnswer)
			{	// accept
				responsepkt[0] = _hpt_Setup;
				responsepkt[1] = _ptip_Accept;	resplen = 2;
				WriteBlockTo(responsepkt, &resplen, &mLastAddress);
				pgp_memcpy(&mRemoteAddress, &mLastAddress, sizeof(mLastAddress));
				CStatusPane::GetStatusPane()->AddStatus(0, "Connected.");
				SetState(_cs_connected);
				break;
			}
			if((mState == _cs_calldetected) && (nextRingTime <= pgp_getticks()))
			{	// ring every 5.5 seconds
				if(rings++ >= MAXUDPRINGS)
					SetState(_cs_listening);
				else
				{
					if(!noted)
					{
						CStatusPane::GetStatusPane()->AddStatus(0,
							"Incoming call from %s(%s)...",
							remoteName, remoteDotName);
						noted = TRUE;
					}
					if(gPGFOpts.popt.playRing)
						PlaySound("RING.WAV",NULL,SND_FILENAME+SND_SYNC);
					nextRingTime = pgp_getticks() + 5500;
				}
			}
			sendResp = FALSE;
			goodaddr = FALSE;
			pgp_memcpy(&savedAddress, &mLastAddress, sizeof(mLastAddress));
			if(len = Read(buf, MAXPGPFUDPSIZE, &channel))
			{
				p=buf;
				if(*p++==_hpt_Setup)
					switch(*p++)
					{
						case _ptip_Call:
							if(mState != _cs_calldetected)
							{
								goodaddr = TRUE;
								remoteName[0]=0;
								if(len > 2)
								{
									if((slen = *p++) && (len-3 >= (ulong)slen))
									{
										if(slen > 63)
											slen = 63;
										pgp_memcpy(remoteName, p, slen);
										remoteName[slen]=0;
										mPFWindow->GetControlThread()->SetRemoteName(remoteName);
									}
								}
								SetState(_cs_calldetected);
								inadr.s_addr = mLastAddress.sin_addr.s_addr;
								strcpy(remoteDotName, inet_ntoa(inadr));
								nextRingTime = pgp_getticks();
								rings = 0;
								sendResp = TRUE;
								noted = FALSE;
							}
							break;
						case _ptip_Probe:
							sendResp = TRUE;
							break;
						case _ptip_Message:
							ReceiveUDPMsg(p, len -2);
							break;
					}
			}
			if(sendResp)
			{
				responsepkt[0] = _hpt_Setup;
				responsepkt[1] = _ptip_ProbeResponse;	resplen = 2;
				if(gPGFOpts.popt.idUnencrypted && gPGFOpts.popt.idOutgoing &&
					gPGFOpts.popt.identity[0])
				{
					resplen+= (responsepkt[2] = strlen(gPGFOpts.popt.identity)) + 1;
					strcpy(&responsepkt[3], gPGFOpts.popt.identity);
				}
				WriteBlockTo(responsepkt, &resplen, &mLastAddress);
				sendResp = FALSE;
			}
			if(!goodaddr)
				pgp_memcpy(&mLastAddress, &savedAddress, sizeof(savedAddress));
			::Sleep(0);
		} while(!mAbort);
		if(mState != _cs_connected)
		{
			//WinSock 1.1 apparently didn't bother to include an unbind
			//call, so we have to hack it by killing the socket and recreating.
			closesocket(mSocket);
			mSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		}
	}
	if(!mAnswer && mAbort)
		result = _pge_InternalAbort;
	mAnswer = FALSE;
	mTMutex->Signal();
	return result;
}

PGErr
CPFTInternet::Reset()
{
	mAbort = FALSE;
	return 0;
}

long
CPFTInternet::Read(void *data, long max, short *channel)
{
	long len = 0, rdlen;
	fd_set rfds;
	struct timeval tv;
	int fromlen, err, rcvd=0;	
	char rdpkt[128];
	SOCKET rcvSock;

	// currently we poll WinSock using select.  Alternatives
	// to this include using the massive async hack from
	// WinSock 1.1 or the message system which will be
	// available in WinSock 2.  The best elegant method seems
	// to be to detect WinSock2 and use it if available,
	// otherwise use this 1.1 polling stuff.

	// return one packet if available
	if(mMSocket != INVALID_SOCKET)
	{
		memset(&tv, 0, sizeof(struct timeval));
		FD_ZERO(&rfds);
		FD_SET(mMSocket, &rfds);
		err = select(1, &rfds, NIL, NIL, &tv);
		if(err > 0)
		{
			rcvSock = mMSocket;
			*channel = _pfc_Media;
			rcvd=1;
		}
	}
	if(!rcvd && mSocket != INVALID_SOCKET)
	{
		memset(&tv, 0, sizeof(struct timeval));
		FD_ZERO(&rfds);
		FD_SET(mSocket, &rfds);
		err = select(1, &rfds, NIL, NIL, &tv);
		if(err > 0)
		{
			rcvSock = mSocket;
			*channel = _pfc_Control;
			rcvd=1;
		}
	}
	if(!rcvd && mMCSocket != INVALID_SOCKET)
	{
		memset(&tv, 0, sizeof(struct timeval));
		FD_ZERO(&rfds);
		FD_SET(mMCSocket, &rfds);
		err = select(1, &rfds, NIL, NIL, &tv);
		if(err > 0)
		{
			rcvSock = mMCSocket;
			*channel = _pfc_MediaControl;
			rcvd=1;
		}
	}
	if(data && max && rcvd)
	{
		fromlen = sizeof(mLastAddress);
		if((len=recvfrom(rcvSock, (char *)data, max, 0,
				(struct sockaddr *)&mLastAddress, &fromlen))>0)
		{
			if(len && ((mState == _cs_connected) &&
				(mLastAddress.sin_addr.s_addr != mRemoteAddress.sin_addr.s_addr)))
			{
				uchar *p = (uchar *)data;
		
				if(*channel == _pfc_Control)
				{
					if(*p++==_hpt_Setup)
						switch(*p++)
						{
							case _ptip_Call:
								// We are already connected.  Send back a busy notice.
								rdpkt[0] = _hpt_Setup;
								rdpkt[1] = _ptip_Busy;	rdlen = 2;
								WriteBlockTo(rdpkt, &rdlen, &mLastAddress);
								break;
							case _ptip_Probe:
								// We are already connected, but we don't report that to a probe.
								// The idea is that we only reveal our status if the other
								// party is also revealing its information.
								rdpkt[0] = _hpt_Setup;
								rdpkt[1] = _ptip_ProbeResponse;	rdlen = 2;
								if(gPGFOpts.popt.idUnencrypted && gPGFOpts.popt.idOutgoing &&
									gPGFOpts.popt.identity[0])
								{
									rdlen+= (rdpkt[2] = strlen(gPGFOpts.popt.identity)) + 1;
									strcpy(&rdpkt[3], gPGFOpts.popt.identity);
								}
								WriteBlockTo(rdpkt, &rdlen, &mLastAddress);
								break;
							case _ptip_Message:
								ReceiveUDPMsg(p, len -2);
								break;
						}
				}
				len = 0;
			}
		}
		else
			len = 0;
	}
	else
	{
		if((mState != _cs_connected) && (mState != _cs_connecting))
			Sleep(100);
	}
	if(err < 0)
	{
		err = WSAGetLastError();
		//pgp_errif(err);
	}
	return len;
}

void
CPFTInternet::ReceiveUDPMsg(uchar *msg, long len)
{
	//not implemented yet
}

PGErr
CPFTInternet::WriteBlockTo(void *buffer, long *count, struct sockaddr_in *addr)
{
	mTMutex->Wait(semaphore_WaitForever);
	// Send a datagram
	if(SOCKET_ERROR == sendto(mSocket, (char *)buffer, *count, 0,
		(struct sockaddr *)addr, sizeof(struct sockaddr_in)))
	{
		int err = WSAGetLastError();
		pgpAssertNoErr(err);
	}
	mTMutex->Signal();
	return 0;
}

PGErr
CPFTInternet::WriteBlock(void *buffer, long *count, short channel)
{
	return WriteBlockTo(buffer, count, &mRemoteAddress);
}

PGErr
CPFTInternet::WriteAsync(long count, short channel, AsyncTransport *async)
{
	int err;

	mTMutex->Wait(semaphore_WaitForever);
	// ###########################################
	// NEXT LINE OVERRIDES MULTIPLE UDP PORT USAGE
	// The support works fine for multiple ports, but 
	// since we chose not to use RTP, it seems best
	// to disable it for now to not clutter the ports
	// with packets that could just as easily be sent
	// on one port.
	channel = _pfc_Control;
	// Send a datagram
	switch(channel)
	{
		case _pfc_Control:
			err = sendto(mSocket, (char *)async->buffer, count, 0,
				(struct sockaddr *)&mRemoteAddress, sizeof(mRemoteAddress));
			break;
		case _pfc_Media:
			err = sendto(mMSocket, (char *)async->buffer, count, 0,
				(struct sockaddr *)&mMRemoteAddress, sizeof(mMRemoteAddress));
			break;
		case _pfc_MediaControl:
			err = sendto(mMCSocket, (char *)async->buffer, count, 0,
				(struct sockaddr *)&mMCRemoteAddress, sizeof(mMCRemoteAddress));
			break;
	}
	if(SOCKET_ERROR == err)
	{
		int err = WSAGetLastError();
		pgpAssertNoErr(err);
	}
	mTMutex->Signal();
	return 0;
}

void
CPFTInternet::CleanUp()
{
}

PGErr
CPFTInternet::BindMediaStreams()
{
	struct sockaddr_in localAddr;
	int err;

	mMSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	mMCSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset(&mMRemoteAddress, 0, sizeof(mMRemoteAddress));
	mMRemoteAddress.sin_family = PF_INET;
	mMRemoteAddress.sin_port = htons(RTPPORTNUMBER);
	mMRemoteAddress.sin_addr.s_addr = mRemoteAddress.sin_addr.s_addr;
	memset(&mMCRemoteAddress, 0, sizeof(mMCRemoteAddress));
	mMCRemoteAddress.sin_family = PF_INET;
	mMCRemoteAddress.sin_port = htons(RTCPPORTNUMBER);
	mMCRemoteAddress.sin_addr.s_addr = mRemoteAddress.sin_addr.s_addr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = PF_INET;
	localAddr.sin_port = htons(RTPPORTNUMBER);
	err = bind(mMSocket, (struct sockaddr *)&localAddr, sizeof(localAddr));
	if(err)
	{
		err = WSAGetLastError();
		pgpAssertNoErr(err);
	}
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = PF_INET;
	localAddr.sin_port = htons(RTCPPORTNUMBER);
	err = bind(mMCSocket, (struct sockaddr *)&localAddr, sizeof(localAddr));
	if(err)
	{
		err = WSAGetLastError();
		pgpAssertNoErr(err);
	}
	return err;
}

short
CPFTInternet::SleepForData(HANDLE *abortEvent)
{
	DWORD	result;

	//This code is not actually used.
	result = WaitForSingleObject(*abortEvent, INFINITE);
	if (result != WAIT_OBJECT_0)
		pgp_errstring("unknown error waiting for abort event");
	return 1;
}

