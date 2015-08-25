/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CControlThread.cpp,v 1.4 1999/03/10 02:32:21 heller Exp $
____________________________________________________________________________*/
#include "CControlThread.h"
#include "CPFTSerial.h"
#include "CPFWindow.h"
#include "CStatusPane.h"
#include "CPFPackets.h"
#include "CMessageQueue.h"
#include "CSoundInput.h"
#include "CSoundOutput.h"
#include "CPacketWatch.h"
#include "CEncryptionStream.h"
#include "HashWordList.h"
#include "SHA.h"
#include "dh.h"
#include "bn.h"
#include "fastpool.h"
#include "CPFTInternet.h"
#ifdef	PGPXFER
#include "CXferWindow.h"
#include "CXferThread.h"
#endif
#ifdef PGP_MACINTOSH
#include "CPFTAppleTalk.h"
#endif

#include <stdio.h>
#include <string.h>


/* CONFIGCALL enables us to "turn off" configuration packets
	This can be helpful in testing for many reasons including
	trying to determine whether a problem lies in configuration
	or elsewhere	*/
#define CONFIGCALL
/* DIRECTCONNECT implements a debug mode where ControlThread
	assumes that a modem connection is already established just
	by creating a transport object and vice versa. */
#define noDIRECTCONNECT
#define noDEBUGCONFIG

const uchar pgpfMajorProtocolVersion = 0;
const uchar pgpfMinorProtocolVersion = 0;

CControlThread::CControlThread(CMessageQueue *msgQueue,
								CPFWindow *cpfWindow,
								CSoundInput *soundInput,
								CSoundOutput *soundOutput,
								void **outResult)
#ifndef PGP_WIN32
        : LThread(FALSE, thread_DefaultStack, threadOption_UsePool, outResult)
#else
        : LThread(outResult)
#endif  // PGP_WIN32
{
	mControlState = _con_None;
	mDone = FALSE;
	mMsgQueue = msgQueue;
	mPFWindow = cpfWindow;
	mSoundInput = soundInput;
	mSoundOutput = soundOutput;
	mStatusPane = CStatusPane::GetStatusPane();
	mXferWindow = NIL;
	mTransport = NIL;
	mInHPP = NIL;
	mOutHPP = NIL;
	mXfer = NIL;
	mXferQueue = NIL;
	mXferResult = mInHPPResult = mOutHPPResult = 1L;
	mSoundOutQ = mControlOutQ = NIL;
	mFullDuplex = FALSE;
	mAuthenticated = FALSE;
	mDHPublic = mDHPrivate = NIL;
	mCoders = NIL;
	mNumCoders = 0;
	mRemotePublicName[0]=0;
	SetSpeaker(TRUE);
	byteFifoInit(&mSharedSecret);
	byteFifoInit(&mOAuthentication);
	byteFifoInit(&mAAuthentication);
}

CControlThread::~CControlThread()
{
	if(mDHPublic)
		pgp_free(mDHPublic);
	if(mDHPrivate)
		pgp_free(mDHPrivate);
	if(mCoders)
		pgp_free(mCoders);
}

