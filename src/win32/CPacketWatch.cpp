/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPacketWatch.cpp,v 1.4 1999/03/10 02:35:20 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CPacketWatch.h"
#include "CPGPFone.h"
#include "PGPFoneUtils.h"
#include "CSoundInput.h"
#include "CMessageQueue.h"
#include "CSoundOutput.h"
#include "CStatusPane.h"

#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const RECT sStatusPaneRect =	{13,5,388,65};
const RECT sPWTitles1Rect =		{13,70,100,125};
const RECT sPWTitles2Rect =		{200,70,300,125};
const RECT sPWInfo1Rect =		{101,70,195,125};
const RECT sPWInfo2Rect =		{301,70,388,125};

#define IDI_PWTITLES1	3000
#define IDI_PWTITLES2	3001
#define IDI_PWINFO1		3002
#define IDI_PWINFO2		3003

#define PWUPDATETIMER	2000

CPGPFPacketView *gPacketWatch;

IMPLEMENT_DYNCREATE(CPGPFPacketView, CView)

CPGPFPacketView::CPGPFPacketView()
{
	gPacketWatch = this;

	mGoodPackets	= 0;
	mBadPackets		= 0;
	mAvgHPP			= 0;
	mMaxQueueSize	= 0;
	mEntryPos		= 0;
	mPacketsSent	= 0;
	mOutputUnderflows = 0;
	mOverflows		= 0;
	mGSMfull		= 0;
	mGSMlite		= 0;
	mGSMfullDec		= 0;
	mGSMliteDec		= 0;
	mJitter			= 0;
	mBandwidth		= 0;
	mDirty			= 0;
	mRTTms			= 0;
	mLastSampleTime = 0;
	mHPPOutQueue	= NULL;
	mSoundOutQueue	= NULL;
	mSoundOutput	= NULL;
}

CPGPFPacketView::~CPGPFPacketView()
{
}

