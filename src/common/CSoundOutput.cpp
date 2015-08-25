/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CSoundOutput.cpp,v 1.5 1999/03/10 02:41:12 heller Exp $
____________________________________________________________________________*/
#include "CSoundOutput.h"

#include "CPFWindow.h"
#include "CControlThread.h"
#include "CStatusPane.h"
#include "CPacketWatch.h"
#include "CMessageQueue.h"

#include "gsm.private.h"
#include "samplerate.h"
#include <string.h>

#ifdef USEVOXWARE
#include "tvgetstr.h"
#endif
#ifdef PGP_MACINTOSH
#include "MacDebug.h"
#endif

#define TOOMANYUNDERFLOWS	2
#define UNDERFLOWLIMIT		5
#define UNDERFLOWTRACKFREQ	5000
#define DEFAULTPLAYPOINT	2
	
#ifdef	PGP_MACINTOSH

	static pascal void
SODoubleBackProc(
	SndChannelPtr		/*theChan*/,
	SndDoubleBufferPtr	doubleBuffer)
{
	((CSoundOutput *)doubleBuffer->dbUserInfo[0])->
		FillDoubleBuffer(doubleBuffer);
}

#elif	PGP_WIN32

#define TONESAMPLES		34							// ~650Hz
#define TONECYCLES		(22050*1/10/TONESAMPLES)	// cycles in 1/10s
#define TONELENGTH		(TONECYCLES*TONESAMPLES)	// samples needed

// 1 cycle of a 400Hz tone sampled at 22050Hz, 16-bits
static short sSwitchTone[TONESAMPLES] =
	{
	0, 6020, 11836, 17249, 22074, 26148, 29331, 31516,	
	32627, 32627, 31516, 29331, 26148, 22074, 17249, 11836,	
	6020, 0, -6020, -11836, -17249, -22074, -26148, -29331,	
	-31516, -32627, -32627, -31516, -29331, -26148, -22074,
	-17249, -11836, -6020
	};
	
static long sBufReturn;

	void CALLBACK
SOCallback(
	HANDLE		hWaveIn,
	WORD		msg,
	DWORD		instance,
	DWORD		param1,
	DWORD		param2)
{
	if(msg==WOM_DONE)
	{
		InterlockedIncrement(&sBufReturn);
		if(sBufReturn==NUMOUTPUTBUFFERS)
			gPacketWatch->Underflow();
	}
}

void
CSoundOutput::CheckWaveError(char *callName, int errCode)
{
	char s[256], t[256];

	if(errCode)
	{
		if(!waveOutGetErrorText(errCode, s, 200))
			sprintf(t, "Error on %s:\n %s", callName, s);
		else
			sprintf(t, "Error on %s:\n %d", callName, errCode);
		PGFAlert(t,0);
		pgpAssert(0);
	}
}

#endif


CSoundOutput::CSoundOutput(CPFWindow *pfWindow, void **outResult)
#ifdef	PGP_MACINTOSH
		: LThread(FALSE, thread_DefaultStack, threadOption_UsePool, outResult)
#else
		: LThread(outResult)
