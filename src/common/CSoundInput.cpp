/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CSoundInput.cpp,v 1.5 1999/03/10 02:39:57 heller Exp $
____________________________________________________________________________*/
#include "CSoundInput.h"

#include "CPFWindow.h"
#include "CStatusPane.h"
#include "CLevelMeter.h"
#include "CPFPackets.h"
#include "CMessageQueue.h"
#include "CPacketWatch.h"
#include "samplerate.h"
#include "fastpool.h"
#include <string.h>

#ifdef PGP_MACINTOSH
#include "MacDebug.h"
#endif

#ifdef USEVOXWARE
#include "tvgetstr.h"
#endif


Boolean gHardwareIsFullDuplex;


#ifdef	PGP_MACINTOSH
#include <Power.h>
#include "PGPFMacUtils.h"

static void SoundReady(SPBPtr spb, char *buf, long len, short /*amplitude*/)
{
	CSoundInput *si;
	long f;

	si=(CSoundInput *)spb->userLong;
	if((f = si->mSampleBufSize - si->mIndex) < len)
	{
		::BlockMoveData(buf, &si->mSamples[si->mIndex], f);
		::BlockMoveData(buf + f, &si->mSamples[0], len - f);
		 si->mIndex = len - f;
	}
	else
	{
		::BlockMoveData(buf, &si->mSamples[si->mIndex], len);
		 si->mIndex += len;
	}
	si->mIndex %=  si->mSampleBufSize;
}

#ifdef	__MC68K__
static asm void SIInterruptProc()
{
	fralloc		+
	movem.l		a0-a2/d0-d2,-(a7)
	move.w		d0, -(a7)
	move.l		d1, -(a7)
	move.l		a1, -(a7)
	move.l		a0, -(a7)
	jsr			SoundReady
	movem.l		(a7)+,a0-a2/d0-d2
	frfree
	rts
}
#else	//__MC68K__
static void SIInterruptProc(SPBPtr spb, Ptr dataBuffer,
							short peakAmplitude, long samplesLen)
{
	SoundReady(spb, dataBuffer, samplesLen, peakAmplitude);
}
#endif	//__MC68K__
#endif	//PGP_MACINTOSH

#ifdef	PGP_WIN32

HANDLE	siSemaphore;
static long bufReturn, sHeaderIndex;
static short sLevel;

//	SoundReady - the callback routine specified when we open the sound
//	device for input.  Whenever Win32 finishes recording a
//	block of sound it calls this routine.

void CALLBACK
SoundReady(HANDLE hWaveIn, UINT msg, DWORD instance, DWORD param1, DWORD param2)
{
	CSoundInput *si;
	long f, count;
	short tLevel, *p, *e;

	if(msg == WIM_DATA)
	{
		si=(CSoundInput *)instance;
		if((f = si->mSampleBufSize - si->mIndex) < RAWRECORDQUANT)
		{
			memcpy(&si->mSamples[si->mIndex], si->mHeaders[sHeaderIndex].lpData, f);
			memcpy(&si->mSamples[0], si->mHeaders[sHeaderIndex].lpData + f, RAWRECORDQUANT - f);
			si->mIndex = RAWRECORDQUANT - f;
		}
		else
		{
			memcpy(&si->mSamples[si->mIndex], si->mHeaders[sHeaderIndex].lpData, RAWRECORDQUANT);
			si->mIndex += RAWRECORDQUANT;
		}
		// Calculate a level meter position.
		// Look at every fourth sample and take
		// the max sample value to send to
		// the level meter.  This is done for us
		// on the Macintosh.
		tLevel = 0;
		p = (short *)si->mHeaders[sHeaderIndex].lpData;
		e = (short *)(si->mHeaders[sHeaderIndex].lpData + RAWRECORDQUANT);
		for(;p<e;p+=4)	// every fourth sample
		{
			if(*p > tLevel)
				tLevel = *p;
		}
		if(tLevel<256)
			sLevel = 0;
		else
			sLevel = (tLevel / 2048) + 1;	// scale it from 0-16
		if(sLevel > 16)
			sLevel = 16;
		sHeaderIndex++;
		sHeaderIndex %= NUMINPUTBUFFERS;
		si->mIndex %= si->mSampleBufSize;
		InterlockedIncrement(&bufReturn);
		ReleaseSemaphore(siSemaphore, 1, &count);
	}
}

void
CSoundInput::CheckWaveError(char *callName, int errCode)
{
	char s[256], t[256];

	if(errCode)
	{
		if(!waveInGetErrorText(errCode, s, 200))
			sprintf(t, "Error on %s:\n %s", callName, s);
		else
			sprintf(t, "Error on %s:\n %d", callName, errCode);
		PGFAlert(t,0);
		pgpAssert(0);
	}
}