void *
CControlThread::Run(void)
{
	short done = 0, err;
	Boolean exit = FALSE;
	PFMessage *msg, *hACK;
	ContactEntry *contact;
	uchar *p, *e, alen[4];
	long codec, authwrit;
	short reply;
#ifdef DEBUGCONFIG
	uchar s[80];
#endif
	
	while(!exit)
	{
		err = 0;
		msg = mMsgQueue->Recv();
		if(!msg)
			continue;
		switch(msg->type)
		{
			case _mt_quit:
				exit = mDone = TRUE;
			case _mt_remoteAbort:
			case _mt_abort:
				mControlState = _con_Disconnecting;
				mPFWindow->ShowStatus();
				if(msg->type != _mt_quit)
				{
					mPFWindow->SetDecoder(0);
					mPFWindow->SetEncoder(0);
					mPFWindow->SetEncryption(0,0);
					mPFWindow->HideAuthParams();
					mPFWindow->ClearDecoderList();
					mPFWindow->ClearEncoderList();
					mPFWindow->SetKeyExchange("");
					mPFWindow->AllowXfers(FALSE);
				}
				mSoundInput->Record(FALSE);
				mSoundOutput->Play(FALSE);
				mAuthenticated = FALSE;
#ifdef	PGPXFER
				if(mXfer)
				{
					mInHPP->SetXferQueue(NIL);
					if(mXferWindow)
						mXferWindow->SetXferThread(NIL);
					mXferQueue->Send(_mt_abort);
					while(!mXferResult)
						(Yield)();
					delete mXferQueue;
					mXferQueue = NIL;
					mXfer = NIL;
				}
#endif
				if(mControlOutQ)
				{
					mSoundOutQ->FreeType(_mt_voicePacket);
					mTransport->Flush();
					if((msg->type == _mt_abort || msg->type == _mt_quit))
					{
						if((p = (uchar *)pgp_malloc(1)) != NULL)
						{
							// Send out a hangup request after clearing the queue
							// and wait 4.1 seconds for a reply
							*p = _ctp_Hangup;
							mControlOutQ->Send(_mt_controlPacket, p, 1);
							hACK = mMsgQueue->Recv(4100);
							//pgpAssert(!hACK || hACK->type == _mt_ack);
						}
					}
					mSoundInput->SetPacketThread(NIL);
					mControlOutQ->Send(_mt_abort);
					while(!mOutHPPResult)
						(Yield)();
					mInHPP->AbortSync();
					while(!mInHPPResult)
						(Yield)();
					delete mControlOutQ;
					mControlOutQ = NIL;
					if(gPacketWatch)
						gPacketWatch->SetHPPQueue(NIL);
					delete mSoundOutQ;
					mSoundOutQ = NIL;
				}
				if(mTransport)
				{
					if(mTransport->GetState() == _cs_connected)
						mTransport->Disconnect();
					if(!mDone && gPGFOpts.popt.alwaysListen)
						mMsgQueue->SendUnique(_mt_waitCall);
					else
					{
						delete mTransport;
						mTransport = NIL;
					}
				}
				byteFifoDestroy(&mSharedSecret);	//destroy all records of key
				byteFifoDestroy(&mOAuthentication);	//material for this call
				byteFifoDestroy(&mAAuthentication);
				memset(mRemotePublicName, 0, 64);	//destroy record of remote party
				mControlState = _con_None;
				break;
			case _mt_answer:
			case _mt_waitCall:
				if(mControlState != _con_None)
					break;
				SetupTransport(gPGFOpts.popt.connection);
				if(mTransport)
				{
#ifndef DIRECTCONNECT
					if(mTransport->Reset())
					{
						delete mTransport;
						mTransport = NIL;
					}
					else if(!(err = mTransport->Listen(msg->type == _mt_answer)) &&
							(mTransport->GetState() == _cs_connected))
					{
						mOriginator = FALSE;
						StartPhone();
					}
					else if(err != _pge_InternalAbort)
					{
						if(gPGFOpts.popt.alwaysListen && !mDone)
							mMsgQueue->SendUnique(_mt_waitCall);
						else
						{
							delete mTransport;
							mTransport = NIL;
						}
					}
#else
					mPFWindow->SetState(_cs_connected);
					mOriginator = FALSE;
					StartPhone();
#endif
				}
				break;
			case _mt_call:
				contact = (ContactEntry *)msg->data;
				reply = _cr_NoReply;
				SetupTransport(contact->method);
				if(mTransport)
				{
#ifndef DIRECTCONNECT
					pgpAssert(contact);
					if(mTransport->Reset())
					{
						delete mTransport;
						mTransport = NIL;
					}
					else if(mTransport->Connect(contact, &reply))
					{
						delete mTransport;
						mTransport = NIL;
					}
					else if(reply == _cr_Connect)
					{
						mOriginator = TRUE;
						StartPhone();
					}
					else if(gPGFOpts.popt.alwaysListen && !mDone)
						mMsgQueue->SendUnique(_mt_waitCall);
#else
					mPFWindow->SetState(_cs_connected);
					mOriginator = TRUE;
					StartPhone();
#endif
				}
				break;
			case _mt_talk:			// outgoing talk request initiated by us
				if(!mFlipInProgress && mControlOutQ && mTransport)
				{
					if(!mFullDuplex)
					{
						if((p = (uchar *)pgp_malloc(1)) != NULL)
						{
							*p = _ctp_Talk;
							mControlOutQ->Send(_mt_controlPacket, p, 1);
							mFlipInProgress = TRUE;
						}
					}
					else if(!mHaveChannel)
					{
						SetSpeaker(TRUE);
						mSoundInput->Record(TRUE);
					}
				}
				break;
			case _mt_listen:		// outgoing listen request initiated by us
				if(!mFlipInProgress && mControlOutQ && mTransport)
				{
					if(!mFullDuplex)
					{
						if((p = (uchar *)pgp_malloc(1)) != NULL)
						{
							*p = _ctp_Listen;
							mControlOutQ->Send(_mt_controlPacket, p, 1);
							mFlipInProgress = TRUE;
						}
					}
					else if(mHaveChannel)
					{
						mSoundInput->Record(FALSE);
						SetSpeaker(FALSE);
					}
				}
				break;
			case _mt_ack:
				switch((uchar)msg->data)
				{
					case _ctp_Talk:
						if(!mHaveChannel && mFlipInProgress)
						{
							SetSpeaker(TRUE);
							mSoundInput->Record(TRUE);
						}
						mFlipInProgress = FALSE;
						break;
					case _ctp_Listen:
						if(mHaveChannel && mFlipInProgress)
						{
							mSoundInput->Record(FALSE);
							SetSpeaker(FALSE);
						}
						mFlipInProgress = FALSE;
						break;
					case _ctp_OpenVoice:
						if(mChangingEnc)
							mSoundInput->Pause(FALSE);
						mChangingEnc = FALSE;
						break;
					case _ctp_VoiceSwitch:
						if(mChangingDec)
							mSoundOutput->Pause(FALSE);
						mChangingDec = FALSE;
						break;
				}
				msg->data = NIL;
				break;
			case _mt_resetTransport:
				if(mTransport)
				{
					delete mTransport;
					mTransport = NIL;
				}
				if(gPGFOpts.popt.alwaysListen)
					mMsgQueue->SendUnique(_mt_waitCall);
				break;
			case _mt_changeDecoder:
				codec = (long)msg->data;
				if(mChangingDec)
				{
					// The user has become overzealous about
					// changing coders.  Lets set the display back
					// to what was requested a couple seconds
					// ago.
					//+++++todo
				}
				else if(!mChangingDec && ((p = e = (uchar *)pgp_malloc(5)) != NULL))
				{
					mSoundOutput->Pause(TRUE);
					*e++ = _ctp_VoiceSwitch;
					LONG_TO_BUFFER(codec, e);	e+=4;
					mControlOutQ->Send(_mt_controlPacket, p, e-p);
					mSoundOutput->SetCodec(codec);
					mChangingDec = TRUE;
				}
				msg->data = NIL;
				break;
			case _mt_changeEncoder:
				if(mChangingEnc)
				{
					// The user has become overzealous about
					// changing coders.  Lets set the display back
					// to what was requested a couple seconds
					// ago.
					//+++++todo
				}
				else if(!mChangingEnc && ((p = e = (uchar *)pgp_malloc(5)) != NULL))
				{
					mSoundInput->Pause(TRUE);
					*e++ = _ctp_OpenVoice;
					codec = (long)msg->data;
					LONG_TO_BUFFER(codec, e);	e+=4;
					mControlOutQ->Send(_mt_controlPacket, p, e-p);
					mSoundInput->SetCodec(codec);
					mChangingEnc = TRUE;
				}
				msg->data = NIL;
				break;
			case _mt_controlPacket:
				switch(*(p=(uchar *)msg->data))
				{
					case _ctp_Hello:
						if(mControlState == _con_Configuring &&
							mConfigState == _cts_Hello)
						{
							SHORT_TO_BUFFER(msg->len, alen);
							authwrit = byteFifoWrite(mOriginator? &mAAuthentication : &mOAuthentication, alen, 2);
							pgpAssert(authwrit == 2);
							authwrit = byteFifoWrite(mOriginator? &mAAuthentication : &mOAuthentication, p, msg->len);
							pgpAssert(authwrit == msg->len);
							if(ReceiveHello(++p, msg->len - 1))
							{
								if(mPrime &&
									memcmp(mEncryptor, sCryptorHash[_enc_none], 4))
								{
									mConfigState = _cts_DHHash;
									SendDHHash();
								}
								else
								{	//non-encrypted call
									mPrime = NIL;
									mStatusPane->AddStatus(0,
										"Unencrypted connection negotiated.");
									PlayInsecureWarning();
									mInHPP->SetNoCryptoMode();
									mConfigState = _cts_Info;
									SendInfo();
								}
							}
							else
							{
								mStatusPane->AddStatus(0,
									"Bad Hello received.  Aborting call.");
								mMsgQueue->Send(_mt_abort);
							}
						}
						break;
					case _ctp_DHHash:
						if(mControlState == _con_Configuring &&
							mConfigState == _cts_DHHash)
						{
							SHORT_TO_BUFFER(msg->len, alen);
							authwrit = byteFifoWrite(mOriginator? &mAAuthentication : &mOAuthentication, alen, 2);
							pgpAssert(authwrit == 2);
							authwrit = byteFifoWrite(mOriginator? &mAAuthentication : &mOAuthentication, p, msg->len);
							pgpAssert(authwrit == msg->len);
							if(ReceiveDHHash(++p, msg->len - 1))
							{
								mConfigState = _cts_DHPublic;
								SendDHPublic();
							}
							else
							{
								mStatusPane->AddStatus(0,
									"Bad DHHash received.  Aborting call.");
								mMsgQueue->Send(_mt_abort);
							}
						}
						break;
					case _ctp_DHPublic:
						if(mControlState == _con_Configuring &&
							mConfigState == _cts_DHPublic)
						{
							SHORT_TO_BUFFER(msg->len, alen);
							authwrit = byteFifoWrite(mOriginator? &mAAuthentication : &mOAuthentication, alen, 2);
							pgpAssert(authwrit == 2);
							authwrit = byteFifoWrite(mOriginator? &mAAuthentication : &mOAuthentication, p, msg->len);
							pgpAssert(authwrit == msg->len);
							if(ReceiveDHPublic(++p, msg->len - 1))
							{
								mConfigState = _cts_Info;
								SendInfo();
							}
							else
							{
								mStatusPane->AddStatus(0,
									"Bad DHPublic received.  Aborting call.");
								mMsgQueue->Send(_mt_abort);
							}
						}
						break;
					case _ctp_Info:
						if(mControlState == _con_Configuring &&
							mConfigState == _cts_Info)
						{
							if(ReceiveInfo(++p, msg->len - 1))
								ConfigComplete();
						}
						break;
					case _ctp_Hangup:
						mControlState = _con_Disconnecting;
						mStatusPane->AddStatus(0,"Remote disconnected.");
						Sleep(1000);	// Try to allow time for the ACK to go out
						mMsgQueue->Send(_mt_remoteAbort);
						break;
					case _ctp_OpenVoice:
						p++;
						BUFFER_TO_LONG(p, codec);
						mPFWindow->SetDecoder(codec);
						mSoundOutput->Pause(TRUE);
						mSoundOutput->SetCodec(codec);
						mSoundOutput->Pause(FALSE);
#ifdef DEBUGCONFIG
						pgp_memcpy(s,p,4);
						s[4]=0;
						mStatusPane->AddStatus(0,"_ctp_OpenVoice(O): %s",s);
#endif
						break;
					case _ctp_VoiceSwitch:
						p++;
						BUFFER_TO_LONG(p, codec);
						mPFWindow->SetEncoder(codec);
						mSoundInput->Pause(TRUE);
						mSoundInput->SetCodec(codec);
						mSoundInput->Pause(FALSE);
#ifdef DEBUGCONFIG
						pgp_memcpy(s,p,4);
						s[4]=0;
						mStatusPane->AddStatus(0,"_ctp_VoiceSwitch(I): %s",s);
#endif
						break;
					case _ctp_Talk:		// other side wishes to speak
						if(!mFullDuplex && (!mFlipInProgress || !mOriginator))
						{
							mSoundInput->Record(FALSE);
							SetSpeaker(FALSE);
							mFlipInProgress = FALSE;
						}
						break;
					case _ctp_Listen:	// other side wishes to listen
						if(!mFullDuplex && (!mFlipInProgress || !mOriginator))
						{
							SetSpeaker(TRUE);	// disable further playing of
												//	incoming packets
							mSoundOutput->ChannelSwitch();
							mSoundInput->Record(TRUE);
							mFlipInProgress = FALSE;
						}
						break;
					default:
						mStatusPane->AddStatus(0,"Unknown control packet received.");
						break;
				}
				break;
		}
		mMsgQueue->Free(msg);
	}
	return (void *)1L;
}