#endif
{
	short err;
	
	mPFWindow = pfWindow;
	mPlayQueue = new CMessageQueue;
	mCodec = 0;
	mIndex = mOutdex = 0;
	mUnderflowRow = 0;
	mUnderflowsExceeded = 0;
	mSendRR = FALSE;
	mUnderStartTime = mLastRR = 0;
	mPlaypoint = DEFAULTPLAYPOINT;
	mPlaying = FALSE;
	mPlay = FALSE;
	mGSM = NIL;
#ifdef	PGP_MACINTOSH
	mOpen = TRUE;
	mUnderflowing = FALSE;
	::GetDefaultOutputVolume(&mVolume);
	mDblBufOne = (SndDoubleBufferPtr)pgp_malloc(sizeof(SndDoubleBuffer) + 3072);
	mDblBufTwo = (SndDoubleBufferPtr)pgp_malloc(sizeof(SndDoubleBuffer) + 3072);
	mDoubleBackProc = NewSndDoubleBackProc(SODoubleBackProc);
	mSndChannel = NIL;	/* allocates default queue of 128 cmds */
	err = SndNewChannel(&mSndChannel, sampledSynth, initMono, NIL);
	pgpAssert(IsntErr(err));
	mSndChannel->userInfo = (long)this;
	mBeepChannel = NIL;
	err = SndNewChannel(&mBeepChannel, squareWaveSynth, initMono, NIL);
	pgpAssert(IsntErr(err));
#elif	PGP_WIN32
	uchar *e;
	short *s, i=TONECYCLES;
	WAVEOUTCAPS devCaps;
	UINT numDevices;
	
	mOpen = FALSE;
	sBufReturn = NUMOUTPUTBUFFERS;
	mHeaderIndex = 0;
	// load buffers for channel switch tone
	// 16-bit, 22050Hz
	mToneHandles[0] = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, TONELENGTH*2);
	// 8-bit, 22050Hz
	mToneHandles[1] = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, TONELENGTH);
	mToneBuffers[0] = (uchar*)GlobalLock(mToneHandles[0]);
	mToneBuffers[1] = (uchar*)GlobalLock(mToneHandles[1]);
	s = (short*)mToneBuffers[0];
	e = mToneBuffers[1];
	// convert the hard-coded 16-bit sample to 8-bits
	pgp_memcpy(s, sSwitchTone, TONESAMPLES*2);
	for(i=0;i<TONESAMPLES;i++)
		*e++ = (uchar)((ushort)(*s++ + 0x8000) >> 8);
	// make the tone by concatenating several copies of the wave
	for(i=1;i<TONECYCLES;i++)
	{
		pgp_memcpy(s, mToneBuffers[0], TONESAMPLES*2);
		pgp_memcpy(e, mToneBuffers[1], TONESAMPLES);
		s+=TONESAMPLES;
		e+=TONESAMPLES;
	}
	mVolumeSupport = FALSE;
	mVolume = 0x7FFF7FFF;
	for(i=0;i<NUMOUTPUTBUFFERS;i++)
	{
		if(!(mMemHandles[i] = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, 3072)))
		{
			pgp_errstring("Error allocating output buffers.");
			return;
		}
		// initialize the header
		mHeaders[i].lpData = (char *)GlobalLock(mMemHandles[i]);
		mHeaders[i].dwFlags = 0;
		mHeaders[i].dwLoops = 0;
		if(!mHeaders[i].lpData)
		{
			pgp_errstring("error locking output buffers");
			return;
		}
	}
	// find out the number of sound devices available
	numDevices = waveOutGetNumDevs();
	if(numDevices < 1)
	{
		PGFAlert("No sound output devices available.", 0);
		return;
	}
	waveOutGetDevCaps(WAVE_MAPPER, &devCaps, sizeof(WAVEOUTCAPS));
	if(devCaps.dwFormats & WAVE_FORMAT_1M16)
	{
		// We'd rather upsample to 11025 than 22050
		mHardwareIs16Bit = TRUE;
		mSampleRate = 11025.0;
	}
	else if(devCaps.dwFormats & WAVE_FORMAT_2M16)
	{
		mHardwareIs16Bit = TRUE;
		mSampleRate = 22050.0;
	}
	else if(devCaps.dwFormats & WAVE_FORMAT_1M08)
	{
		mHardwareIs16Bit = FALSE;
		mSampleRate = 11025.0;
	}
	else if(devCaps.dwFormats & WAVE_FORMAT_2M08)
	{
		mHardwareIs16Bit = FALSE;
		mSampleRate = 22050.0;
	}
	else
		PGFAlert("Sound output device does not support a compatible format.", 0);
	if(devCaps.dwSupport & WAVECAPS_VOLUME ||
		devCaps.dwSupport & WAVECAPS_LRVOLUME)
		mVolumeSupport = TRUE;
	else
		mVolumeSupport = FALSE;
#endif
	SetCodec('GSM7');	// just setting a default coder to init things
}

CSoundOutput::~CSoundOutput()
{
	delete mPlayQueue;
	DisposeCodec();
#ifdef	PGP_MACINTOSH
	::SndDisposeChannel(mSndChannel, TRUE);
	::SndDisposeChannel(mBeepChannel, TRUE);
	DisposeRoutineDescriptor(mDoubleBackProc);
	pgp_free(mDblBufOne);
	pgp_free(mDblBufTwo);
#elif	PGP_WIN32
	short i;

	if(mOpen)
		Close();
	GlobalUnlock(mToneHandles[0]);
	GlobalUnlock(mToneHandles[1]);
	GlobalFree(mToneHandles[0]);
	GlobalFree(mToneHandles[1]);
	for(i=0;i<NUMOUTPUTBUFFERS;i++)
	{
		GlobalUnlock(mMemHandles[i]);
		GlobalFree(mMemHandles[i]);
	}
#endif
}