void
CPGPFPacketView::SetRTTime(ulong rttVal)
{
	InterlockedExchange(&mRTTms, rttVal);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::SetJitter(long jitter)
{
	InterlockedExchange(&mJitter, jitter);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::SetBandwidth(long bandwidth)
{
	InterlockedExchange(&mBandwidth, bandwidth);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::GotPacket(Boolean good)
{
	if(!good)
		InterlockedIncrement(&mBadPackets);
	else
		InterlockedIncrement(&mGoodPackets);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::SentPacket(void)
{
	InterlockedIncrement(&mPacketsSent);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::SetGSMTimes(long full, long lite, long fullDec, long liteDec)
{
	InterlockedExchange(&mGSMfull, full/10);
	InterlockedExchange(&mGSMlite, lite/10);
	InterlockedExchange(&mGSMfullDec, fullDec/10);
	InterlockedExchange(&mGSMliteDec, liteDec/10);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::InitCall(void)
{
	long inx;

	InterlockedExchange(&mGoodPackets, 0);
	InterlockedExchange(&mBadPackets, 0);
	InterlockedExchange(&mAvgHPP, 0);
	InterlockedExchange(&mMaxQueueSize, 0);
	InterlockedExchange(&mEntryPos, 0);
	InterlockedExchange(&mPacketsSent, 0);
	InterlockedExchange(&mOutputUnderflows, 0);
	InterlockedExchange(&mOverflows, 0);
	InterlockedExchange(&mDirty, 1);
	InterlockedExchange(&mRTTms, 0);
	InterlockedExchange(&mAvgHPP, 0);
	InterlockedExchange(&mMaxQueueSize, 0);
	for(inx=0;inx<PWOUTQUEUEENTRIES;inx++)
		mOutQueueSizes[inx] = 0;
}

void
CPGPFPacketView::Underflow()
{
	InterlockedIncrement(&mOutputUnderflows);
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::OverflowOut(short numLost)
{
	mOverflows += numLost;
	InterlockedExchange(&mDirty, 1);
}

void
CPGPFPacketView::OnDraw(CDC* pDC)
{
}

BEGIN_MESSAGE_MAP(CPGPFPacketView, CView)
	//{{AFX_MSG_MAP(CPGPFPacketView)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL
CPGPFPacketView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.lpszClass = MAKEINTRESOURCE(32770);
	return CView::PreCreateWindow(cs);
}

BOOL
CPGPFPacketView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
							 DWORD dwStyle, const RECT& rect, CWnd* pParentWnd,
							 UINT nID, CCreateContext* pContext) 
{
	BOOL result;

	result = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect,
							pParentWnd, nID, pContext);
	mPFont.CreateFont(11,0,0,0,0,0,0,0,0,0,0,0,0,"Small Fonts");
	mPBFont.CreateFont(11,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Small Fonts");
	mPWTitles1.Create("Packets Sent:\rGood Rcvd:\rBad Rcvd:\r"
						"Round Trip:\rGSM Load:",
							WS_CHILD|WS_VISIBLE,
							sPWTitles1Rect,this,IDI_PWTITLES1);
	mPWTitles1.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mPWTitles2.Create("Samples Queued:\rSound Underflows:\rSound Overflows:\r"
						"Jitter:\rEst. Bandwidth:",
							WS_CHILD|WS_VISIBLE,
							sPWTitles2Rect,this,IDI_PWTITLES2);
	mPWTitles2.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mPWInfo1.Create("", WS_CHILD|WS_VISIBLE,
							sPWInfo1Rect,this,IDI_PWINFO1);
	mPWInfo1.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);
	mPWInfo2.Create("", WS_CHILD|WS_VISIBLE,
							sPWInfo2Rect,this,IDI_PWINFO2);
	mPWInfo2.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);
	mStatusPane.CreateEx(WS_EX_CLIENTEDGE,"EDIT","",
						WS_VISIBLE|WS_CHILD|
						WS_VSCROLL|ES_READONLY|ES_MULTILINE,
						sStatusPaneRect.left, sStatusPaneRect.top,
						sStatusPaneRect.right-sStatusPaneRect.left,
						sStatusPaneRect.bottom-sStatusPaneRect.top,
						GetSafeHwnd(),0);
	mStatusPane.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);
	if(!SetTimer(PWUPDATETIMER, 500, NULL))
		pgp_errstring("error creating timer");
	return result;
}

void
CPGPFPacketView::OnTimer(UINT nIDEvent) 
{
	char s[128], t[128];
	long samp, inx;

	if(mHPPOutQueue)
	{
		samp = mHPPOutQueue->GetSize();
		if(samp > mMaxQueueSize)
			mMaxQueueSize = samp;
		if(mLastSampleTime + 1000 <= pgp_getticks())
		{
			mOutQueueSizes[mEntryPos] = samp;
			mEntryPos++;
			mEntryPos %= PWOUTQUEUEENTRIES;
			mLastSampleTime = pgp_getticks();
			for(inx=mAvgHPP=0;inx<PWOUTQUEUEENTRIES;inx++)
				mAvgHPP += mOutQueueSizes[inx];
			mAvgHPP /= PWOUTQUEUEENTRIES;
			if(mAvgHPP >= SOUNDINPUTQUEUEAVGMAX)
			{	// we have become very backed up
				mOverflows += samp;
				for(inx=mAvgHPP=0;inx<PWOUTQUEUEENTRIES;inx++)
					mOutQueueSizes[inx]=0;
				mHPPOutQueue->FreeType(_mt_voicePacket);	// so ditch outgoing
				InterlockedIncrement(&mDirty);
			}
		}
	}
	if(!mDirty)
		return;
	InterlockedExchange(&mDirty, 0);

	_itoa(mRTTms, t, 10);
	sprintf(s, "%d\r%d\r%d\r%sms\r%d%%",
				mPacketsSent, mGoodPackets, mBadPackets,
				t, mGSMfull);
	mPWInfo1.SetWindowText(s);

	_itoa(mSoundOutput->GetNumPlaying(), t, 10);
	sprintf(s, "%s\r%d\r%d\r%dms\r%d",
				t, mOutputUnderflows, mOverflows, mJitter, mBandwidth);
	mPWInfo2.SetWindowText(s);

	mStatusPane.UpdateStatus();
	CView::OnTimer(nIDEvent);
}