#endif	//PGP_WIN32

void Amplify(short *samples, int numSamples, double gain);

CSoundInput::CSoundInput(CPFWindow *pfWindow, CLevelMeter *levelMeter, void **outResult)
#ifndef PGP_WIN32
		: LThread(0, thread_DefaultStack, threadOption_UsePool, outResult)
#else
		: LThread(outResult)		
#endif
{
	long gestValue;
	
	mPFWindow = pfWindow;
	mAbort = FALSE;
	mOpen = FALSE;
	mRecording = FALSE;
	mPaused = FALSE;
	mLevelMeter = levelMeter;
	mGSM = NIL;
	mOutQueue = NIL;
	mOutThread = NIL;
	mPacketThread = NIL;
	mIndex = mOutdex = 0;
	mSampleSize = 16;
	mDefaultTrail = mTrailSamples = 0;
	mHardwareIs16Bit = TRUE;
	mCodec = 0;
	mGain = 0;
	SetGain(gPGFOpts.popt.siGain);
	mThreshold = 0;
	mDownDone = mCompDone = 0;
	mCompSnd = NIL;
	gHardwareIsFullDuplex = TRUE;
	
#ifdef PGP_MACINTOSH
	mSoundRef = 0;
	Gestalt(gestaltSoundAttr, &gestValue);
	gHardwareIsFullDuplex = !!(gestValue & (1<<gestaltPlayAndRecord));
	mHardwareIs16Bit = !!(gestValue & (1<<gestalt16BitSoundIO));
	mSIIntProc = NewSIInterruptProc(SIInterruptProc);
	mSPB.completionRoutine = NIL;
	mSPB.interruptRoutine = mSIIntProc;
	mSPB.userLong = (long)this;
	mSPB.unused1 = 0;
	mPMExists = PowerManagerExists();
	if(mPMExists)
		::DisableIdle();// recording sucks with idle mode enabled
#elif PGP_WIN32
	WAVEFORMATEX waveFormat;
	WAVEINCAPS devCaps;
	MMRESULT result;
	Boolean flag = TRUE;
	short i;

	memset(mHeaders, 0, sizeof(mHeaders));
	for(i=0;i<NUMINPUTBUFFERS;i++)
	{
		mMemHandles[i] = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,RAWRECORDQUANT);
		mMemPtrs[i] = (uchar*)GlobalLock(mMemHandles[i]);

		// initialize the header
		mHeaders[i].lpData = (char *)mMemPtrs[i];
		mHeaders[i].dwBufferLength = RAWRECORDQUANT;
	}
	bufReturn = 0;
	mHeaderIndex = sHeaderIndex = 0;
	siSemaphore = CreateSemaphore(NULL, 0, NUMINPUTBUFFERS, NULL);
	if(!siSemaphore)
		pgp_errstring("Error creating semaphore.");

	// Now lets test the hardware to see whether we are running on a full duplex
	// machine or a half duplex one.  Lets open sound input and then we'll see if
	// there's an error when we open sound output.  If there is, the hardware is
	// half duplex.

	if(waveInGetNumDevs() < 1)
		PGFAlert("No sound input devices found!", 0);
	waveInGetDevCaps(WAVE_MAPPER, &devCaps, sizeof(WAVEINCAPS));
	if(devCaps.dwFormats & WAVE_FORMAT_1M16)
	{
		mHardwareIs16Bit = TRUE;
		mSampleRate = 11025.0;
	}
	else if(devCaps.dwFormats & WAVE_FORMAT_2M16)
	{
		mHardwareIs16Bit = TRUE;
		mSampleRate = 22050.0;
	}
	else if(devCaps.dwFormats & WAVE_FORMAT_2M08)
	{
		mHardwareIs16Bit = FALSE;
		mSampleRate = 22050.0;
	}
	else if(devCaps.dwFormats & WAVE_FORMAT_1M08)
	{
		mHardwareIs16Bit = FALSE;
		mSampleRate = 11025.0;
	}
	else
	{
		PGFAlert("Sound input device does not support a compatible format.", 0);
		flag = FALSE;
	}
	if(flag)
	{
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels	= 1;
		waveFormat.nSamplesPerSec = (long)mSampleRate;
		waveFormat.nAvgBytesPerSec = (long)mSampleRate * (mHardwareIs16Bit ? 2 : 1);
		waveFormat.nBlockAlign = mHardwareIs16Bit ? 2 : 1;
		waveFormat.wBitsPerSample = 8 * (mHardwareIs16Bit ? 2 : 1);
		waveFormat.cbSize = 0;
		if(!waveInOpen(&mWaveHandle, WAVE_MAPPER, &waveFormat, 
						(DWORD)SoundReady, NIL, CALLBACK_FUNCTION))
		{
			HWAVEOUT mOWaveHandle = NULL;
			float mOSampleRate;
			Boolean mOHardwareIs16Bit;
			WAVEOUTCAPS devCaps;
	
			flag = TRUE;
			if(waveOutGetNumDevs() < 1)
				PGFAlert("No sound output devices available.", 0);
			waveOutGetDevCaps(WAVE_MAPPER, &devCaps, sizeof(WAVEOUTCAPS));
			if(devCaps.dwFormats & WAVE_FORMAT_1M16)
			{
				// We'd rather upsample to 11025 than 22050
				mOHardwareIs16Bit = TRUE;
				mOSampleRate = 11025.0;
			}
			else if(devCaps.dwFormats & WAVE_FORMAT_2M16)
			{
				mOHardwareIs16Bit = TRUE;
				mOSampleRate = 22050.0;
			}
			else if(devCaps.dwFormats & WAVE_FORMAT_1M08)
			{
				mOHardwareIs16Bit = FALSE;
				mOSampleRate = 11025.0;
			}
			else if(devCaps.dwFormats & WAVE_FORMAT_2M08)
			{
				mOHardwareIs16Bit = FALSE;
				mOSampleRate = 22050.0;
			}
			else
			{
				PGFAlert("Sound output device does not support a compatible format.", 0);
				flag = FALSE;
			}
			if(flag)
			{
				MMRESULT err;
				waveFormat.wFormatTag = WAVE_FORMAT_PCM;
				waveFormat.nChannels = 1;
				waveFormat.nSamplesPerSec = (long)mOSampleRate;
				waveFormat.nAvgBytesPerSec = (long)mOSampleRate * (mOHardwareIs16Bit ? 2 : 1);
				waveFormat.nBlockAlign = mOHardwareIs16Bit ? 2 : 1;
				waveFormat.wBitsPerSample = 8 * (mOHardwareIs16Bit ? 2 : 1);
				waveFormat.cbSize = 0;
				err=waveOutOpen(&mOWaveHandle, WAVE_MAPPER,
						&waveFormat,
						(DWORD)SoundReady, NIL, CALLBACK_FUNCTION);
				if(!err || !(err=waveOutOpen(&mOWaveHandle, WAVE_MAPPER,
						&waveFormat,
						(DWORD)SoundReady, NIL, CALLBACK_FUNCTION)))
				{
					gHardwareIsFullDuplex = TRUE;
					waveOutClose(mOWaveHandle);
				}
				//else
				//	CheckWaveError("waveOutOpen", err);
			}
			waveInClose(mWaveHandle);
		}
	}