#ifdef	PGP_MACINTOSH
// FillDoubleBuffer
//	We use the double buffering scheme provided by the Sound Manager
//	in order to get the lowest possible latency.  This routine
//	is called whenever we need to fill one of our double buffers
//	with sound from our buffer of uncompressed samples.
void
CSoundOutput::FillDoubleBuffer(SndDoubleBufferPtr buf)
{
	long avail;
	ushort *p, *e;
	ulong now;
	
	now = pgp_getticks();
	buf->dbFlags = 0;
	if(mIndex > mOutdex)
		avail = mIndex - mOutdex;
	else if(mIndex < mOutdex)
		avail = mSampleBufLen - mOutdex + mIndex;
	else
		avail = 0;
	buf->dbFlags |= dbBufferReady;
	if(mUnderflowing ? (avail >= mBufferQuant * mPlaypoint) :
		(avail >= mBufferQuant))
	{
		::BlockMoveData(&mSamples[mOutdex], &buf->dbSoundData[0], mBufferQuant);
		mOutdex += mBufferQuant;
		mOutdex %= mSampleBufLen;
		mUnderflowRow = 0;
		mUnderflowing = FALSE;
	}
	else
	{
		// UNDERFLOW
		
		//	When we run out of sound to play, we fill the buffer with 0's.
		//	This has the great benefit of keeping the double buffer interrupts
		//	going.  There is a delay of up to 100ms between the call to start
		//	double buffering and when the sound actually starts.  This
		//	technique eliminates that delay on the first underflow.
		mUnderflowing = TRUE;
		gPacketWatch->Underflow();
		p=(ushort *)&buf->dbSoundData[0];
		e=(ushort *)((uchar *)p+mBufferQuant);
		while(p<e)
			*p++=0;
		if(!mPlay)
		{
			buf->dbFlags |= dbLastBuffer;
			mPlaying=FALSE;
		}
		if(++mUnderflowRow == TOOMANYUNDERFLOWS)
		{
			// We send an RR when we reach the TOOMANYUNDERFLOWS limit,
			// but only when we exactly reach the limit.  Since it is
			// possible that the sender has only muted their sound, we
			// wait for the next sound packet to actually reset the
			// mUnderflowRow counter.
			
			if((++mUnderflowsExceeded >= UNDERFLOWLIMIT) &&
				(mLastRR+6000 < now))
			{
				mSendRR = TRUE;
				mLastRR = now;
			}
		}
	}
	if(mUnderStartTime + UNDERFLOWTRACKFREQ < now)
	{
		mUnderStartTime = now;
		mUnderflowsExceeded = 0;
	}
}

#endif	PGP_MACINTOSH


void
CSoundOutput::Open(void)
{
#ifdef	PGP_WIN32
	WAVEFORMATEX waveFormat;
	UINT i, did;
	MMRESULT result;
	short err;

	if(mOpen)
		return;
	// open a device for playing
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 1;
	waveFormat.nSamplesPerSec = (long)mSampleRate;
	waveFormat.nAvgBytesPerSec = (long)mSampleRate * (mHardwareIs16Bit ? 2 : 1);
	waveFormat.nBlockAlign = mHardwareIs16Bit ? 2 : 1;
	waveFormat.wBitsPerSample = 8 * (mHardwareIs16Bit ? 2 : 1);
	waveFormat.cbSize = 0;
	if(result = waveOutOpen(&mWaveHandle, WAVE_MAPPER,
					&waveFormat, (DWORD)SOCallback,
					(DWORD)this, CALLBACK_FUNCTION))
	{
		char str[200], error[235];
		
		if(waveOutGetErrorText(result, str, 200))
			sprintf(error, "Unknown error opening sound output device: %d", result);
		else
			sprintf(error, "Error opening sound output device: %s", str);
		PGFAlert(error, 0);
		mOpen = FALSE;
		return;
	}
	else
	{
		// The sound device has been successfully opened.
		
		waveOutPause(mWaveHandle);
		for(i=0;i<NUMOUTPUTBUFFERS;i++)
		{
			// Setting a maximum buffer length here, we set it on a per-call
			// basis, but for now we assume that 2048 is a maximum, this is
			// required by the driver.
			mHeaders[i].dwBufferLength = 2048;
			mHeaders[i].dwFlags = 0;
			mHeaders[i].dwLoops = 0;
			err = waveOutPrepareHeader(mWaveHandle, &(mHeaders[i]),
										sizeof(WAVEHDR));
			CheckWaveError("waveOutPrepareHeader", err);
		}
		if(mVolumeSupport)
		{
			// get the device ID for volume control and get the current volume
			if(waveOutGetID(mWaveHandle, &did))
			{
				pgp_errstring("Error getting output device ID.");
				return;
			}
			if(waveOutGetVolume(mWaveHandle, &mOriginalVolume))
			{
				pgp_errstring("Error getting original sound output volume.");
				return;
			}
			SetVolume(mVolume);								   
		}
		mOpen = TRUE;
	}
#endif
}

