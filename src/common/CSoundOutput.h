/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CSoundOutput.h,v 1.5 1999/03/10 02:41:14 heller Exp $
____________________________________________________________________________*/
#ifndef CSOUNTOUTPUT_H
#define CSOUNDOUTPUT_H

#include "LMutexSemaphore.h"
#include "LThread.h"

#include "PGPFoneUtils.h"
#include "gsm.h"
#include "ADPCM.h"

#ifdef USEVOXWARE
#include "toolvox.h"
#include "RTShell.h"
#endif
#ifdef	PGP_WIN32
#include <mmsystem.h>
#endif
#ifdef PGP_MACINTOSH
#include <Sound.h>
#endif

class CMessageQueue;
class CPFWindow;
class CControlThread;

#define SAMPLEBUFMAX		44100L
#define NUMOUTPUTBUFFERS	12

class CSoundOutput		:	public LThread
{
public:
					CSoundOutput(CPFWindow *pfWindow, void **outResult);
					~CSoundOutput();
	void			*Run(void);
	void			SetControlThread(CControlThread *controlThread)
										{mControlThread = controlThread;}
	CMessageQueue	*GetPlayQueue()		{	return mPlayQueue;	}
	long			GetNumPlaying();
	void			SetCodec(ulong type);
	void			DisposeCodec(void);
	void			Pause(Boolean pause);
	void			Play(Boolean play);
	void			Open(void);
	void			Close(void);
	void			SetVolume(long volume);
	void			SetJitter(long jitter);
	void			ChannelSwitch();
	
#ifdef	MACINTOSH
	void			FillDoubleBuffer(SndDoubleBufferPtr buf);
#elif	PGP_WIN32
	DWORD			GetOrgVolume()		{	return mOriginalVolume;	}
	void			CheckWaveError(char *callName, int errCode);
#endif
private:
	ulong			mCodec;			// which codec to use, in 4 char format
	Boolean			mOpen;			// is the device open
	Boolean			mPlay;			// whether it is ok to play sounds now if they arrive
	Boolean			mHardwareIsFullDuplex, mHardwareIs16Bit;
	CMessageQueue	*mPlayQueue;	// incoming sound packets
	LMutexSemaphore	mSoundMutex;	// to prevent collisions between tasks
	long			mVolume;		// Sound Manager 3.0 style stereo 0x01000100 volume level
	Boolean			mPlaying;		// are we playing anything right now?
	CPFWindow		*mPFWindow;		// for notifying of the current coder
	CControlThread	*mControlThread;// to negotiate auto coder dropping
	short			mUnderflowRow,	// number of underflows in a row
					mUnderflowsExceeded;	// number of underflow limit overruns
	Boolean			mSendRR;		// underflows have exceeded limits, send RR
	ulong			mLastRR,		// time we last sent RR
					mUnderStartTime;// time we started measuring underflows
	
	// buffer members
	uchar			mSamples[SAMPLEBUFMAX];	// incoming packets expanded here
	long			mSampleBufLen, mIndex, mOutdex;
	
	// generic coder setup members
	// these can be set to accomodate any fixed coder
	short			mSampleSize, mCoderHeaderSize;
	long			mCFrameSize, mFrameSamples;
	long			mBufferQuant, mPlaypoint;
	ulong			mFinishTime;
	
#ifdef	MACINTOSH
	Fixed			mSampleRate, mCoderRate;
	SndChannelPtr	mSndChannel;	// mac sound channel to play voice on
	SndChannelPtr	mBeepChannel;	// mac sound channel to play half duplex beep on	
	// double buffer members
	SndDoubleBufferPtr		mDblBufOne, mDblBufTwo;
	SndDoubleBufferHeader	mDoubleBufHdr;
	SndDoubleBackUPP		mDoubleBackProc;
	Boolean					mUnderflowing;
#elif	PGP_WIN32
	float			mSampleRate, mCoderRate;
	long			mHeaderIndex;
	HWAVEOUT		mWaveHandle;					// handle of the sound output device
	HGLOBAL			mMemHandles[NUMOUTPUTBUFFERS];// handles for the memory
	WAVEHDR			mHeaders[NUMOUTPUTBUFFERS];	// headers for blocks of sound
	DWORD			mOriginalVolume;				// original sound volume of device
	WAVEHDR			mToneHeader;					// sound output header for the beep
	HGLOBAL			mToneHandles[2];				// handles to buf for ch switch tone
	uchar			*mToneBuffers[2];				// pointer to buf for ch switch tone
	BOOLEAN			mVolumeSupport;					// whether or not we have volume
#endif
	
	// coder specific members
	gsm				mGSM;
	adpcm_state		mADPCM;
#ifdef USEVOXWARE
	DECOMPRESSION_VARS		mVoxComVars;
	DECODER_REQUIREMENTS	mVoxCoderReqs;
	COMPRESSION_VARS		mtVoxComVars;
	CODER_REQUIREMENTS		mtVoxCoderReqs;
#endif
};

#endif