#endif
	SetCodec('GSM7');	// just setting a default coder for testing only
}

CSoundInput::~CSoundInput()
{
	Close();
	DisposeCodec();
	if(mCompSnd)
		safe_free(mCompSnd);
#ifdef	PGP_MACINTOSH
	DisposeRoutineDescriptor(mSIIntProc);
	if(mPMExists)
		::EnableIdle();
#elif	PGP_WIN32
	for(short i=0;i<NUMINPUTBUFFERS;i++)
	{
		GlobalUnlock(mMemHandles[i]);
		GlobalFree(mMemHandles[i]);
	}
	if(siSemaphore)
		CloseHandle(siSemaphore);
#endif
}

// Open is called when a call begins upon connection before configuration,
// full duplex CPUs are always open so that the level meter can be always on

void
CSoundInput::Open(void)
{
#ifdef PGP_MACINTOSH
	OSErr result;
	OSType comp;
	Fixed rate;
	short lm;

	if(!mOpen)
	{
		if(result = ::SPBOpenDevice(NIL, siWritePermission, &mSoundRef))
			PGFAlert("Sound input device is already open!  Recording disabled.", 0);
		else
		{
			mSPB.inRefNum = mSoundRef;
			//SPBSetDeviceInfo(mSoundRef, siOptionsDialog, NIL);
			rate = rate22050hz;
			::SPBSetDeviceInfo(mSoundRef, siSampleRate, &rate);
			::SPBGetDeviceInfo(mSoundRef, siSampleRate, &rate);
			mSampleRate = rate;
			result = ::SPBSetDeviceInfo(mSoundRef, siInputGain, &mGain);
			//pgpAssert(!result);
			comp = 'NONE';
			result = ::SPBSetDeviceInfo(mSoundRef, siCompressionType, &comp);
			pgpAssert(IsntErr(result));
			lm = 1;
			result = ::SPBSetDeviceInfo(mSoundRef, siNumberChannels, &lm);
			pgpAssert(IsntErr(result));
			lm = 1;
			result = ::SPBSetDeviceInfo(mSoundRef, siLevelMeterOnOff, &lm);
			pgpAssert(IsntErr(result));
			if(mHardwareIs16Bit)
			{
				result = ::SPBSetDeviceInfo(mSoundRef, siSampleSize, &mSampleSize);
				if(result)
					mHardwareIs16Bit = false;
				//pgpAssert(IsntErr(result));
			}
			mOpen = TRUE;
		}
	}
#elif	PGP_WIN32
	WAVEFORMATEX waveFormat;
	WAVEINCAPS devCaps;
	MMRESULT result;

	if(!mOpen)
	{
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels	= 1;
		waveFormat.nSamplesPerSec = (long)mSampleRate;
		waveFormat.nAvgBytesPerSec = (long)mSampleRate * (mHardwareIs16Bit ? 2 : 1);
		waveFormat.nBlockAlign = mHardwareIs16Bit ? 2 : 1;
		waveFormat.wBitsPerSample = 8 * (mHardwareIs16Bit ? 2 : 1);
		waveFormat.cbSize = 0;
		if(result = waveInOpen(&mWaveHandle, WAVE_MAPPER, &waveFormat, 
						(DWORD)SoundReady, (DWORD)this, CALLBACK_FUNCTION))
		{
			char str[200], error[220];

			if(!waveInGetErrorText(result, str, 200))
			{
				if(!str[0])
					sprintf(error, "Unknown error opening sound input device: %d", result);
				else
					sprintf(error, "Error opening sound input device:\n %s", str);
			}
			else
				sprintf(error, "Error opening sound input device:\n %s", str);
			PGFAlert(error, 0);
			mOpen = FALSE;
		}
		else
			mOpen = TRUE;
	}
#endif
}