void
CSoundOutput::Close(void)
{
#ifdef	PGP_WIN32
	short err;

	if(mOpen)
	{	
		if(mVolumeSupport && waveOutSetVolume(mWaveHandle, mOriginalVolume))
			pgp_errstring("Error restoring original sound output volume.");
  		for(int i=0;i<NUMOUTPUTBUFFERS;i++)
			waveOutUnprepareHeader(mWaveHandle, &(mHeaders[i]),
									sizeof(WAVEHDR));
		err = waveOutClose(mWaveHandle);
		CheckWaveError("waveOutClose", err);
		mOpen = FALSE;
	}
#endif
}

// GetNumPlaying
//	This routine returns the number of samples waiting to be played.
long
CSoundOutput::GetNumPlaying()
{
	long avail, outdex;
	
	outdex = mOutdex;
	if(mIndex > outdex)
		avail = mIndex - outdex;
	else if(mIndex < outdex)
		avail = mSampleBufLen - outdex + mIndex;
	else
		avail = 0;
	if(mSampleSize == 16)
		avail /= 2;
	return avail;
}

void
CSoundOutput::SetJitter(long jitter)
{
	long	coderRate,
			samplesPerMillisecond,
			jitterSamples,
			sampleQuant;
	
#ifdef	PGP_MACINTOSH
	coderRate = mCoderRate >> 16;
#else
	coderRate = mCoderRate;
#endif
	//mFinishTime				= finishTime;
	samplesPerMillisecond	= (coderRate / 1000) + 1;
	jitterSamples			= samplesPerMillisecond * jitter;
	sampleQuant				= mBufferQuant / (mSampleSize == 16 ? 2 : 1);
	mPlaypoint				= jitterSamples / sampleQuant;
	if(jitterSamples % sampleQuant)
		mPlaypoint ++;
	if(mPlaypoint <= 2)
		mPlaypoint = 2;
	else
		mPlaypoint ++;
	if(mPlaypoint > 20)
		mPlaypoint = 20;
	//CStatusPane::GetStatusPane()->AddStatus(0, "PP: %ld", mPlaypoint);
}

void
CSoundOutput::DisposeCodec(void)
{
	switch(mCodec)
	{
		case 'GS4L':
		case 'GS5L':
		case 'GS6L':
		case 'GS7L':
		case 'GL80':
		case 'GS8L':
		case 'GS1L':
		case 'GSM4':
		case 'GSM5':
		case 'GSM6':
		case 'GSM7':
		case 'GS80':
		case 'GSM8':
		case 'GSM1':
			gsm_destroy(mGSM);
			break;
#ifdef USEVOXWARE
		case 'VOXW':{
			VOXWARE_RETCODE voxerr;
			
			voxerr = RTFreeDecoder(&mVoxComVars);	pgpAssertNoErr(voxerr);
			voxerr = RTFreeEncoder(&mtVoxComVars);	pgpAssertNoErr(voxerr);
			break;}
#endif
		case 0:
			break;
	}
}

