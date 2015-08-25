/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CSoundInput.h,v 1.5 1999/03/10 02:39:59 heller Exp $
____________________________________________________________________________*/
#ifndef CSOUNDINPUT_H
#define CSOUNDINPUT_H

#include "LMutexSemaphore.h"
#include "LThread.h"

#ifdef PGP_WIN32
#include <mmsystem.h>
#endif

#ifdef PGP_MACINTOSH
#include <Sound.h>
#endif

#include "PGPFoneUtils.h"
#include "gsm.h"
#include "ADPCM.h"

#ifdef USEVOXWARE
#include "toolvox.h"
#include "RTShell.h"
#endif

class CLevelMeter;
class CMessageQueue;
class CPFWindow;
class CPFPacketsOut;

#define NUMINPUTBUFFERS			3		//# of raw recording buffers(WIN32)
#define RAWRECORDQUANT			1536
#define SOUNDINPUTQUEUEAVGMAX	5		//the maximum average queueing of outgoing sound
#define SOUNDINPUTQUEUEMAX		8		//the maximum queueing of outgoing sound
#define INSAMPLEBUFMAX			22050
#define DOWNBUFSIZE				22050
#define UPSAMPBUFSIZE			14700	//7350 * 2
#define MAX_THRESHOLD           32767

class CSoundInput	:	public LThread
{
public:
					CSoundInput(CPFWindow *pfWindow, CLevelMeter *levelMeter,
								void **outResult);
					~CSoundInput();
	void			*Run(void);
	Boolean			Recording(void)		{	return mRecording;	}
	void			AbortSync();
	void			SetCodec(ulong type);
	void			DisposeCodec(void);
	
	void			Open();
	void			Close();
	void			Record(Boolean record);
	void			Pause(Boolean pause);
	void			SetOutput(LThread *outThread = NIL, CMessageQueue *outQueue = NIL);
	void			SetPacketThread(CPFPacketsOut *packetThread);
	void			SetGain(short gain);
	void			SetThreshold(long thresh);
	void			SetStatus(Boolean status);

	//	the following shouldn't really be public, but the interrupt
	//	routine is much more efficient if they are
	char			mSamples[INSAMPLEBUFMAX];
	long			mIndex, mOutdex, mSampleBufSize;
#ifdef	PGP_WIN32
	void			CheckWaveError(char *callName, int errCode);
	short			mHeaderIndex;
	HGLOBAL			mMemHandles[NUMINPUTBUFFERS];	// handles for memory
	WAVEHDR			mHeaders[NUMINPUTBUFFERS];	// headers for sound
	uchar			*mMemPtrs[NUMINPUTBUFFERS];	// pointers to memory
#endif
private:
	CPFWindow		*mPFWindow;
	CLevelMeter		*mLevelMeter;
	CMessageQueue	*mOutQueue;
	LThread			*mOutThread;
	CPFPacketsOut	*mPacketThread;
	LMutexSemaphore	mSoundMutex;
	ulong			mCodec;
	short			mSampleSize, mFrames, mFrameSize, mCFrameSize, mCoderSizeSlack;
	long			mCompDone, mDownDone,
					mThreshold,				//0-32767 value minimum amplitude
					mTrailSamples,			//# samples exempt from silence detection
					mDefaultTrail;			//initial value for trail samples
	void			*mCompSnd;
	Boolean			mAbort, mOpen, mPaused, mRecording;
	Boolean			mHardwareIsFullDuplex;	// can we record while we play
	Boolean			mHardwareIs16Bit, mUpSampleSize;// supports 16 bit sound
	char			mDownBuf[DOWNBUFSIZE];
	char			mUpSampleBuf[UPSAMPBUFSIZE];
	
	//	Platform specific members
#ifdef	MACINTOSH
	long			mSoundRef;
	Fixed			mSampleRate, mCoderRate, mGain;
	SPB				mSPB;				// sound input param block
	SICompletionUPP	mSICompProc;
	SIInterruptUPP	mSIIntProc;
	Boolean			mPMExists;			// is Power Manager installed
#elif	PGP_WIN32
	HWAVEIN         mWaveHandle;		// handle for the sound input device
	float			mSampleRate, mCoderRate;
	double			mGain;
	HANDLE			mRecordSemaphore;	// semaphore for controlling sound
#endif

	//	Coder specific members
	gsm				mGSM;
	adpcm_state		mADPCM;
#ifdef USEVOXWARE
	COMPRESSION_VARS	mVoxComVars;
	CODER_REQUIREMENTS	mVoxCoderReqs;
#endif
};

extern Boolean gHardwareIsFullDuplex;

#endif