void
CControlThread::ConfigComplete()
{
	uchar *p, *e;
	short sl, inx;
	char s[128];
	long codec;

	// Configuration complete, start call
	mStatusPane->AddStatus(0, "Configuration complete.");
	mControlState = _con_Phone;
	mSoundInput->Record(FALSE);
	mPFWindow->SetDecoderList((uchar *)mVoiceDecoders,
								mNumDecoders);
	mPFWindow->SetEncoderList((uchar *)mVoiceEncoders,
								mNumEncoders);
	BUFFER_TO_LONG(mEncryptor, codec);
	mPFWindow->SetEncryption(codec, 0);
	mPFWindow->ShowStatus();
	mTransport->BindMediaStreams();
	SetSpeaker(mOriginator || mFullDuplex);
	BUFFER_TO_LONG(mVoiceEncoders, codec);
	mPFWindow->SetEncoder(codec);
	mSoundInput->SetCodec(codec);
	mSoundInput->SetOutput(mOutHPP, mSoundOutQ);
	mSoundInput->Pause(TRUE);
	if(mHaveChannel)
		mSoundInput->Record(TRUE);
	mChangingEnc = TRUE;
	if(gPacketWatch)
		gPacketWatch->SetHPPQueue(mSoundOutQ);
	p = e = (uchar *)pgp_malloc(5);
	*e++ = _ctp_OpenVoice;
	LONG_TO_BUFFER(codec, e);	e+=4;
	mControlOutQ->Send(_mt_controlPacket, p, e-p);
	if(mUseWordList)
	{
		for(sl=0,inx=0;inx<mAuthBytes;inx++)
		{
			sprintf(&s[sl], "%s\r", (inx==1 || inx==3) ?
						hashWordListOdd[mAuthInfo[inx]] :
						hashWordListEven[mAuthInfo[inx]]);
			sl = strlen(s);
		}
	}
	else
	{
		for(sl=0,inx=0;inx<mAuthBytes;inx++)
		{
			sprintf(&s[sl], "%02x\r", mAuthInfo[inx]);
			sl = strlen(s);
		}
	}
	if(mPrime)
		mPFWindow->ShowAuthParams(s);
#ifdef	PGPXFER
	if(mSupportsFileXfer)
	{
		mXferResult = 0;
		mXferQueue = new CMessageQueue(16);
		(mXfer = new CXferThread(mPFWindow, mXferWindow, mOutHPP, mXferQueue,
			mControlOutQ, (void **)&mXferResult))->Resume();
		mPFWindow->AllowXfers(TRUE);
		mInHPP->SetXferQueue(mXferQueue);
		mOutHPP->SetXferThread(mXfer);
		mXferWindow->SetXferThread(mXfer);
	}
#endif
}