void
CSoundOutput::SetCodec(ulong type)
{
	short inx;
	
	mSoundMutex.Wait();
	DisposeCodec();
	mCoderHeaderSize = 0;
	switch(type)
	{
		case 'GS4L':
		case 'GS5L':
		case 'GS6L':
		case 'GS7L':
		case 'GL80':
		case 'GS8L':
		case 'GS1L':			// 160 shorts -> 29 bytes
			mGSM = gsm_create();
			mCFrameSize = 29;
			mFrameSamples = 160;
			mSampleSize = 16;
			mBufferQuant = 320;
			switch(type)
			{
				case 'GS4L':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x113A0000;
					#elif	PGP_WIN32
						mCoderRate = 4410.0;
					#endif
					break;
				case 'GS5L':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x15880000;
					#elif	PGP_WIN32
						mCoderRate = 5512.0;
					#endif
					break;
				case 'GS6L':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x17700000;
					#elif	PGP_WIN32
						mCoderRate = 6000.0;
					#endif
					break;
				case 'GS7L':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1cb60000;
					#elif	PGP_WIN32
						mCoderRate = 7350.0;
					#endif
					break;
				case 'GL80':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1f400000;
					#elif	PGP_WIN32
						mCoderRate = 8000.0;
					#endif
					break;
				case 'GS8L':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x22740000;
					#elif	PGP_WIN32
						mCoderRate = 8820.0;
					#endif
					break;
				case 'GS1L':
					#ifdef	PGP_MACINTOSH
						mCoderRate = rate11025hz;
					#elif	PGP_WIN32
						mCoderRate = 11025.0;
					#endif
					break;
			}
			inx = 1;
			gsm_option(mGSM, GSM_OPT_LTP_CUT, &inx);
			break;
		case 'GSM4':
		case 'GSM5':
		case 'GSM6':
		case 'GSM7':
		case 'GS80':
		case 'GSM8':
		case 'GSM1':			// 160 shorts -> 33 bytes
			mGSM = gsm_create();
			mCFrameSize = 33;
			mFrameSamples = 160;
			mSampleSize = 16;
			mBufferQuant = 320;
			switch(type)
			{
				case 'GSM4':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x113A0000;
					#elif	PGP_WIN32
						mCoderRate = 4410.0;
					#endif
					break;
				case 'GSM5':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x15880000;
					#elif	PGP_WIN32
						mCoderRate = 5512.0;
					#endif
					break;
				case 'GSM6':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x17700000;
					#elif	PGP_WIN32
						mCoderRate = 6000.0;
					#endif
					break;
				case 'GSM7':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1cb60000;
					#elif	PGP_WIN32
						mCoderRate = 7350.0;
					#endif
					break;
				case 'GS80':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1f400000;
					#elif	PGP_WIN32
						mCoderRate = 8000.0;
					#endif
					break;
				case 'GSM8':
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x22740000;
					#elif	PGP_WIN32
						mCoderRate = 8820.0;
					#endif
					break;
				case 'GSM1':
					#ifdef	PGP_MACINTOSH
						mCoderRate = rate11025hz;
					#elif	PGP_WIN32
						mCoderRate = 11025.0;
					#endif
					break;
			}
			break;
		case 'ADP8':
			memset(&mADPCM,0,sizeof(adpcm_state));
			mCFrameSize = 80;
			mFrameSamples = 160;
			mSampleSize = 16;
			mBufferQuant = 320;
			#ifdef	PGP_MACINTOSH
				mCoderRate = 0x1f400000;
			#elif	PGP_WIN32
				mCoderRate = 8000.0;
			#endif
			mCoderHeaderSize = 3;
			break;
#ifdef USEVOXWARE
		case 'VOXW':{
			VOXWARE_RETCODE voxerr;
			ushort framesPerPkt;
			
			#ifdef	PGP_MACINTOSH
				mCoderRate = 0x1f400000;
			#elif	PGP_WIN32
				mCoderRate = 8000.0;
			#endif
			mCFrameSize = 30;
			mSampleSize = 16;
			
			voxerr = RTInitEncoder(&mtVoxComVars, &mtVoxCoderReqs);	pgpAssert(IsntErr(voxerr));
			framesPerPkt = ((mCFrameSize * 8) / mtVoxCoderReqs.wNumBitsPerFrame);
			mtVoxComVars.dwNumBytesInSpeechBuffer =
				framesPerPkt * (mtVoxCoderReqs.wNumSamplesPerFrame * mtVoxCoderReqs.wBytesPerSample);
			mtVoxComVars.dwNumBytesInCodedDataBuffer =
				((framesPerPkt * mtVoxCoderReqs.wNumBitsPerFrame) / 8);
			mtVoxComVars.dwNumBytesInCodedDataBuffer +=
				(0 != ((framesPerPkt * mtVoxCoderReqs.wNumBitsPerFrame) % 8));
			mFrameSamples = mtVoxComVars.dwNumBytesInSpeechBuffer / 2;
			mCFrameSize = mtVoxComVars.dwNumBytesInCodedDataBuffer;
			mVoxComVars.lpVoxwareHeader = &(mtVoxComVars.lpVoxStreamHeader->voxStreamHeader);
			voxerr = RTInitDecoder(&mVoxComVars, &mVoxCoderReqs);	pgpAssert(IsntErr(voxerr));
			mVoxComVars.dwNumBytesInCodedDataBuffer = mtVoxComVars.dwNumBytesInCodedDataBuffer;
			mVoxComVars.dwNumBytesInSpeechBuffer = mtVoxComVars.dwNumBytesInSpeechBuffer;
			mBufferQuant = mtVoxComVars.dwNumBytesInSpeechBuffer;
			break;}
#endif
		default:
			pgp_errstring("Bad codec");
			break;
	}
	mSampleBufLen = SAMPLEBUFMAX / mBufferQuant * mBufferQuant;
	mCodec = type;
	mPFWindow->SetDecoder(type);
#ifdef	PGP_MACINTOSH
	SndCommand sndCmd;

	sndCmd.cmd = flushCmd;
	sndCmd.param1 = 0;
	sndCmd.param2 = 0;
	SndDoImmediate(mSndChannel, &sndCmd);
	sndCmd.cmd = quietCmd;
	sndCmd.param1 = 0;
	sndCmd.param2 = 0;
	SndDoImmediate(mSndChannel, &sndCmd);
#endif
	mPlaying = FALSE;
	mSoundMutex.Signal();
}

