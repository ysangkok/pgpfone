/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFWindow.h,v 1.10 1999/03/10 02:36:24 heller Exp $
____________________________________________________________________________*/
#ifndef CPFWINDOW_H
#define CPFWINDOW_H

#include "PGPFone.h" 
#include "CStatusPane.h"
#include "LMutexSemaphore.h"
#include "CLevelMeter.h"
#include "CTriThreshold.h"
#include "CAuthWindow.h"
#include "CSoundLight.h"
#include "CPFTransport.h"
#include "CXferWindow.h"

#define OUTQUEUEENTRIES	10

class CPGPFoneFrame;
class CControlThread;
class CStatusPane;
class CMessageQueue;
class CSoundInput;
class CSoundOutput;
class CCoderRegistry;

class CPFWindow : public CView, public LMutexSemaphore
{
protected:
	CPFWindow();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CPFWindow)

// Attributes
public:

// Operations
public:
	void		ShowCaller();
	void		SetState(enum CSTypes newState);
	void		SetLocalAddress(char *addr);
	void		SetSpeaker(Boolean speaker, Boolean fullDuplex);
	void		ShowAuthParams(char *params);
	void		HideAuthParams(void);
	void		ShowStatus(void);
	void		SetDecoder(ulong coder);
	void		SetEncoder(ulong coder);
	void		SetEncryption(ulong firstCryptor, ulong secondCryptor);
	void		SetDecoderList(uchar *decoders, short numDecoders);
	void		SetEncoderList(uchar *encoders, short numEncoders);
	void		ClearDecoderList(void);
	void		ClearEncoderList(void);
	void		SetKeyExchange(char *keyExchange);
	void		AllowXfers(Boolean allow);
	void		SetSoundInStatus(Boolean sending);
	void        Die(void);
	CStatusPane *GetStatusPane(void);
	CControlThread*GetControlThread()	{return mControlThread;}
	void	InitCall(void);
	void	BytesRead(long read);
	void	BytesWritten(long written);
	void	EscapedByte(void);
	void	GotPacket(Boolean good);
	void	OverflowOut(short numLost);
	void	SentPacket(void);
	void	SetGSMTimes(long full, long lite, long fullDec, long liteDec);
	void	SetRTTime(ulong rttVal);
	void	SetJitter(long jitter);
	void	SetBandwidth(long bandwidth);
	void	SetSoundOutput(CSoundOutput *soundOut)	{	mSoundOutput = soundOut;	}
	void	SetHPPQueue(CMessageQueue *hppOutQueue)
				{	mHPPOutQueue=hppOutQueue;	}
	void	SetSoundOutQueue(CMessageQueue *sndOutQ)
				{	mSoundOutQueue=sndOutQ;		}
	void	SkippedInputBlock(void);
	void	Underflow(void);
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPFWindow)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CPFWindow();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	void UpdateViewOptions();
	//{{AFX_MSG(CPFWindow)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnFileAnswer();
	afx_msg void OnUpdateFileAnswer(CCmdUI* pCmdUI);
	afx_msg void OnViewStatusInfo();
	afx_msg void OnUpdateViewStatusInfo(CCmdUI* pCmdUI);
	afx_msg void OnEditPreferences();
	afx_msg void OnUpdateEditPreferences(CCmdUI* pCmdUI);
	afx_msg void OnViewEncodingDetails();
	afx_msg void OnUpdateViewEncodingDetails(CCmdUI* pCmdUI);
	afx_msg void OnTransferFile();
	afx_msg void OnUpdateTransferFile(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CPGPFoneFrame			*mPFoneFrame;
	CControlThread			*mControlThread;
	CXferWindow				mFTP;
	Boolean					mTalkFlag, mSending, mFullDuplex, mDead;
	long					mControlResult, mSoundInputResult,
							mSoundOutputResult,
							mViewEncodingDetails, mViewStatusInfo;
	CMessageQueue			*mControlQueue;
	enum CSTypes			mState;
	CSoundInput				*mSoundInput;
	CSoundOutput			*mSoundOutput;
	CCoderRegistry			*mCoderRegistry;
	CAuthWindow				mAuthWindow;

	short					mVolume;
	uchar					mCryptor[4];

	// Interface objects
	CFont					mPFont, mPBFont,mPFont2, mPBFont2;
	CStatic					mLocalIPTitleStatic,
							mLocalIPStatic,
							mRemoteIDStatic,
							mRemoteIDTitleStatic,
							mCoderTitleStatic,
							mDecoderTitleStatic,
							mCryptorTitleStatic,
							mCryptorStatic,
							mPrimeTitleStatic,
							mPrimeStatic;
	HBRUSH					mBgBrush;
	CSliderCtrl				mVolumeSlider,
							mGainSlider;
	CButton					mConnectButton,
							mTalkButton;
	CLevelMeter				mLevelMeter;
	CTriThreshold			mThreshold;
	CSoundLight             mSoundLight;
	CBitmap					mLogo,
							mVolumeBitmap,
							mGainBitmap;
	CComboBox				mCallComboBox,
							mCoderComboBox,
							mDecoderComboBox;
	CString					mStatusBar;
	
	CStatusPane				mStatusPane;
	CStatic					mTitles1, mTitles2,
							mInfo1, mInfo2, mDivider;
	CMessageQueue			*mHPPOutQueue;
	CMessageQueue			*mSoundOutQueue;
	long		mBadPackets,		// number of bad packets received
				mGoodPackets,		// number of good packets received
				mPacketsSent,		// total number of packets sent
				mOutputUnderflows,	// number of sound-output underflows
				mOverflows,			// number of sound-input overflows
				mGSMfull,			// number of milliseconds to compress
									//	1-second of sound with GSM 7350Hz
				mGSMlite,			// number of milliseconds to compress
									//	1-second with GSM 7350Hz Lite
				mGSMfullDec,		// number of milliseconds to decompress
									//	1-second of sound with GSM 7350Hz
				mGSMliteDec,		// number of milliseconds to decompress
									//	1-second of sound with GSM 7350Hz Lite
				mTotalEscaped,		// total number of bytes escaped by HPP
				mRTTms,				// round trip milliseconds
				mMaxQueueSize,		// maximum outgoing queue size
				mEntryPos,			// Out queue sizes index
				mAvgHPP,			// average outgoing queue size
				mLastSampleTime,	// last time we analyzed queues in milliseconds
				mDirty,				// whether the watch must be redrawn
				mJitter,			// network packet jitter
				mBandwidth;			// network estimated bandwidth

	short		mOutQueueSizes[OUTQUEUEENTRIES];
};

#endif