void
CControlThread::SetupTransport(enum ContactMethods method)
{
	short result=0;
	 
	if(!mTransport)
	{
		switch(method)
		{
			case _cme_Serial:
				mTransport = new CPFTSerial(&gPGFOpts.sopt, mPFWindow, &result);
				break;
			case _cme_Internet:
				mTransport = new CPFTInternet(mPFWindow, this, &result);
				break;
			case _cme_AppleTalk:
#ifndef PGP_WIN32
				mTransport = new CPFTAppleTalk(mPFWindow, this, &result);
#endif
				break;
		}
		if(result)
		{
			delete mTransport;
			mTransport = NULL;
		}
	}
}

void
CControlThread::StartPhone()
{
	mControlState = _con_Configuring;
	mConfigState = _cts_Hello;
	mRemotePublicName[0]=0;
	mRemoteSystemType = _pst_Generic;
	pgp_memcpy(mEncryptor, sCryptorHash[_enc_none], 4);
	mNumPrimes = 0;
	mPrime = NIL;
	if(mDHPublic)
		pgp_free(mDHPublic);
	if(mDHPrivate)
		pgp_free(mDHPrivate);
	mDHPublic = mDHPrivate = NIL;
	mAuthenticated = mUseWordList = mFullDuplex = mAllowNone =
		mFlipInProgress = mChangingEnc = mChangingDec =
		mSupportsFileXfer = FALSE;
	mRemoteWindow = 1;
	mAuthBytes = 4;
	bnInit();
	byteFifoInit(&mSharedSecret);
	byteFifoInit(&mOAuthentication);
	byteFifoInit(&mAAuthentication);
	gPacketWatch->InitCall();	// reset packet watch
	mControlOutQ = new CMessageQueue(4);
	mSoundOutQ = new CMessageQueue(10);
	mInHPPResult = mOutHPPResult = 0;
	(mInHPP = new CPFPacketsIn(gPGFOpts.popt.connection != _cme_Serial,
		mPFWindow, mSoundOutput, mTransport, mMsgQueue,
		mControlOutQ, mSoundOutput->GetPlayQueue(),
		(void **)&mInHPPResult))->Resume();
	(mOutHPP = new CPFPacketsOut(gPGFOpts.popt.connection != _cme_Serial,
		mPFWindow, mTransport, mSoundInput, mControlOutQ, mSoundOutQ,
		(void **)&mOutHPPResult))->Resume();
	mSoundInput->SetPacketThread(mOutHPP);
#ifdef CONFIGCALL
	// record sound until the call is configured
	mSoundInput->SetCodec('rand');
	mSoundInput->SetOutput(NIL, NIL);
	mSoundInput->Record(TRUE);
	SendHello();	//begin configuration
#else
	// this code exists to test calls without configuration packets
	// and without encryption
	mControlState = _con_Phone;
	mFullDuplex = TRUE;
	mPFWindow->ShowStatus();
	mSoundInput->SetCodec('GS5L');	// hardcoded default configuration
	mSoundInput->SetOutput(mOutHPP, mControlOutQ);
	mSoundInput->Record(TRUE);
	mSoundOutput->SetCodec('GS5L');
	mSoundOutput->Play(TRUE);
#endif
}

// CSoundOutput calls DropCoder when its underflow algorithm
// has determined that the coder currently in use is not
// adequately low CPU or low bandwidth for the connection.
// This tries not to but does partially assume the list of
// coders we got back intersected is listed in an estimate of
// decreasing order of coder complexity

void
CControlThread::DropCoder(ulong curCoder)
{
	short i;
	ulong curTime, codTime;
	long curcps, codcps;
	
	return;
	//This feature has been temporarily disabled.
	
	GetCoderReqs(curCoder, &curTime, &curcps);
	for(i=0;i<mNumDecoders;i++)
	{
		if(curCoder != mVoiceDecoders[i])
		{
			GetCoderReqs(mVoiceDecoders[i], &codTime, &codcps);
			if(codcps < curcps)
			{
				mMsgQueue->SendUnique(_mt_changeDecoder,
										(void *)mVoiceDecoders[i]);
				return;
			}
		}
	}
	mStatusPane->AddStatus(0,"Alternate coders exhausted, "
								"connection is hopeless.");
}

//	This function sets the "speaker" or the talking party in a half duplex
//	conversation.  It also passes the current information up to the window
//	so that the appropriate information can be displayed to the user

void
CControlThread::SetSpeaker(Boolean speaker)
{
	mHaveChannel = speaker;
	mSoundOutput->Play(!speaker || mFullDuplex);
	mPFWindow->SetSpeaker(speaker, mFullDuplex);
}

//	Other threads can call this function to abort a call
//	or cause the thread to exit

void
CControlThread::AbortSync(Boolean exit, Boolean remote)
{
	mControlState = _con_Disconnecting;
	if(mTransport)
		mTransport->AbortSync();
	if(exit)
	{
		mDone = TRUE;
		mMsgQueue->SendUnique(_mt_quit);
	}
	else if(remote)
		mMsgQueue->SendUnique(_mt_remoteAbort);
	else
		mMsgQueue->SendUnique(_mt_abort);
}

//	The following function is called when the user switches port information
//	or switches from Modem <-> Internet while the program is running.

void
CControlThread::ResetTransport()
{
	if(mTransport)
		mTransport->AbortSync();
	mMsgQueue->SendUnique(_mt_resetTransport);
}

#define GSM7INDEX	7
#define GSM7LINDEX	2

void
CControlThread::SetCoderList(CoderEntry *coders, short numCoders)
{
	mCoders = coders;
	mNumCoders = numCoders;
	if(gPacketWatch)
		gPacketWatch->SetGSMTimes(
			coders[GSM7INDEX].encTime, coders[GSM7LINDEX].encTime,
			coders[GSM7INDEX].decTime, coders[GSM7LINDEX].decTime);	//##### hardcoded GSM 7350/Lite
	if(coders[GSM7INDEX].encTime > 750)	// 1 second GSM encode took >70% of processor!
		PGFAlert("PGPfone has determined that this computer is probably too slow.", 0);
}

char *
CControlThread::GetCoderName(ulong coder)
{
	if(mCoders)
	{
		for(short inx = 0;inx<mNumCoders;inx++)
			if(coder == (ulong)mCoders[inx].code)
				return mCoders[inx].name;
	}
	return NIL;
}