void
CSoundOutput::ChannelSwitch()
{
	mSoundMutex.Wait();
#ifdef	PGP_MACINTOSH
	SndCommand sndCmd;

	sndCmd.cmd = flushCmd;
	sndCmd.param1 = 0;
	sndCmd.param2 = 0;
	SndDoImmediate(mSndChannel, &sndCmd);		/* flush the queue */
	sndCmd.cmd = quietCmd;
	sndCmd.param1 = 0;
	sndCmd.param2 = 0;
	SndDoImmediate(mSndChannel, &sndCmd);		/* stop the sound playing */
	mPlaying = FALSE;
	sndCmd.cmd = freqDurationCmd;
	sndCmd.param1 = 75;							/* play for 150 milliseconds */
	sndCmd.param2 = 85;							/* octave 8, C# */
	SndDoCommand(mBeepChannel, &sndCmd, TRUE);
	sndCmd.cmd = quietCmd;
	sndCmd.param1 = 0;
	sndCmd.param2 = 0;
	SndDoCommand(mBeepChannel, &sndCmd, TRUE);	/* make sure the beep stops */
#elif	PGP_WIN32
	Open();
	if(mOpen)
	{
		short err;

		err = waveOutRestart(mWaveHandle);	CheckWaveError("waveOutReset", err);
		InterlockedDecrement(&sBufReturn);
		mToneHeader.lpData = (char *)(mHardwareIs16Bit ? mToneBuffers[0] : mToneBuffers[1]);
		mToneHeader.dwBufferLength = mHardwareIs16Bit ? TONELENGTH*2 : TONELENGTH;
		mToneHeader.dwFlags = 0;
		mToneHeader.dwLoops = 0;
		err = waveOutPrepareHeader(mWaveHandle, &mToneHeader, sizeof(WAVEHDR));
		CheckWaveError("waveOutPrepareHeader", err);
		if(!(err = waveOutWrite(mWaveHandle, &mToneHeader, sizeof(WAVEHDR))))
		{
			while(!(mToneHeader.dwFlags & WHDR_DONE))
				Sleep(0);
			err = waveOutUnprepareHeader(mWaveHandle, &mToneHeader, sizeof(WAVEHDR));
			CheckWaveError("waveOutUnprepareHeader", err);
		}
		else
			CheckWaveError("waveOutWrite", err);

		Close();
	}
#endif
	mSoundMutex.Signal();
}

void
CSoundOutput::SetVolume(long volume)
{
#ifdef	PGP_MACINTOSH
	SndCommand sndCmd;
	long newVolume;

	newVolume = (volume << 16) | volume;
	if(newVolume != mVolume)
	{
		mVolume = newVolume;
		sndCmd.cmd = volumeCmd;
		sndCmd.param1 = 0;
		sndCmd.param2 = mVolume;
		SndDoImmediate(mSndChannel, &sndCmd);
	}
#elif	PGP_WIN32
	mVolume = volume;
	if(!mOpen)
		return;
	DWORD tvol = (DWORD)(volume << 16 | volume);
	waveOutSetVolume(mWaveHandle, tvol);
#endif
}

// ControlThread calls the following function to inform SoundOutput whether it
// should be playing packets or discarding them

