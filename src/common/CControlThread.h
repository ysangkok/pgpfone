/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CControlThread.h,v 1.4 1999/03/10 02:32:23 heller Exp $
____________________________________________________________________________*/
#ifndef CCONTROLTHREAD_H
#define CCONTROLTHREAD_H

#include "CCoderRegistry.h"
#include "CPFWindow.h"
#include "PGPFoneUtils.h"
#include "DHPrimes.h"
#include "bytefifo.h"

#ifdef PGP_WIN32
#include "LThread.h"
#endif	// PGP_WIN32

class CXferWindow;
class CPFTransport;
class CMessageQueue;
class CStatusPane;
class CPFPacketsIn;
class CPFPacketsOut;
class CXferThread;
class CSoundInput;
class CSoundOutput;

enum ConState		{
						_con_None,
						_con_Configuring,
						_con_Phone,
						_con_Disconnecting
					};
enum ConfigState 	{	_cts_Hello,
						_cts_DHHash,
						_cts_DHPublic,
						_cts_Info
					};
enum ControlPacket	{	_ctp_Hello,			//initial configuration packet
						_ctp_DHHash,		//Diffie-Hellman public hash
						_ctp_DHPublic,		//Diffie-Hellman public
						_ctp_Info,			//configuration info
						_ctp_Hangup,		//hangup call
						_ctp_OpenVoice,		//speaker setting new coder
						_ctp_VoiceSwitch,	//listener requests coder switch
						_ctp_Talk,
						_ctp_Listen
					};
enum ConfigInfo		{	_cti_Version,		//Part of the Hello packet
						_cti_PublicName,
						_cti_HashAlg,
						_cti_PrimeHashList,
						_cti_EncryptAlgorithms,
						_cti_VoiceEncoders,	//Part of the Info packet
						_cti_VoiceDecoders,
						_cti_Duplex,
						_cti_Sync,
						_cti_AuthenticationBytes,
						_cti_WordList,
						_cti_FileTransfer,
						_cti_SystemType
					};
enum SystType		{
						_pst_Generic,
						_pst_Macintosh
					};
enum ProtViolation	{
						_pv_invalidSound,	//an error-checked sound packet was invalid
						_pv_badPrime,
						_pv_noRemote
					};
					
class CControlThread		:	public LThread
{
public:
					CControlThread(CMessageQueue *msgQueue,
									CPFWindow *cpfWindow,
									CSoundInput *soundInput,
									CSoundOutput *soundOutput,
									void **outResult);
					~CControlThread();
	void			*Run(void);
	void			StartPhone();
	void			SetSpeaker(Boolean local);
	void			SetCoderList(CoderEntry *coders, short numCoders);
	void			DropCoder(ulong curCoder);
	char			*GetCoderName(ulong coder);
	void			GetCoderReqs(ulong coder, ulong *reqTime, long *reqcps);
	void			SetAuthentication(Boolean authenticated)
										{	mAuthenticated = authenticated;}
	Boolean			GetAuthentication()	{	return mAuthenticated;	}
	enum ConState	GetControlState()	{	return mControlState;	}
	void			ResetTransport();
	void			AbortSync(Boolean exit, Boolean remote=FALSE);
	CPFTransport	*GetTransport()		{	return mTransport;		}
	CMessageQueue	*GetOutQueue()	{	return mControlOutQ;	}
	void			GetCryptorName(short id, uchar *name);
	short			GetCryptorID(uchar *name);
	void			GetRemoteName(char *name);
	void			SetRemoteName(char *name);
	void			ProtocolViolation(enum ProtViolation id);
	void			PlayInsecureWarning();
	Boolean			IsOriginator()		{	return mOriginator;		}
	void			SetXferWindow(CXferWindow *xferWindow);
	enum SystType	GetRemoteSystemType();

	CoderEntry			*mCoders;		// list of coders sorted by time
	short				mNumCoders;
protected:
	void			SendHello();
	Boolean			ReceiveHello(uchar *data, short length);
	void			SendDHHash();
	Boolean			ReceiveDHHash(uchar *data, short length);
	void			SendDHPublic();
	Boolean			ReceiveDHPublic(uchar *data, short length);
	void			SendInfo();
	Boolean			ReceiveInfo(uchar *data, short length);
	void			SetupEncryption(short dir);
	void			SetupTransport(enum ContactMethods method);
	void			ConfigComplete();
	
	enum ConState		mControlState;
	enum ConfigState	mConfigState;
	CXferWindow			*mXferWindow;
	CMessageQueue		*mMsgQueue, *mXferQueue, *mControlOutQ,
						*mSoundOutQ;
	CPFTransport		*mTransport;
	CPFWindow			*mPFWindow;
	CPFPacketsOut		*mOutHPP;
	CPFPacketsIn		*mInHPP;
	CXferThread			*mXfer;
	CSoundInput			*mSoundInput;
	CSoundOutput		*mSoundOutput;
	CStatusPane			*mStatusPane;
	long				mInHPPResult, mOutHPPResult, mXferResult;
	Boolean				mFlipInProgress, mChangingEnc, mChangingDec;
	Boolean				mHaveChannel;		// TRUE = we are speaking now
	Boolean				mChangeWait;		// are we waiting on channel mod response
	enum ControlPacket	mChangeType;		// type of change waiting on
	Boolean				mFullDuplex;		// has full duplex been configured
	Boolean				mAuthenticated;		// hash parameters authenticated?
	Boolean				mOriginator;		// did we initiate the call
	Boolean				mUseWordList;		// use Zimmermann-Juola word list
	uchar				mVoiceEncoders[128];// intersected list
	short				mNumEncoders;		// num coders on intersected list
	uchar				mVoiceDecoders[128];// intersected list
	short				mNumDecoders;		// num coders on intersected list
	Boolean				mSupportsFileXfer;	// Remote supports file transfer
	short				mRemoteWindow;		// Size of remote packet window
	enum SystType		mRemoteSystemType;	// SystType of remote system
	Boolean				mDone;				// thread is exiting
	
	DHPrime				*mPrimes[NUM_DH_PRIMES];
	short				mNumPrimes, mAuthBytes;
	char				mRemotePublicName[64];
	uchar				mDHHash[32];
	uchar				mEncryptor[4], mAuthInfo[4];
	DHPrime				*mPrime;			// prime to use
	uchar				*mDHPublic, *mDHPrivate;
	Boolean				mAllowNone;			// allow no encryption?
	ByteFifo			mSharedSecret;		// Shared Secret buffer
	ByteFifo			mOAuthentication;	// Originator Authentication buffer
	ByteFifo			mAAuthentication;	// Answerer Authentication buffer
};

#endif