void
CControlThread::GetCoderReqs(ulong coder, ulong *reqTime, long *reqcps)
{
	if(mCoders)
	{
		for(short inx = 0;inx<mNumCoders;inx++)
			if(coder == (ulong)mCoders[inx].code)
			{
				*reqTime = mCoders[inx].encTime;
				*reqcps = mCoders[inx].reqcps;
				return;
			}
	}
}

void
CControlThread::SendInfo()
{
	uchar *p, *e;
	short dInx, eInx, inx;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"SendInfo");
#endif
	// the following block of code sends the _ctp_Info packet
	// which is the first packet sent under cloak.  It
	// negotiates various parameters such as voice encoders,
	// duplex, sync, wordlist/hex, and # of authentication bytes
	p = e = (uchar *)pgp_malloc(512);
	pgpAssert(p);
	*e++ = _ctp_Info;
	if(!gPGFOpts.popt.idUnencrypted &&
		((mOriginator && gPGFOpts.popt.idOutgoing) ||
		(!mOriginator && gPGFOpts.popt.idIncoming)) &&
		gPGFOpts.popt.identity[0])
	{
		*e++ = _cti_PublicName;
		*e++ = strlen(gPGFOpts.popt.identity);
		strcpy((char *)e, gPGFOpts.popt.identity);
		e+= strlen((char *)e);
	}
	*e++ = _cti_SystemType;
	*e++ = 1;
#ifdef PGP_MACINTOSH
	*e++ = _pst_Macintosh;
#else
	*e++ = _pst_Generic;
#endif
	*e++ = _cti_Duplex;
	*e++ = 1;
	*e++ = (gPGFOpts.popt.fullDuplex);
	*e++ = _cti_WordList;	// We want the word list by default
	*e++ = 1;
	*e++ = 1;
	*e++ = _cti_AuthenticationBytes;// Number of Authentication Bytes
	*e++ = 1;
	*e++ = mAuthBytes;
	// Setup list of voice encoders and decoders
	LONG_TO_BUFFER(gPGFOpts.popt.prefCoder, mVoiceDecoders);
	dInx = 4;
	mNumDecoders = 1;
	if(gPGFOpts.popt.prefCoder != gPGFOpts.popt.altCoder)
	{
		LONG_TO_BUFFER(gPGFOpts.popt.altCoder, mVoiceDecoders+dInx);
		dInx+=4;
		mNumDecoders++;
	}
	mNumEncoders = 0;
	eInx = 0;
	pgpAssert(mCoders);
	for(inx = mNumCoders-1;inx>=0;inx--)
	{
		if((mCoders[inx].code != gPGFOpts.popt.prefCoder) &&
			(mCoders[inx].code != gPGFOpts.popt.altCoder) &&
			((mCoders[inx].reqSpeed <= gPGFOpts.sopt.maxBaud) ||
			(gPGFOpts.popt.connection != _cme_Serial)))
		{
			LONG_TO_BUFFER(mCoders[inx].code, mVoiceDecoders+dInx);
			dInx+=4;
			mNumDecoders++;
		}
		if((mCoders[inx].encTime < MAXCODERTIMETHRESHOLD) &&
			((mCoders[inx].reqSpeed <= gPGFOpts.sopt.maxBaud) ||
			(gPGFOpts.popt.connection != _cme_Serial)))
		{
			LONG_TO_BUFFER(mCoders[inx].code, mVoiceEncoders+eInx);
			eInx+=4;
			mNumEncoders++;
		}
	}
	*e++ = _cti_VoiceEncoders;	// Send our list of encoders
	*e++ = eInx;
	pgp_memcpy(e, mVoiceEncoders, eInx);
	e+=eInx;
	*e++ = _cti_VoiceDecoders;	// Send our list of decoders
	*e++ = dInx;
	pgp_memcpy(e, mVoiceDecoders, dInx);
	e+=dInx;
#ifdef PGPXFER
	*e++ = _cti_FileTransfer;
	*e++ = 2;
	SHORT_TO_BUFFER((short)MAXSAVEDOOPACKETS, e);	e+=2;
#endif
	mControlOutQ->Send(_mt_controlPacket, p, e-p);
}

Boolean
CControlThread::ReceiveInfo(uchar *data, short length)
{
	uchar *p, *e;
	ulong *cd;
	short len, num, inx, inx2, nd;
	Boolean good = TRUE;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"ReceiveInfo");
#endif
	len = 0;
	for(p=data,e=data+length;p<e;p+=len)
	{
		num = *p++;
		if(p == e)
		{
			good = FALSE;
			break;
		}
		len = *p++;
		if(e-p < len)
		{
			good = FALSE;
			break;
		}
		switch(num)
		{
			case _cti_PublicName:
				if(len>0)
				{
					if(len>63)
						len=63;
					pgp_memcpy(mRemotePublicName,p,len);
					mRemotePublicName[len]=0;
				}
				break;
			case _cti_SystemType:
				mRemoteSystemType = (enum SystType)*p;
				break;
			case _cti_Duplex:
				if(gPGFOpts.popt.fullDuplex && *p)
					mFullDuplex = TRUE;
				break;
			case _cti_Sync:
				//we dont support it, so it wont be negotiated
				break;
			case _cti_VoiceEncoders:
				num = len / 4;
				cd = (ulong *)p;
				for(inx=0;inx<mNumDecoders;inx++)
				{
					for(inx2=0;inx2<num;inx2++)
					{
						if(!memcmp(&mVoiceDecoders[inx*4],&cd[inx2],4))
							break;
					}
					if(inx2>=num)
					{
						if(inx<mNumDecoders-1)
							pgp_memcpy(&mVoiceDecoders[inx*4],
										&mVoiceDecoders[(inx+1)*4],
										(mNumDecoders-1-inx) * 4);
						mNumDecoders--;
						inx--;
					}
				}
				if(!mNumDecoders)
				{
					mStatusPane->AddStatus(0,
						"Could not negotiate a voice decoder.");
					good = FALSE;
				}
				break;
			case _cti_VoiceDecoders:
				nd = num = len / 4;
				cd = (ulong *)p;
				for(inx=0;inx<nd;inx++)
				{
					for(inx2=0;inx2<mNumEncoders;inx2++)
					{
						if(!memcmp(&mVoiceEncoders[inx2*4],&cd[inx],4))
							break;
					}
					if(inx2>=mNumEncoders)
					{
						if(inx<nd-1)
							pgp_memcpy(&cd[inx], &cd[inx+1], (nd-1-inx) * 4);
						num--;
					}
				}
				if(num > 32)
					num = 32;
				mNumEncoders = num;
				pgp_memcpy(mVoiceEncoders, p, num * 4);
				if(!mNumEncoders)
				{
					mStatusPane->AddStatus(0,
						"Could not negotiate a voice encoder.");
					good = FALSE;
				}
				break;
			case _cti_AuthenticationBytes:
				if(*p < mAuthBytes && *p >= 2)
					mAuthBytes = *p;
				break;
			case _cti_WordList:
				if(*p)
					mUseWordList = TRUE;
				break;
			case _cti_FileTransfer:
				mSupportsFileXfer = TRUE;
				BUFFER_TO_SHORT(p, mRemoteWindow);
				break;
			default:
				// unknown data, so ignore it
				break;
		}
	}	
	return good;
}