void
CSoundOutput::Play(Boolean play)
{
	mSoundMutex.Wait();
#ifdef	PGP_WIN32
	short err;
	
	if(play && !mPlay)
	{
		Open();
		err = waveOutRestart(mWaveHandle);	CheckWaveError("waveOutRestart", err);
	}
	else if(!play && mPlay)
	{
		err = waveOutReset(mWaveHandle);	CheckWaveError("waveOutReset", err);
		Close();
		// refill the semaphore
		while(sBufReturn < NUMOUTPUTBUFFERS)
			InterlockedIncrement(&sBufReturn);
	}
#endif
	mPlay = play;
	mIndex = mOutdex = 0;
	mSoundMutex.Signal();
}

// Called to halt playing of sound packets temporarily

void
CSoundOutput::Pause(Boolean pause)
{
	mSoundMutex.Wait();
#ifdef	PGP_WIN32
	if(mOpen)
	{
		short err;
		
		if(mPlay && pause)
		{
			// stop playing blocks
			err = waveOutReset(mWaveHandle);	CheckWaveError("waveOutReset", err);
			// refill the semaphore
			while(sBufReturn < NUMOUTPUTBUFFERS)
				InterlockedIncrement(&sBufReturn);
		}
		else if(!mPlay && pause)
		{
			// start playing blocks
			err = waveOutRestart(mWaveHandle);	CheckWaveError("waveOutRestart", err);
		}
	}
#endif
	mPlayQueue->FreeAll();
	if(mOpen)
		mPlay = !pause;
	mIndex = mOutdex = 0;
	mSoundMutex.Signal();
}