// Close is called when a call is terminated, or when the program is exited
// on a full duplex CPU

void
CSoundInput::Close(void)
{
	if(mOpen)
	{
		mLevelMeter->SetLevel(0);
		if(mRecording)
			Record(FALSE);
		mOpen = FALSE;
		mPaused = FALSE;
		
#ifdef	PGP_MACINTOSH
		::SPBCloseDevice(mSoundRef);
#elif	PGP_WIN32
		short err;
		
		err = waveInReset(mWaveHandle);	CheckWaveError("waveInReset", err);
		for(short i=0;i<NUMINPUTBUFFERS;i++)
		{
			while(mHeaders[i].dwFlags && !(mHeaders[i].dwFlags & WHDR_DONE))
				Sleep(0);
			err = waveInUnprepareHeader(mWaveHandle, &(mHeaders[i]), sizeof(WAVEHDR));
			CheckWaveError("waveInUnprepareHeader", err);
		}
		err = waveInClose(mWaveHandle);	CheckWaveError("waveInClose", err);
		mWaveHandle = NIL;
#endif	//PGP_WIN32
	}
}

void *
CSoundInput::Run(void)
{
	long levelInfo, newlen, index, downSize;
	short *src, *s, *p, *e;
	uchar *d, *de;
	Boolean passedThreshold;
	
	while(!mAbort)
	{
#ifdef	PGP_WIN32
		int err;
		WaitForSingleObject(siSemaphore, INFINITE);
		if(mAbort)
			break;
		while(bufReturn>0)
		{
			mSoundMutex.Wait();
			if(mOpen && mRecording)
			{
				if(!(mHeaders[mHeaderIndex].dwFlags & WHDR_PREPARED))
				{
					err = waveInPrepareHeader(mWaveHandle, &(mHeaders[mHeaderIndex]), sizeof(WAVEHDR));
					CheckWaveError("waveInPrepareHeader", err);
				}
				err = waveInAddBuffer(mWaveHandle, &(mHeaders[mHeaderIndex]), sizeof(WAVEHDR));
				CheckWaveError("waveInAddBuffer", err);
			}
			mHeaderIndex++;
			mHeaderIndex %= NUMINPUTBUFFERS;
			mSoundMutex.Signal();
			InterlockedDecrement(&bufReturn);
		}
#endif
		mSoundMutex.Wait();
#ifdef	PGP_WIN32
		if(mOpen)
			mLevelMeter->SetLevel(sLevel);
#endif
		index = mIndex;
		if(mOpen && mRecording && mOutdex != index)
		{
			if(index < mOutdex)
				newlen = mSampleBufSize - mOutdex;
			else
				newlen = index - mOutdex;
			newlen = minl(DOWNBUFSIZE - mDownDone, newlen);
			if(!mPaused && mOutQueue && mCodec)
			{
				src = (short *)&mSamples[mOutdex];
				if(mCodec == 'rand')
					randPoolAddBytes((uchar *)src, newlen);
				else
				{
					switch(mSampleSize)
					{
						case 16:
							if(mUpSampleSize)
							{
								char *p, *e;

								p = (char *)src;
								e = p + newlen;
								if(e-p > UPSAMPBUFSIZE/2)
									newlen = UPSAMPBUFSIZE/2;
								src = s = (short *)mUpSampleBuf;
								for(;p<e;p++)
									*s++ = (*p - 0x80) * 0x100;
								downSize = RateChange(src, (short *)(mDownBuf + mDownDone),
													newlen, mSampleRate, mCoderRate);
							}
							else
								downSize = RateChange(src, (short *)(mDownBuf + mDownDone),
													newlen / 2, mSampleRate, mCoderRate);
#ifdef PGP_WIN32
							// Amplify the sound samples based on the user's setting
							if(mGain)
								Amplify((short *)(mDownBuf + mDownDone), downSize * 2, mGain);
#endif
							mDownDone += downSize * 2;

							// Handle silence detection
							if(mThreshold)
							{
								// If mThreshold is 0, silence detection is deactivated
								
								if(mTrailSamples > 0)
								{
									// mTrailSamples allows us to have a number of
									// samples guaranteed to be sent any time we
									// go above the minimum amplitude.
									// This will probably trail a bit more than
									// its value because mDownDone may be greater
									// than mTrailSamples, but thats okay.
									
									mTrailSamples -= mDownDone;
									if(mTrailSamples < 0)
										mTrailSamples = 0;
								}
								else
								{
									// This is the meat of the silence detection.
									// See whether every 2nd sample is above or below
									// 0 and then measure its amplitude difference from
									// our threshold.
									
									passedThreshold = FALSE;
									p = (short *)mDownBuf;
									e = (short *)(mDownBuf + mDownDone);
									for(;!passedThreshold && (p<e);p+=2)
									{
										if(*p < 0)
										{
											if(*p <= -mThreshold)
											{
												passedThreshold = TRUE;
												break;
											}
										}
										else
										{
											if(*p >= mThreshold)
											{
												passedThreshold = TRUE;
												break;
											}
										}
									}
									if(!passedThreshold)
									{
										// The sound did not pass the amplitude test,
										// ditch it all so it never gets sent.
										
										mDownDone = 0;
										SetStatus(FALSE);
									}
									else
									{
										mTrailSamples = mDefaultTrail;
										SetStatus(TRUE);
									}
								}
							}
							break;
						case 8:
							break;
							/*{//	mSampleSize == 8
							char *p, *s, *e;
							
							downSize = newlen / mDownsampleD;
							p = (char *)src;
							e = p + newlen / mDownsampleD * mDownsampleD;
							s = mDownBuf + mDownDone;
							for(;p<e;p++)
							{
								if(!toggle)
									*s++ = *p;
								if(++toggle == mDownsampleD)
									toggle=0;
							}
							mDownDone += downSize;
							break;}*/
					}
					if(mDownDone >= mFrameSize)
					{
						if(!mCompSnd)
						{
							mCompSnd = safe_malloc(mCFrameSize * mFrames + mCoderSizeSlack);
							mCompDone = 0;
							// Record the sampling instant for RTP
						}
						s = (short *)mDownBuf;
						e = (short *)(mDownBuf + mDownDone);
						d = (uchar *)mCompSnd + mCompDone + (mCompDone ? mCoderSizeSlack : 0);
						de = (uchar *)mCompSnd + (mCFrameSize * mFrames) + mCoderSizeSlack;
						switch(mCodec)
						{
							case 'GS4L':
							case 'GS7L':
							case 'GS6L':
							case 'GL80':
							case 'GS1L':
							case 'GSM4':
							case 'GSM7':
							case 'GSM6':
							case 'GS80':
							case 'GSM1':
								for(;(d < de) && (e-s >= 160);s+=160,d+=mCFrameSize)
								{
									gsm_encode(mGSM, s, d);
									mCompDone += mCFrameSize;
								}
								pgp_memcpy(mDownBuf, s, (char *)e-(char *)s);
								mDownDone = (char *)e-(char *)s;
								break;
							case 'ADP8':
								if(!mCompDone)
								{
									SHORT_TO_BUFFER(mADPCM.valprev, d);	d += sizeof(short);
									*d++ = mADPCM.index;
								}
								for(;(d < de) && (e-s >= 160);s+=160,d+=mCFrameSize)
								{
									adpcm_coder(s,(char *)d,mFrameSize/2,&mADPCM);
									mCompDone += mCFrameSize;
								}
								pgp_memcpy(mDownBuf, s, (char *)e-(char *)s);
								mDownDone = (char *)e-(char *)s;
								break;
#ifdef USEVOXWARE
							case 'VOXW':{
								VOXWARE_RETCODE voxerr;
								
								mVoxComVars.lpSpeechBuffer = (uchar *)s;
								mVoxComVars.lpCodedDataBuffer = d;
								voxerr = RTEncode(&mVoxComVars);	pgpAssert(IsntErr(voxerr));
								d += mCFrameSize;
								mCompDone += mCFrameSize;
								s += mFrameSize / 2;
								pgp_memcpy(mDownBuf, s, (char *)e-(char *)s);
								mDownDone = (char *)e-(char *)s;
								break;}
#endif
							case 0:
								break;
						}
						if(d >= de)
						{
							pgpAssert(d == de);	//make sure we didn't go over
							if(mOutQueue->GetSize() < SOUNDINPUTQUEUEMAX)
								mOutQueue->Send(_mt_voicePacket, mCompSnd,
											mFrames * mCFrameSize + mCoderSizeSlack, 1);
							else
							{
								safe_free(mCompSnd);
								gPacketWatch->OverflowOut(1);
							}
							mCompSnd = NIL;
							mCompDone = 0;
						}
					}
				}
			}
			mOutdex += newlen;
			mOutdex %= mSampleBufSize;
		}
#ifdef	PGP_MACINTOSH
		if(mOpen)
		{
			::SPBGetDeviceInfo(mSoundRef, siLevelMeterOnOff, &levelInfo);
			mLevelMeter->SetLevel(levelInfo & 0xFFFF);
		}
#endif
		mSoundMutex.Signal();
#ifdef	PGP_MACINTOSH
		Yield(mOutThread);
#elif	PGP_WIN32
		Yield();
#endif
	}
	return (void *)1L;
}