Boolean
CControlThread::ReceiveDHHash(uchar *data, short length)
{
	Boolean good = TRUE;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"ReceiveDHHash");
#endif
	if(length >= SHS_DIGESTSIZE)
		pgp_memcpy(mDHHash, data, SHS_DIGESTSIZE);
	else
		good = FALSE;
	return good;
}

void
CControlThread::SendDHHash()
{
	uchar *p, *e, alen[4];
	long authwrit;
	SHA hash;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"SendDHHash");
#endif
	// The following block of code sends the _ctp_DHHash packet
	// which sends a hash of the public Diffie-Hellman calculation
	// This is integral to the security by making sure that both
	// parties commit to a value before revealing the public
	// calculation
	p = e = (uchar *)pgp_malloc(512);
	pgpAssert(p);
	*e++ = _ctp_DHHash;
	// Calculate first half of the Diffie-Hellman
	mDHPublic = (uchar *)pgp_malloc(mPrime->length);
	mDHPrivate = (uchar *)pgp_malloc(mPrime->length);
	if(dhFirstHalf(mPrime->prime, mPrime->length, mPrime->expSize,
		dhGenerator, sizeof(dhGenerator),
		mDHPrivate, mPrime->length,
		mDHPublic) < 0)
		pgp_errstring("Diffie-Hellman error.");
	// Hash the Public field and commit to it by sending it to
	// the other side
	hash.Init();
	hash.Update(mDHPublic, mPrime->length);
	hash.Final(e);
	e+=SHS_DIGESTSIZE;
	SHORT_TO_BUFFER((short)(e-p), alen);
	authwrit = byteFifoWrite(mOriginator? &mOAuthentication : &mAAuthentication, alen, 2);
	pgpAssert(authwrit == 2);
	authwrit = byteFifoWrite(mOriginator? &mOAuthentication : &mAAuthentication, p, e-p);
	pgpAssert(authwrit == e-p);
	mControlOutQ->Send(_mt_controlPacket, p, e-p);
}

void
CControlThread::SetupEncryption(short dir)
{
	uchar s[256];
	ulong len;
	ushort keylen;
	uchar *p, *e;

	keylen = KeySize(mEncryptor);
	if(byteFifoSize(&mSharedSecret) < keylen)
		ProtocolViolation(_pv_badPrime);
	else
	{
		for(e=s;keylen;e+=len)
		{
			p = byteFifoGetBytes(&mSharedSecret, &len);
			len = (keylen < len ? keylen : len);
			pgp_memcpy(e, p, len);
			byteFifoSkipBytes(&mSharedSecret, len);
			keylen -= len;
		}
		if(dir)
			mOutHPP->ChangeTXKey(s, mEncryptor, 0, 0);
		else
			mInHPP->ChangeRXKey(s, mEncryptor, 0, 0);
#ifdef DEBUGCONFIG
		char t[256];
		short i, ks = KeySize(mEncryptor);
		
		for(i=0,p=(uchar *)t;i<ks;i++,p+=2)
			sprintf((char *)p, "%02x", s[i]);
		*p=0;
		if(dir)
			strcat(t, "(e)");
		else
			strcat(t, "(d)");
		mStatusPane->AddStatus(0,t);
		DebugLog("%s",t);
#endif
		memset(s, 0, 256);	// erase key material after use
	}
}

Boolean
CControlThread::ReceiveDHPublic(uchar *data, short length)
{
	Boolean good = TRUE;
	short plen, err;
	ulong len;
	uchar *e, *p, h[32];
	SHA hash;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"ReceiveDHPublic");
#endif
	e = data;
	BUFFER_TO_SHORT(e, plen);
	e+=2;
	if((plen == mPrime->length) && (length == mPrime->length+sizeof(short)))
	{
		// Make sure the public field matches the hash they sent
		hash.Init();
		hash.Update(e, plen);
		hash.Final(h);
		if(!memcmp(h, mDHHash, SHS_DIGESTSIZE))
		{
			// Do second half of Diffie-Hellman
			// to obtain encryption keys
			err = dhSecondHalf(mPrime->prime, mPrime->length,
				dhGenerator, sizeof(dhGenerator),
				mDHPrivate, mPrime->length,
				e, mPrime->length, &mSharedSecret, NIL);
			pgpAssert(err>=0);
			mStatusPane->AddStatus(0,
				"Completed Diffie-Hellman key agreement.");
			hash.Init();
			len = byteFifoSize(&mOAuthentication);
			while((p = byteFifoGetBytes(&mOAuthentication, &len)) != NULL)
			{
				hash.Update(p, len);
				byteFifoSkipBytes(&mOAuthentication, len);
			}
			len = byteFifoSize(&mAAuthentication);
			while((p = byteFifoGetBytes(&mAAuthentication, &len)) != NULL)
			{
				hash.Update(p, len);
				byteFifoSkipBytes(&mAAuthentication, len);
			}
			hash.Final(h);
			pgp_memcpy(mAuthInfo, h, 4);
			// all future packets are now encrypted
			SetupEncryption(mOriginator);
			SetupEncryption(!mOriginator);
		}
		else
			good = FALSE;
	}
	else
		good = FALSE;
	return good;
}

void
CControlThread::SendDHPublic()
{
	uchar *p, *e, alen[4];
	long authwrit;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"SendDHPublic");
#endif
	// the following block of code sends the _ctp_DHPublic packet
	// which is the result of the initial Diffie-Hellman
	// calculation previously committed to by the hash we
	// sent.
	p = e = (uchar *)pgp_malloc(768);
	pgpAssert(p);
	pgpAssert(mPrime->length < 700);
	*e++ = _ctp_DHPublic;
	SHORT_TO_BUFFER(mPrime->length, e);
	e+=2;
	pgp_memcpy(e, mDHPublic, mPrime->length);
	e+=mPrime->length;
	SHORT_TO_BUFFER((short)(e-p), alen);
	authwrit = byteFifoWrite(mOriginator? &mOAuthentication : &mAAuthentication, alen, 2);
	pgpAssert(authwrit == 2);
	authwrit = byteFifoWrite(mOriginator? &mOAuthentication : &mAAuthentication, p, e-p);
	pgpAssert(authwrit == e-p);
	mControlOutQ->Send(_mt_controlPacket, p, e-p);
}