void *
CSoundOutput::Run(void)
{
	PFMessage *msg;
	uchar *buf, *d, frames;
	short done = 0, err;
	long len, processed, avail, outdex, overrunDivisor;
	
	while(!done)
	{
		if(msg = (PFMessage *)mPlayQueue->Recv())
		{
			switch(msg->type)
			{
				case _mt_voicePacket:
					mSoundMutex.Wait();
					if(mOpen && mPlay)
					{
						/*if(mSendRR)
						{
							uchar *p, *e;
							
							p = e = (uchar *)safe_malloc(1);
							*e++ = 0;
							mControlThread->GetOutQueue()->Send(_mt_rr, p, 1, TRUE);
							mSendRR = FALSE;
						}*/
						outdex = mOutdex;
						if(mIndex > outdex)
							avail = mIndex - outdex;
						else if(mIndex < outdex)
							avail = mSampleBufLen - outdex + mIndex;
						else
							avail = 0;
						if(mPlaypoint <= 2)
							overrunDivisor = 5;
						else if(mPlaypoint <= 4)
							overrunDivisor = 4;
						else if(mPlaypoint <= 6)
							overrunDivisor = 3;
						else if(mPlaypoint <= 10)
							overrunDivisor = 2;
						else
							overrunDivisor = 1;
#ifdef	PGP_MACINTOSH
						if((avail / (mSampleSize == 16 ? 2 : 1)) > (mCoderRate >> 16) / overrunDivisor)
#else
						if((avail / (mSampleSize == 16 ? 2 : 1)) > mCoderRate / overrunDivisor)
#endif
						{
							// If the number of samples waiting to play is greater than
							// one fifth the sample rate, drop old stuff to reduce latency
							mIndex = mOutdex;
						}
						if(msg->len > 5)
						{
							d = (uchar *)msg->data;
							frames = msg->len / mCFrameSize;
							len = (frames * mFrameSamples) * (mSampleSize == 16 ? 2 : 1);
							processed = 0;
							while(processed<len)
							{
								buf = &mSamples[mIndex];
								switch(mCodec)
								{
									case 'GS4L':
									case 'GS5L':
									case 'GS6L':
									case 'GS7L':
									case 'GL80':
									case 'GS8L':
									case 'GS1L':
									case 'GSM4':
									case 'GSM5':
									case 'GSM6':
									case 'GSM7':
									case 'GS80':
									case 'GSM8':
									case 'GSM1':
										if(gsm_decode(mGSM, d, (short *)buf))
										{
											//GSM buffer was bad!
											
											//Let the upper layer know about this because if we
											//are on a serial connection this could mean
											//that the line has disconnected unexpectedly and
											//we should hangup.
											//We only do this only if processed is 0 because it is
											//theoretically possible to receive sound encoded by
											//another GSM which would have a different CFrameSize,
											//causing it to falsely invoke on the second GSM frame.
											
											if(!processed)
												mControlThread->ProtocolViolation(_pv_invalidSound);
										}
										d+=mCFrameSize;
										processed+=mBufferQuant;
										break;
									case 'ADP8':
										if(!processed)
										{
											BUFFER_TO_SHORT(d, mADPCM.valprev);	d += sizeof(short);
											mADPCM.index = *d++;
										}
										adpcm_decoder((char *)d, (short *)buf, mCFrameSize*2, &mADPCM);
										d+=mCFrameSize;
										processed+=320;
										break;
#ifdef USEVOXWARE
									case 'VOXW':{
										VOXWARE_RETCODE voxerr;
										
										mVoxComVars.lpCodedDataBuffer = d;
										mVoxComVars.lpSpeechBuffer = buf;
										voxerr = RTDecode(&mVoxComVars);	pgpAssert(IsntErr(voxerr));
										
										d+=mCFrameSize;
										processed+=mBufferQuant;
										break;}
#endif
									case 0:
										break;
								}
								mIndex+=mBufferQuant;
								mIndex %= mSampleBufLen;
								// how much sound is available?
								outdex = mOutdex;
								if(mIndex > outdex)
									avail = mIndex - outdex;
								else if(mIndex < outdex)
									avail = mSampleBufLen - outdex + mIndex;
								else
									avail = 0;
#ifdef	PGP_MACINTOSH
								if(!mPlaying && (avail >= mBufferQuant * mPlaypoint))// are we starting up/underflowing?
								{
									SndCommand sndCmd;
									
									// there is enough sound to start up a double
									// buffer
									sndCmd.cmd = flushCmd;
									sndCmd.param1 = 0;
									sndCmd.param2 = 0;
									// cancel anything old
									SndDoImmediate(mSndChannel, &sndCmd);
									mDoubleBufHdr.dbhNumChannels = 1;
									mDoubleBufHdr.dbhSampleSize = mSampleSize;
									mDoubleBufHdr.dbhCompressionID = 0;
									mDoubleBufHdr.dbhPacketSize = 0;
									mDoubleBufHdr.dbhSampleRate = mCoderRate;
									mDoubleBufHdr.dbhDoubleBack = mDoubleBackProc;
									mDoubleBufHdr.dbhBufferPtr[0] = mDblBufOne;
									mDoubleBufHdr.dbhBufferPtr[1] = mDblBufTwo;
									mDblBufOne->dbNumFrames = mDblBufTwo->dbNumFrames =
										mBufferQuant / (mSampleSize == 16 ? 2 : 1);
									mPlaying = TRUE;
									FillDoubleBuffer(mDblBufOne);
									FillDoubleBuffer(mDblBufTwo);
									mDblBufOne->dbUserInfo[0] = mDblBufTwo->dbUserInfo[0] =
										(long)this;
									err = SndPlayDoubleBuffer(mSndChannel, &mDoubleBufHdr);
									pgpAssert(IsntErr(err));
								}
#elif	PGP_WIN32
								if(avail >= mBufferQuant * mPlaypoint)
									while((avail >= mBufferQuant) && (sBufReturn>0))
									{
										// send the buffer to the sound card
										InterlockedDecrement(&sBufReturn);
										mHeaders[mHeaderIndex].dwBufferLength = RateChange((short *)&mSamples[mOutdex],
													(short *)mHeaders[mHeaderIndex].lpData,
													mBufferQuant / 2, mCoderRate, mSampleRate) * 2;
										mOutdex += mBufferQuant;
										mOutdex %= mSampleBufLen;
										err = waveOutWrite(mWaveHandle, &(mHeaders[mHeaderIndex]), sizeof(WAVEHDR));
										CheckWaveError("waveOutWrite", err);
										mHeaderIndex++;
										mHeaderIndex %= NUMOUTPUTBUFFERS;
										avail -= mBufferQuant;
									}
#endif
							}
						}
						else
						{
							//	fill buf with silence data
							//	we have received a "silence" packet which
							//	is a way of saving bandwidth during silence
							//	first byte is # of frames, next 4 are random
							//+++++todo
						}
						Yield();
					}
					else
					{
						/* we are not playing sound now, so discard this packet */
					}
					mSoundMutex.Signal();
					break;
				case _mt_quit:
					done = 1;
					break;
			}
			mPlayQueue->Free(msg);
		}
	}
	return (void *)1L;
}