void
CSoundInput::SetGain(short gain)
{
#ifdef	PGP_MACINTOSH
	ulong step = 0x00010000;
	
	mGain = 0x00007FFF + (step / 256 * gain);
	if(mOpen)
		::SPBSetDeviceInfo(mSoundRef, siInputGain, &mGain);
#elif	PGP_WIN32
	mGain = gain;
#endif	//PGP_WIN32
}

void
CSoundInput::DisposeCodec(void)
{
	mSoundMutex.Wait();
	switch(mCodec)
	{
		case 'GSM4':
		case 'GSM6':
		case 'GSM7':
		case 'GS80':
		case 'GSM1':
		case 'GS4L':
		case 'GS6L':
		case 'GS7L':
		case 'GL80':
		case 'GS1L':
			gsm_destroy(mGSM);
			break;
#ifdef USEVOXWARE
		case 'VOXW':{
			VOXWARE_RETCODE voxerr;
			
			voxerr = RTFreeEncoder(&mVoxComVars);	pgpAssert(IsntErr(voxerr));
			break;}
#endif
		case 'ADP8':
		case 0:
			break;
	}
	mCodec = 0;
	mSoundMutex.Signal();
}

void
CSoundInput::SetCodec(ulong type)
{
	short inx;
	
	mSoundMutex.Wait();
	DisposeCodec();
	mIndex = mOutdex = 0;
	mCoderSizeSlack = 0;
	switch(type)
	{
#ifdef USEVOXWARE
		case 'VOXW':{
			VOXWARE_RETCODE voxerr;
			ushort framesPerPkt;
			
			#ifdef	PGP_MACINTOSH
				mCoderRate = 0x1f400000;
			#elif	PGP_WIN32
				mCoderRate = 8000.0;
			#endif
			mSampleSize = 16;
			mCFrameSize = 30;	// We use less than 30 byte packets with Voxware!!
			mFrames = 1;
			voxerr = RTInitEncoder(&mVoxComVars, &mVoxCoderReqs);	pgpAssert(IsntErr(voxerr));
			if((mVoxCoderReqs.dwSamplingRate == 8000) &&
				(mVoxCoderReqs.wBytesPerSample == 2))
			{
				framesPerPkt = ((mCFrameSize * 8) / mVoxCoderReqs.wNumBitsPerFrame);
				mVoxComVars.dwNumBytesInSpeechBuffer =
					framesPerPkt * (mVoxCoderReqs.wNumSamplesPerFrame * mVoxCoderReqs.wBytesPerSample);
				mVoxComVars.dwNumBytesInCodedDataBuffer =
					((framesPerPkt * mVoxCoderReqs.wNumBitsPerFrame) / 8);
				mVoxComVars.dwNumBytesInCodedDataBuffer +=
					(0 != ((framesPerPkt * mVoxCoderReqs.wNumBitsPerFrame) % 8));
				mFrameSize = mVoxComVars.dwNumBytesInSpeechBuffer;
				mCFrameSize = mVoxComVars.dwNumBytesInCodedDataBuffer;
			}
			break;}
#endif
		case 'ADP8':
			mSampleSize = 16;
			mFrameSize = 320;
			mCFrameSize = 80;
			mFrames = 4;
			memset(&mADPCM, 0, sizeof(adpcm_state));
			mCoderSizeSlack = 3;
			#ifdef	PGP_MACINTOSH
				mCoderRate = 0x1f400000;
			#elif	PGP_WIN32
				mCoderRate = 8000.0;
			#endif
			break;
		case 'GS4L':
		case 'GS6L':
		case 'GS7L':
		case 'GL80':
		case 'GS1L':				/* 160 shorts -> 29 bytes			*/
			mGSM = gsm_create();
			mFrameSize = 320;
			mCFrameSize = 29;
			mSampleSize = 16;
			switch(type)
			{
				case 'GS4L':		// 4410 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x113A0000;
					#elif	PGP_WIN32
						mCoderRate = 4410.0;
					#endif
					mFrames = 2;
					break;
				case 'GS5L':		// 5512 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x15880000;
					#elif	PGP_WIN32
						mCoderRate = 5512.0;
					#endif
					mFrames = 3;
					break;
				case 'GS6L':		// 6000 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x17700000;
					#elif	PGP_WIN32
						mCoderRate = 6000.0;
					#endif
					mFrames = 3;
					break;
				case 'GS7L':		// 7350 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1cb60000;
					#elif	PGP_WIN32
						mCoderRate = 7350.0;
					#endif
					mFrames = 4;
					break;
				case 'GL80':		// 8000 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1f400000;
					#elif	PGP_WIN32
						mCoderRate = 8000.0;
					#endif
					mFrames = 5;
					break;
				case 'GS8L':		// 8820 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x22740000;
					#elif	PGP_WIN32
						mCoderRate = 8820.0;
					#endif
					mFrames = 6;
					break;
				case 'GS1L':		// 11025 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = rate11025hz;
					#elif	PGP_WIN32
						mCoderRate = 11025.0;
					#endif
					mFrames = 6;
					break;
			}
			inx = 1;
			gsm_option(mGSM, GSM_OPT_LTP_CUT, &inx);
			break;
		case 'GSM4':
		case 'GSM6':
		case 'GSM7':
		case 'GS80':
		case 'GSM1':				/* 160 shorts -> 33 bytes		4.8:1 */
			mGSM = gsm_create();
			mFrameSize = 320;
			mCFrameSize = 33;
			mSampleSize = 16;
			switch(type)
			{
				case 'GSM4':		// 4410 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x113A0000;
					#elif	PGP_WIN32
						mCoderRate = 4410.0;
					#endif
					mFrames = 2;
					break;
				case 'GSM6':		// 6000 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x17700000;
					#elif	PGP_WIN32
						mCoderRate = 6000.0;
					#endif
					mFrames = 3;
					break;
				case 'GSM7':		// 7350 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1cb60000;
					#elif	PGP_WIN32
						mCoderRate = 7350.0;
					#endif
					mFrames = 4;
					break;
				case 'GS80':		// 8000 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = 0x1f400000;
					#elif	PGP_WIN32
						mCoderRate = 8000.0;
					#endif
					mFrames = 5;
					break;
				case 'GSM1':		// 11025 hz
					#ifdef	PGP_MACINTOSH
						mCoderRate = rate11025hz;
					#elif	PGP_WIN32
						mCoderRate = 11025.0;
					#endif
					mFrames = 6;
					break;
			}
			break;
		case 'rand':
			mFrames = 100;
			mFrameSize = 2;
			mCFrameSize = 2;
			mSampleSize = 16;
			break;
		default:
			/* unknown codec */
			pgp_errstring("CSoundInput: Bad Codec");
			break;
	}
	mUpSampleSize = FALSE;
	if(mHardwareIs16Bit)
	{
		if(mOpen)
		{
#ifdef PGP_MACINTOSH
			::SPBSetDeviceInfo(mSoundRef, siSampleSize, &mSampleSize);
#elif	PGP_WIN32
			//Not implemented yet, careful using 8 bit coders on WIN32.
			//We dont have any 8 bit coders right now.
#endif
		}
	}
	else if(mSampleSize == 16)
		mUpSampleSize = TRUE;
	/*
	// Use the following code to force 8 bit samples
	mUpSampleSize = TRUE;	inx=8;
	::SPBSetDeviceInfo(mSoundRef, siSampleSize, &inx);
	*/
	
	mSampleBufSize = INSAMPLEBUFMAX / mFrameSize * mFrameSize;
#ifdef	PGP_MACINTOSH
	mDefaultTrail = mCoderRate >> 16;	// send for half a second
#elif	PGP_WIN32
	mDefaultTrail = mCoderRate;			// send for half a second
#endif
	mCodec = type;
	// Tell the packet thread milliseconds between packets
	if(mPacketThread)
		mPacketThread->SetSound(1000.0 /
				((float)mDefaultTrail / ((float)(mFrames * (mFrameSize / 2)))));
	mTrailSamples = 0;
	mPFWindow->SetEncoder(type);
	mSoundMutex.Signal();
}