Boolean
CControlThread::ReceiveHello(uchar *data, short length)
{
	uchar *p, *e, *u, *t, *v, s[NUM_DH_PRIMES * 8], rmtSalt[8], h[32],
			*foundHash=NIL, alg;
	short len, num, inx, cnt, cryp, j, found, foundPrime=-1, localPrime=-1;
	uchar vers;
	Boolean good;
	SHA hash;
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"ReceiveHello");
#endif
	good = TRUE;
	len = 0;
	for(p=data,e=data+length;p<e;p+=len)
	{
		num = *p++;
		if(p == e)
		{
			good = FALSE;
			break;
		}
		len = *p++;
		if(e-p < len)
		{
			good = FALSE;
			break;
		}
		switch(num)
		{
			case _cti_Version:
				vers = *p;
				if(vers > pgpfMajorProtocolVersion)
				{
					mStatusPane->AddStatus(0, "Incompatible PGPfone versions, terminating call.");
					mStatusPane->AddStatus(0, "Please get the latest PGPfone at:");
					mStatusPane->AddStatus(0, "http://web.mit.edu/pgp");
					good = FALSE;
				}
				break;
			case _cti_PublicName:
				if(len>0)
				{
					if(len>63)
						len=63;
					pgp_memcpy(mRemotePublicName,p,len);
					mRemotePublicName[len]=0;
				}
				break;
			case _cti_HashAlg:
				if(len >= 4)
				{
					if(memcmp(p, "SHA1", 4))	// only SHA for now
					{
						mStatusPane->AddStatus(0,
							"Could not negotiate a hash algorithm.");
						good = FALSE;
					}
				}
				break;
			case _cti_PrimeHashList:
				mPrime = NIL;
				if(len >= 16)
				{
					pgp_memcpy(rmtSalt, p, 8);
					for(u=s,inx=0;inx<mNumPrimes;inx++,u+=8)
					{
						hash.Init();
						hash.Update(rmtSalt, 8);
						alg = 2;
						hash.Update(&alg, 1);
						hash.Update(mPrimes[inx]->prime,
									mPrimes[inx]->length);
						hash.Final(h);
						pgp_memcpy(u, h, 8);
					}
					v=p+8;
					found = (len - 8) / 8;
					for(u=s,inx=0;inx<mNumPrimes;inx++,u+=8)
					{
						for(t=v,j=0;j<found;j++,t+=8)
						{
							if(!memcmp(u, t, 8))
							{
								// Remember this prime if it's earlier
								// (more preferred) in their list than
								// the one we've already found.
								if(!foundHash || (t<foundHash))
								{
									foundHash=t;
									foundPrime=inx;
								}
								// Remember the first prime we find (in
								// descending order of *our* priority)
								// that's on their list.
								if(localPrime<0)
									localPrime = inx;
							}
						}
					}
					if(localPrime>=0 && foundPrime>=0)
					{
						if(mPrimes[localPrime]->length > mPrimes[foundPrime]->length)
							mPrime = mPrimes[localPrime];
						else if(mPrimes[localPrime]->length < mPrimes[foundPrime]->length)
							mPrime = mPrimes[foundPrime];
						else if(memcmp(mPrimes[localPrime]->prime,
										mPrimes[foundPrime]->prime,
										mPrimes[localPrime]->length) > 0)
							mPrime = mPrimes[localPrime];
						else
							mPrime = mPrimes[foundPrime];
					}
					if(mPrime)
					{
						char str[32];

						sprintf(str,"DH(%d)",mPrime->length * 8);
						mPFWindow->SetKeyExchange(str);
						mStatusPane->AddStatus(0,
							"Agreed upon a %d bit prime.",
							mPrime->length * 8);
					}
				}
				if(!mPrime)
				{
					mStatusPane->AddStatus(0,
						"Could not negotiate a prime.");
					good = FALSE;
				}
				break;
			case _cti_EncryptAlgorithms:
				cnt = len / 4;
				if(mOriginator)
				{
					found = -1;
					for(u=p,inx=0;inx<cnt;inx++,u+=4)
						if(!memcmp(sCryptorHash[gPGFOpts.popt.prefEncryptor], u, 4))
						{
							found = gPGFOpts.popt.prefEncryptor;
							break;
						}
					if(found < 0)
						for(cryp=0;cryp<NUMCRYPTORS && found<0;cryp++)
							if((cryp != gPGFOpts.popt.prefEncryptor) &&
								(gPGFOpts.popt.cryptorMask & (1<<cryp)))
							{
								for(u=p,inx=0;inx<cnt;inx++,u+=4)
									if(!memcmp(sCryptorHash[cryp], u, 4))
									{
										found = cryp;
										break;
									}
							}
					
				}
				else
				{
					for(u=p,inx=0;inx<cnt;inx++,u+=4)
						if(gPGFOpts.popt.cryptorMask & (1<<(cryp=GetCryptorID(u))))
						{
							found = cryp;
							break;
						}
				}
				if(found>=0)
					pgp_memcpy(mEncryptor, sCryptorHash[found], 4);
				else
				{
					mStatusPane->AddStatus(0, "Could not negotiate encryption.");
					good = FALSE;
				}
				break;
			default:
				// unknown data, so ignore it
				break;
		}
	}
	return good;
}