void
CSoundInput::Record(Boolean record)
{
#ifdef PGP_WIN32
	short i;
	int err;
#endif

	mSoundMutex.Wait();
	if(mRecording != record)
	{
		mRecording = record;
		if(mRecording)
		{
			SetStatus(TRUE);
			mLevelMeter->SetStatus(TRUE);
			Open();
			if(mCompSnd)
				safe_free(mCompSnd);
			mCompSnd = NIL;
			mCompDone = 0;
			mDownDone = 0;
			mTrailSamples = 0;
#ifdef	PGP_MACINTOSH
			mSPB.count = 0;
			mSPB.milliseconds = 0;
			mSPB.bufferLength = 0;
			mSPB.bufferPtr = NIL;
			::SPBRecord(&mSPB, TRUE);
#elif	PGP_WIN32
			SetThreadPriority(mThreadHandle, THREAD_PRIORITY_NORMAL);
			mHeaderIndex = sHeaderIndex = 0;
			for(i=0;i<NUMINPUTBUFFERS;i++)
			{
				if(!(mHeaders[i].dwFlags & WHDR_PREPARED))
				{
					mHeaders[i].dwBufferLength = RAWRECORDQUANT;
					mHeaders[i].lpData = (char *)mMemPtrs[i];
					err = waveInPrepareHeader(mWaveHandle, &(mHeaders[i]), sizeof(WAVEHDR));
					CheckWaveError("waveInPrepareHeader", err);
				}
				err = waveInAddBuffer(mWaveHandle, &(mHeaders[i]), sizeof(WAVEHDR));
				CheckWaveError("waveInAddBuffer", err);
			}
			// start recording
			err = waveInStart(mWaveHandle);
			CheckWaveError("waveInStart", err);
#endif
		}
		else
		{
			SetStatus(FALSE);
			mLevelMeter->SetStatus(FALSE);
#ifdef	PGP_MACINTOSH
			::SPBStopRecording(mSoundRef);
#elif	PGP_WIN32
			err = waveInReset(mWaveHandle);
			CheckWaveError("waveInReset", err);
#endif
			Close();
#ifdef	PGP_WIN32
			// Wait until all of the blocks are finished.
			// This is necessary to ensure that half duplex
			// machines do not open the output before the input
			// is finished.

			for(i=0;i<NUMINPUTBUFFERS;i++)
				while(mHeaders[i].dwFlags && !(mHeaders[i].dwFlags & WHDR_DONE))
					Sleep(0);
#endif
		}
	}
	mSoundMutex.Signal();
}