void
CControlThread::SendHello()
{
	uchar *p, *e, *r, *u, s[128], alg, alen[4];
	ulong authwrit;
	short inx, cnt;
	SHA hash;
	static DHPrime *primes[NUM_DH_PRIMES] = {
		&DH_768bitPrime, &DH_1024bitPrime,
		&DH_1536bitPrime, &DH_2048bitPrime, &DH_3072bitPrime,
		&DH_4096bitPrime };
	
#ifdef DEBUGCONFIG
	mStatusPane->AddStatus(0,"SendHello");
#endif
	// the following block of code sends the _ctp_Hello packet
	// which is the first config packet establishing basic
	// parameters such as which prime and which encryption
	// algorithms to use.
	p = e = (uchar *)pgp_malloc(512);
	pgpAssert(p);
	*e++ = _ctp_Hello;
	*e++ = _cti_Version;
	*e++ = 2;
	*e++ = pgpfMajorProtocolVersion;
	*e++ = pgpfMinorProtocolVersion;
	if(gPGFOpts.popt.idUnencrypted &&
		((mOriginator && gPGFOpts.popt.idOutgoing) ||
		(!mOriginator && gPGFOpts.popt.idIncoming)) &&
		gPGFOpts.popt.identity[0])
	{
		*e++ = _cti_PublicName;
		*e++ = strlen(gPGFOpts.popt.identity);
		strcpy((char *)e, gPGFOpts.popt.identity);
		e+= strlen((char *)e);
	}
	*e++ = _cti_HashAlg;
	*e++ = 4;
	pgp_memcpy(e, "SHA1", 4);
	e+=4;
	*e++ = _cti_PrimeHashList;
	u=e++;
	// Setup internal structure of prime preferences
	// Start with preferred prime upwards and then down
	for(inx=gPGFOpts.popt.prefPrime;inx<NUM_DH_PRIMES;inx++)
	{
		if(gPGFOpts.popt.primeMask & (1 << inx))
		{
			mPrimes[mNumPrimes] = primes[inx];
			mNumPrimes++;
		}
	}
	for(inx=gPGFOpts.popt.prefPrime-1;inx>=0;inx--)
	{
		if(gPGFOpts.popt.primeMask & (1 << inx))
		{
			mPrimes[mNumPrimes] = primes[inx];
			mNumPrimes++;
		}
	}
	// send random salt to obscure primes
	randPoolGetBytes(r=e, 8);
	e+=8;
	cnt = 8;
	// send salted hashes of allowed primes in preferred order 
	for(inx=0;inx<mNumPrimes;inx++)
	{
		hash.Init();
		hash.Update(r, 8);	// Hash the salt with the prime
		alg = 2;			// Diffie-Hellman Algorithm Identifier
		hash.Update(&alg, 1);
		hash.Update(mPrimes[inx]->prime, mPrimes[inx]->length);
		hash.Final(s);
		pgp_memcpy(e, s, 8);
		e+=8;
		cnt+=8;
	}
	*u = cnt;
	// send a list of the encryption algorithms we support preference first
	*e++ = _cti_EncryptAlgorithms;
	u=e++;	cnt = 0;
	if(mOriginator && (gPGFOpts.popt.prefEncryptor >= 0))
	{
		pgp_memcpy(e, sCryptorHash[gPGFOpts.popt.prefEncryptor], 4); e+=4; cnt+=4;
	}
	for(inx=0;inx<NUMCRYPTORS;inx++)
	{
		if((!mOriginator || (inx != gPGFOpts.popt.prefEncryptor)) &&
			(gPGFOpts.popt.cryptorMask & (1<<inx)))
		{
			pgp_memcpy(e, sCryptorHash[inx], 4); e+=4; cnt+=4;
		}
	}
	*u = cnt;
	if(gPGFOpts.popt.cryptorMask & (1 << _enc_none))
		mAllowNone = TRUE;
	SHORT_TO_BUFFER((short)(e-p), alen);
	authwrit = byteFifoWrite(mOriginator? &mOAuthentication : &mAAuthentication, alen, 2);
	pgpAssert(authwrit == 2);
	authwrit = byteFifoWrite(mOriginator? &mOAuthentication : &mAAuthentication, p, e-p);
	pgpAssert(authwrit == e-p);
	mControlOutQ->Send(_mt_controlPacket, p, e-p);
}

void
CControlThread::ProtocolViolation(enum ProtViolation id)
{
	// This function is intended to receive notices from the lower
	// layers about protocol violations so it can decide what to
	// do about them.
	
	switch(id)
	{
		case _pv_invalidSound:
			// In the case of serial, we want to disconnect here.  The most
			// likely cause is that the remote party has disconnected
			// unexpectedly and our own packets are being echoed back
			// to us, decrypted by the wrong key, and attempted to be
			// played.
			// In the case of internet or appletalk, we'll just ignore it
			// because connection shutdown is very clean on both of those.
			
			if(mControlState != _con_Disconnecting)
			{
				AbortSync(FALSE, TRUE);	//act like the other side hungup
				mStatusPane->AddStatus(0, "Unexpected disconnection.");
			}
			break;
		case _pv_badPrime:
			// The Diffie-Hellman prime selected is not adequate for the
			// encryption algorithms selected.
			
			if(mControlState != _con_Disconnecting)
			{
				AbortSync(FALSE, FALSE);
				mStatusPane->AddStatus(0, "Diffie-Hellman prime not large enough, terminating call.");
			}
			break;
		case _pv_noRemote:
			// The remote end has failed to respond to a reliable packet.
			// We retry 10 times over 20 seconds.  No response disconnects
			// the call.
			
			if(mControlState != _con_Disconnecting)
			{
				AbortSync(FALSE, TRUE);
				mStatusPane->AddStatus(0, "Remote is not responding, terminating call.");
			}
			break;
	}
}

void
CControlThread::GetCryptorName(short id, uchar *name)
{
	switch(id)
	{
		case _enc_none:		pgp_memcpy(name, "NONE", 4);	break;
		case _enc_tripleDES:pgp_memcpy(name, "TDEA", 4);	break;
		case _enc_blowfish:	pgp_memcpy(name, "BLOW", 4);	break;
		case _enc_cast:		pgp_memcpy(name, "CAST", 4);	break;
	}
}

short
CControlThread::GetCryptorID(uchar *name)
{
	if(!memcmp(name, "TDEA", 4))
		return _enc_tripleDES;
	else if(!memcmp(name, "BLOW", 4))
		return _enc_blowfish;
	else if(!memcmp(name, "CAST", 4))
		return _enc_cast;
	else if(!memcmp(name, "NONE", 4))
		return _enc_none;
	return _enc_none+1;		// return one higher than known for invalid
}

void
CControlThread::GetRemoteName(char *name)
{
	strcpy(name, mRemotePublicName);
}

void
CControlThread::SetRemoteName(char *name)
{
	strcpy(mRemotePublicName, name);
}

void
CControlThread::PlayInsecureWarning()
{
#ifdef	PGP_MACINTOSH
	SndListHandle respSound;
	
	if((respSound = (SndListHandle)GetNamedResource('snd ', "\pInsecure")) != NULL)
	{
		HLock((Handle)respSound);
		SndPlay(nil, respSound, false);
		HUnlock((Handle)respSound);
	}
#elif	PGP_WIN32
	PlaySound("INSECURE.WAV", NULL, SND_FILENAME+SND_SYNC);
#endif
}

void
CControlThread::SetXferWindow(CXferWindow *xferWindow)
{
	mXferWindow = xferWindow;
}

enum SystType
CControlThread::GetRemoteSystemType()
{
	return mRemoteSystemType;
}