void
CSoundInput::Pause(Boolean pause)
{
	mSoundMutex.Wait();
	mPaused = pause;
	if(mCompSnd)
		safe_free(mCompSnd);
	mCompSnd = NIL;
	mCompDone = 0;
	mDownDone = 0;
	mTrailSamples = 0;
	SetStatus(!pause);
	mSoundMutex.Signal();
}

void
CSoundInput::SetOutput(LThread *outThread, CMessageQueue *outQueue)
{
	mSoundMutex.Wait();
	mOutThread = outThread;
	mOutQueue = outQueue;
	mSoundMutex.Signal();
}

void
CSoundInput::SetPacketThread(CPFPacketsOut *packetThread)
{
	mSoundMutex.Wait();
	mPacketThread = packetThread;
	mSoundMutex.Signal();
}

void
CSoundInput::SetThreshold(long thresh)
{
	// We are given a threshold value from 0-32767
	// representing the amplitude which we should
	// require before sending sound packets
	pgpAssert(thresh <= MAX_THRESHOLD);
	
	mThreshold = thresh;
	mTrailSamples = 0;
	if(mRecording)
		SetStatus(TRUE);
}

void
CSoundInput::SetStatus(Boolean status)
{
	// We set the window to the actual status just to let the user know,
	// but if we are not in a real call, we set our internal status
	// to false.
	mPFWindow->SetSoundInStatus(status);
	if(mPacketThread)
	{
		if(mCodec == 'rand')
			mPacketThread->SetSoundStatus(FALSE);
		else
			mPacketThread->SetSoundStatus(status);
	}
}

void
CSoundInput::AbortSync()
{
	mAbort = TRUE;
#ifdef PGP_WIN32
	long count;
	ReleaseSemaphore(siSemaphore, 1, &count);
#endif
}

void Amplify(short *samples, int numSamples, double gain)
{
    /*
     * The following is equivalent to gain * 256, but rounds to the
     * nearest integer instead of truncating the fractional part
     */
    short x = (gain * 512 + 1) / 2;

    while (numSamples--)
    {
		*samples = ((long)*samples * x) >> 8;
		samples++;
    }
}

