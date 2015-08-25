/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPFWindow.cpp,v 1.14 1999/03/10 02:36:22 heller Exp $
____________________________________________________________________________*/
#include "CPFWindow.h"
#include "CXferWindow.h"
#include "CPGPFone.h"
#include "CPGPFoneFrame.h"
#include "PGPFoneUtils.h"
#include "CSoundInput.h"
#include "CSoundOutput.h"
#include "CMessageQueue.h"
#include "CControlThread.h"
#include "CEncryptionStream.h"
#include "CPhoneDialog.h"
#include "CModemDialog.h"
#include "CEncryptionDialog.h"
#include "CFileTransferDialog.h"
#include "CPFTransport.h"
#include "CStatusPane.h"
#include "fastpool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const POINT sLogoLeftTop =			{14,8};
const POINT sGainIconLeftTop =		{211,12};
const POINT sVolIconLeftTop =		{123,12};
const POINT sLevelMeterLeftTop =	{303,11};
const RECT sCallComboBoxRect =		{13,44,217,164};
const RECT sVolumeSliderRect =		{138,10,198,28};
const RECT sGainSliderRect =		{228,10,288,28};
const RECT sLocalIPTitleRect =		{230,69,295,84};
const RECT sLocalIPRect =			{296,67,388,85};
const RECT sConnectButtonRect =		{13,67,85,87};
const RECT sTalkButtonRect =		{86,67,217,87};
const RECT sCoderComboBoxRect =		{73,97,217,217};
const RECT sDecoderComboBoxRect =	{73,122,217,242};
const RECT sCoderTitleRect =		{13,99,72,114};
const RECT sDecoderTitleRect =		{13,124,72,139};
const RECT sRemoteIDTitleRect =		{13,49,66,64};
const RECT sRemoteIDRect =			{67,47,217,65};
const RECT sCryptorTitleRect =		{230,99,295,114};
const RECT sCryptorRect =			{296,97,388,115};
const RECT sPrimeTitleRect =		{230,124,295,139};
const RECT sPrimeRect =				{296,122,388,142};

const RECT sStatusPaneRect =		{13,220,388,280}; 	//{13,155,388,215};
const RECT sTitles1Rect =			{13,155,100,210}; 	//{13,220,100,275};
const RECT sTitles2Rect =			{230,155,330,210};	//{200,220,300,275};
const RECT sInfo1Rect =				{101,155,195,210};	//{101,220,195,275};
const RECT sInfo2Rect =				{331,155,400,210};	//{301,220,388,275};
const RECT sDividerRect =			{13,149,388,151}; 	//{13,155,388,215};

const RECT sStatusPaneRect2 =		{13,162,388,222};	//{13,97,388,157}
const RECT sTitles1Rect2 =			{13,97,100,152};	//{13,162,100,217}
const RECT sTitles2Rect2 =			{230,97,330,152};	//{200,162,300,217}
const RECT sInfo1Rect2 =			{101,97,195,152};	//{101,162,195,217}
const RECT sInfo2Rect2 =			{331,97,400,152};	//{301,162,388,217}
const RECT sDividerRect2 =			{13,92,388,94}; 	//{13,155,388,215};

const int sAuthWindowWidth =		153;
const int sAuthWindowHeight =		148;

const int sFTPWindowWidth =			363;
const int sFTPWindowHeight =		284;

#define IDI_TALKBUTTON				2000
#define IDI_CONNECTBUTTON			2001
#define IDI_LOCALIPTITLESTATIC		2002
#define IDI_DECODERCOMBOBOX			2003
#define IDI_CODERCOMBOBOX			2004
#define IDI_CALLCOMBOBOX			2005
#define IDI_CRYPTORTITLESTATIC		2006
#define IDI_REMOTEIDTITLESTATIC		2007
#define IDI_PRIMETITLESTATIC		2008
#define IDI_CODERTITLESTATIC		2009
#define IDI_DECODERTITLESTATIC		2010
#define IDI_LEVELMETER				2011
#define IDI_TRITHRESHOLD        	2012
#define IDI_SOUNDLIGHT          	2013
#define IDI_TITLES1					2014
#define IDI_TITLES2					2015
#define IDI_INFO1					2016
#define IDI_INFO2					2017
#define IDI_DIVIDER					2018
#define IDI_VOLUMESLIDER			2019
#define IDI_GAINSLIDER				2020

#define RANDOM_DATA_TIMER       	1000
#define DEBUG_TIMER             	1001
#define UPDATE_TIMER				1002

void DrawMaskedBitmap(CDC* pDC, UINT bitmapID, UINT maskID, POINT leftTop);

CPFWindow *gPacketWatch;

IMPLEMENT_DYNCREATE(CPFWindow, CView)


CPFWindow::CPFWindow()
{
	gPacketWatch 		= this;

	mGoodPackets		= 0;
	mBadPackets			= 0;
	mAvgHPP				= 0;
	mMaxQueueSize		= 0;
	mEntryPos			= 0;
	mPacketsSent		= 0;
	mOutputUnderflows 	= 0;
	mOverflows			= 0;
	mGSMfull			= 0;
	mGSMlite			= 0;
	mGSMfullDec			= 0;
	mGSMliteDec			= 0;
	mJitter				= 0;
	mBandwidth			= 0;
	mDirty				= 0;
	mRTTms				= 0;
	mLastSampleTime 	= 0;
	mHPPOutQueue		= NULL;
	mSoundOutQueue		= NULL;
	mSoundOutput		= NULL;
	mDead 				= FALSE;
	mTalkFlag 			= FALSE;
	mSending 			= FALSE;
	mControlThread 		= NULL;
	mPFoneFrame 		= NULL;
	
	mViewStatusInfo = gPGFOpts.sysopt.viewStatusInfo;
	mViewEncodingDetails = gPGFOpts.sysopt.viewEncodingDetails;
	
}

CPFWindow::~CPFWindow()
{
}

void
CPFWindow::ShowCaller()
{
	StMutex	mutex(*this);
	char s[64];

	if(mDead)
		return;
	mControlThread->GetRemoteName(s);
	mRemoteIDTitleStatic.ShowWindow(SW_SHOW);
	mRemoteIDStatic.ShowWindow(SW_SHOW);
	mRemoteIDStatic.SetWindowText(s);
}



void
CPFWindow::SetState(enum CSTypes newState)
{
	StMutex	mutex(*this);

	if(mDead)
		return;
	mState = newState;
	ShowStatus();
}

void
CPFWindow::SetLocalAddress(char *addr)
{
	mLocalIPStatic.SetWindowText(addr);
}

void
CPFWindow::SetSpeaker(Boolean speaker, Boolean fullDuplex)
{
	StMutex mutex(*this);

	if(mDead)
		return;
	mTalkFlag = (BOOLEAN)speaker;
	if(fullDuplex)
		mTalkButton.SetWindowText(speaker ? "Mute" : "Unmute");
	else
		mTalkButton.SetWindowText(speaker ? "Push to Listen":"Push to Talk");
}

void
CPFWindow::ShowAuthParams(char *params)
{
	StMutex	mutex(*this);
	RECT frame;

	if(mDead)
		return;
	GetParent()->GetWindowRect(&frame);
	mAuthWindow.MoveWindow(frame.right+2,frame.top,
							sAuthWindowWidth,
							sAuthWindowHeight);
	mAuthWindow.SetAuthWords(params);
	mAuthWindow.ShowWindow(SW_SHOW);
}

void
CPFWindow::HideAuthParams(void)
{
	StMutex	mutex(*this);

	if(mDead)
		return;
	mAuthWindow.ShowWindow(SW_HIDE);
}

void
CPFWindow::ShowStatus(void)
{
	StMutex mutex(*this);
	static int ID;

	if(mDead || !mPFoneFrame)
		return;
	switch(mState)
	{
		case _cs_none:
		case _cs_uninitialized:
		case _cs_listening:
			mRemoteIDStatic.ShowWindow(SW_HIDE);
			mRemoteIDTitleStatic.ShowWindow(SW_HIDE);
			if( gHardwareIsFullDuplex )
			{
				mTalkButton.SetWindowText("Test");
				mTalkButton.ShowWindow(SW_SHOW);
			}
			mCallComboBox.ShowWindow(SW_SHOW);
			mConnectButton.ShowWindow(SW_SHOW);

			if(gPGFOpts.popt.connection == _cme_Serial)
			{
				mConnectButton.SetWindowText("Dial");
				mLocalIPStatic.ShowWindow(SW_HIDE);
				mLocalIPTitleStatic.ShowWindow(SW_HIDE);
			}
			else
			{
				mConnectButton.SetWindowText("Connect");
				mLocalIPStatic.ShowWindow(SW_SHOW);
				mLocalIPTitleStatic.ShowWindow(SW_SHOW);
			}

			mDecoderComboBox.EnableWindow(FALSE);
			mCoderComboBox.EnableWindow(FALSE);
			//SetDefID(IDC_DIAL);
			break;
		case _cs_connected:
			mConnectButton.SetWindowText("Hangup");
			mConnectButton.ShowWindow(SW_SHOW);
			if(mControlThread->GetControlState() == _con_Phone)
			{
				SetSpeaker(mTalkFlag, mFullDuplex);
				mTalkButton.ShowWindow(SW_SHOW);
				mDecoderComboBox.EnableWindow(TRUE);
				mCoderComboBox.EnableWindow(TRUE);
				//SetDefID(IDC_TALK_LISTEN);
			}
			else
			{
				if( !gHardwareIsFullDuplex )
					mTalkButton.ShowWindow(SW_HIDE);
				mDecoderComboBox.EnableWindow(FALSE);
				mCoderComboBox.EnableWindow(FALSE);
				//SetDefID(IDC_DIAL);
			}
			ShowCaller();
			break;
		case _cs_calldetected:
			mCallComboBox.ShowWindow(SW_HIDE);
			mConnectButton.SetWindowText("Answer");
			ShowCaller();
			//SetDefID(IDC_DIAL);
			break;
		case _cs_connecting:
		case _cs_disconnecting:
			mCallComboBox.ShowWindow(SW_HIDE);
			mConnectButton.ShowWindow(SW_SHOW);
			if( !gHardwareIsFullDuplex )
				mTalkButton.ShowWindow(SW_HIDE);
			mConnectButton.SetWindowText("Hangup");

			mDecoderComboBox.EnableWindow(FALSE);
			mCoderComboBox.EnableWindow(FALSE);

			//SetDefID(IDC_DIAL);
			break;
		case _cs_initializing:
			mCallComboBox.ShowWindow(SW_HIDE);
			mConnectButton.ShowWindow(SW_SHOW);
			if( !gHardwareIsFullDuplex )
				mTalkButton.ShowWindow(SW_HIDE);
			mConnectButton.SetWindowText("Cancel");

			mDecoderComboBox.EnableWindow(FALSE);
			mCoderComboBox.EnableWindow(FALSE);

			//SetDefID(IDC_DIAL);
			break;
	}
	switch(mState)
	{
		case _cs_listening:
			mStatusBar = "Waiting for call";
			mPFoneFrame->setStatusPhoneIconID(IDR_WAITING);
			break;
		case _cs_none:
		case _cs_uninitialized:
			mStatusBar = "None";
			mPFoneFrame->setStatusPhoneIconID(0);
			break;
		case _cs_connected:
			switch(mControlThread->GetControlState())
			{
				default:
				case _con_Configuring:
					mStatusBar = "Configuring";
					break;
				case _con_Phone:
					if(!memcmp(&mCryptor, "NONE", 4))
						mPFoneFrame->setStatusSecureIconID(IDR_INSECURE);
					else
						mPFoneFrame->setStatusSecureIconID(IDR_SECURE);
					break;
				case _con_Disconnecting:
					mStatusBar = "Confirming Hangup";
					break;
			}
			mPFoneFrame->setStatusPhoneIconID(IDR_CONNECTED);
			break;
		case _cs_calldetected:
			mStatusBar = "Ring";
			break;
		case _cs_connecting:
			mStatusBar = "Connecting";
			mPFoneFrame->setStatusPhoneIconID(IDR_CONNECTED);
			break;
		case _cs_disconnecting:
			mStatusBar = "Disconnecting";
			mPFoneFrame->setStatusPhoneIconID(IDR_CONNECTED);
			break;
		case _cs_initializing:
			mStatusBar = "Initializing modem";
			break;
		default:
			pgp_errstring("unknown status");
	}
	if(mState != _cs_connected)
		mPFoneFrame->setStatusSecureIconID(ID);
}

void
CPFWindow::SetDecoder(ulong coder)
{
	StMutex		mutex(*this);
	short		index = 0;
	char		*name;

	if(mDead)
		return;
	if(coder && mControlThread &&
		(name = mControlThread->GetCoderName(coder)))
		index = mDecoderComboBox.FindStringExact(-1, name);

	if(index == CB_ERR)
		index = 0;

	mDecoderComboBox.SetCurSel(index);
}

void
CPFWindow::SetEncoder(ulong coder)
{
	StMutex		mutex(*this);
	short		index = 0;
	char		*name;

	if(mDead)
		return;
	if(coder && mControlThread &&
		(name = mControlThread->GetCoderName(coder)))
		index = mCoderComboBox.FindStringExact(-1, name);

	if(index == CB_ERR)
		index = 0;

	mCoderComboBox.SetCurSel(index);
}

void
CPFWindow::SetEncryption(ulong firstCryptor, ulong secondCryptor)
{
	StMutex	mutex(*this);
	char str[64], *p = str;

	if(mDead)
		return;
	LONG_TO_BUFFER(firstCryptor, &mCryptor);
		 if(!memcmp(&mCryptor, sCryptorHash[_enc_none], 4))
		 	p += sprintf(p, "None");
	else if(!memcmp(&mCryptor, sCryptorHash[_enc_tripleDES], 4))
		 	p += sprintf(p, "TripleDES");
	else if(!memcmp(&mCryptor, sCryptorHash[_enc_blowfish], 4))
		 	p += sprintf(p, "Blowfish");
	else if(!memcmp(&mCryptor, sCryptorHash[_enc_cast], 4))
		 	p += sprintf(p, "CAST");
	else
		 	p += sprintf(p, "");
	mCryptorStatic.SetWindowText(str);
}

void
CPFWindow::SetDecoderList(uchar *decoders, short numDecoders)
{
	StMutex	mutex(*this);
	ulong	coder;
	char	*name;

	if(mDead)
		return;
	if(decoders && numDecoders && mControlThread)
	{
		while (mDecoderComboBox.DeleteString(0) != CB_ERR);

		while (numDecoders--)
		{
			BUFFER_TO_LONG(decoders, coder);
			if(name = mControlThread->GetCoderName(coder))
				mDecoderComboBox.AddString(name);
			decoders += 4;
		}
	}
	mDecoderComboBox.SetCurSel(0);
}

void
CPFWindow::SetEncoderList(uchar *encoders, short numEncoders)
{
	StMutex	mutex(*this);
	ulong	coder;
	char	*name;

	if(mDead)
		return;
	if(encoders && numEncoders && mControlThread)
	{
		while(mCoderComboBox.DeleteString(0) != CB_ERR);

		while(numEncoders--)
		{
			BUFFER_TO_LONG(encoders, coder);
			if(name = mControlThread->GetCoderName(coder))
				mCoderComboBox.AddString(name);
			encoders += 4;
		}
	}
}

void
CPFWindow::ClearDecoderList(void)
{
	StMutex	mutex(*this);

	if(mDead)
		return;
	while(mDecoderComboBox.DeleteString(0) != CB_ERR);
		mDecoderComboBox.Clear();
}

void
CPFWindow::ClearEncoderList(void)
{
	StMutex	mutex(*this);

	if(mDead)
		return;
	while(mCoderComboBox.DeleteString(0) != CB_ERR);
		mCoderComboBox.Clear();
}

void
CPFWindow::AllowXfers(Boolean allow)
{
}

void
CPFWindow::SetSoundInStatus(Boolean sending)
{
	if(sending != mSending)
	{
		if(sending)
		{
			mSoundLight.Switch(TRUE);
			mSending = TRUE;
		}
		else
		{
			mSoundLight.Switch(FALSE);
			mSending = FALSE;
		}
	}
}

CStatusPane *
CPFWindow::GetStatusPane(void)
{
	return NIL;
}


void CPFWindow::OnDraw(CDC* pDC)
{
	DrawMaskedBitmap(pDC,IDB_LOGO,IDB_LOGO_MASK,sLogoLeftTop);
	DrawMaskedBitmap(pDC, IDB_VOLUME, IDB_VOLUME_MASK, sVolIconLeftTop);
	DrawMaskedBitmap(pDC, IDB_MICROPHONE, IDB_MICROPHONE_MASK, sGainIconLeftTop);
}

BOOL CPFWindow::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.lpszClass = MAKEINTRESOURCE(32770);
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPFWindow diagnostics

#ifdef _DEBUG
void
CPFWindow::AssertValid() const
{
	CView::AssertValid();
}

void
CPFWindow::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

void
CPFWindow::SetKeyExchange(char *keyExchange)
{
	mPrimeStatic.SetWindowText(keyExchange);
}

BOOL
CPFWindow::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
						   DWORD dwStyle, const RECT& rect, CWnd* pParentWnd,
						   UINT nID, CCreateContext* pContext) 
{
	long coderResult;
	RECT frame;

	CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd,
					nID, pContext);
	mPFont.CreateFont(-10,0,0,0,0,0,0,0,0,0,0,0,0,"MS Sans Serif");
	mPBFont.CreateFont(-10,0,0,0,FW_BLACK,0,0,0,0,0,0,0,0,"MS Sans Serif");
	mLocalIPTitleStatic.Create("Local IP:",WS_CHILD|WS_VISIBLE,
							sLocalIPTitleRect,this,IDI_LOCALIPTITLESTATIC);
	mLocalIPTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mLocalIPStatic.CreateEx(WS_EX_CLIENTEDGE,"STATIC","",WS_CHILD|WS_VISIBLE,
							sLocalIPRect.left,sLocalIPRect.top,
							sLocalIPRect.right-sLocalIPRect.left,
							sLocalIPRect.bottom-sLocalIPRect.top,
							GetSafeHwnd(),0);
	mLocalIPStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);

	mRemoteIDTitleStatic.Create("Remote:",WS_CHILD|WS_VISIBLE,
							sRemoteIDTitleRect,this,IDI_REMOTEIDTITLESTATIC);
	mRemoteIDTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mRemoteIDStatic.CreateEx(WS_EX_CLIENTEDGE,"STATIC","",WS_CHILD|WS_VISIBLE,
							sRemoteIDRect.left,
							sRemoteIDRect.top,
							sRemoteIDRect.right-sRemoteIDRect.left,
							sRemoteIDRect.bottom-sRemoteIDRect.top,
							GetSafeHwnd(),0);
	mRemoteIDStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);

	mCryptorTitleStatic.Create("Encryption:",WS_CHILD|WS_VISIBLE,
							sCryptorTitleRect,this,IDI_CRYPTORTITLESTATIC);
	mCryptorTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mCryptorStatic.CreateEx(WS_EX_CLIENTEDGE,"STATIC","",WS_CHILD|WS_VISIBLE,
							sCryptorRect.left,sCryptorRect.top,
							sCryptorRect.right-sCryptorRect.left,
							sCryptorRect.bottom-sCryptorRect.top,
							GetSafeHwnd(),0);
	mCryptorStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);

	mPrimeTitleStatic.Create("Exchange:",WS_CHILD|WS_VISIBLE,
							sPrimeTitleRect,this,IDI_PRIMETITLESTATIC);
	mPrimeTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mPrimeStatic.CreateEx(WS_EX_CLIENTEDGE,"STATIC","",WS_CHILD|WS_VISIBLE,
							sPrimeRect.left,sPrimeRect.top,
							sPrimeRect.right-sPrimeRect.left,
							sPrimeRect.bottom-sPrimeRect.top,
							GetSafeHwnd(),0);
	mPrimeStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);

	mCoderTitleStatic.Create("Coder:",WS_CHILD|WS_VISIBLE,
							sCoderTitleRect,this,IDI_CODERTITLESTATIC);
	mCoderTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	mDecoderTitleStatic.Create("Decoder:",WS_CHILD|WS_VISIBLE,
							sDecoderTitleRect,this,IDI_DECODERTITLESTATIC);
	mDecoderTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);

	mCallComboBox.Create(CBS_AUTOHSCROLL|CBS_DROPDOWN|WS_CHILD|
							CBS_DISABLENOSCROLL|WS_VSCROLL|
							WS_VISIBLE|WS_TABSTOP,
							sCallComboBoxRect,this,
							IDI_CALLCOMBOBOX);
	mCallComboBox.LimitText(PHONENUMBERLEN-1);
	mCallComboBox.SetFont(&mPFont);

	mConnectButton.Create("Connect",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
							sConnectButtonRect,this,IDI_CONNECTBUTTON);
	mConnectButton.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, TRUE);
	mTalkButton.Create("Mute",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
							sTalkButtonRect,this,IDI_TALKBUTTON);
	mTalkButton.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, TRUE);

	mCoderComboBox.Create(CBS_DROPDOWNLIST|WS_CHILD|WS_VISIBLE|WS_VSCROLL|
							CBS_DISABLENOSCROLL|WS_TABSTOP,
							sCoderComboBoxRect,this,
							IDI_CODERCOMBOBOX);
	mCoderComboBox.SetFont(&mPFont);
	mDecoderComboBox.Create(CBS_DROPDOWNLIST|WS_CHILD|WS_VISIBLE|WS_VSCROLL|
							CBS_DISABLENOSCROLL|WS_TABSTOP,
							sDecoderComboBoxRect,this,
							IDI_DECODERCOMBOBOX);
	mDecoderComboBox.SetFont(&mPFont);

	/*mVolumeSlider.CreateEx(WS_EX_CLIENTEDGE,TRACKBAR_CLASS,"",
		TBS_NOTICKS|WS_VISIBLE|WS_CHILD|WS_BORDER|WS_TABSTOP,
		sVolumeSliderRect.left,
		sVolumeSliderRect.top,
		sVolumeSliderRect.right-sVolumeSliderRect.left,
		sVolumeSliderRect.bottom-sVolumeSliderRect.top,
		GetSafeHwnd(), 0);*/
	mVolumeSlider.Create(TBS_BOTTOM,sVolumeSliderRect,this,IDI_VOLUMESLIDER);
	mVolumeSlider.SetRange(0, 0xffff / 100);
	mVolumeSlider.SetPos(0x7FFF/100);
	mVolumeSlider.ShowWindow(SW_SHOW);
	mGainSlider.Create(TBS_BOTTOM,sGainSliderRect,this,IDI_GAINSLIDER);
	/*mGainSlider.CreateEx(WS_EX_CLIENTEDGE,TRACKBAR_CLASS,"",
		TBS_NOTICKS|WS_VISIBLE|WS_CHILD|WS_BORDER|WS_TABSTOP,
		sGainSliderRect.left,
		sGainSliderRect.top,
		sGainSliderRect.right-sGainSliderRect.left,
		sGainSliderRect.bottom-sGainSliderRect.top,
		GetSafeHwnd(), 0);*/
	mGainSlider.SetRange(0, 99);
	mGainSlider.SetPos(gPGFOpts.popt.siGain);
	mGainSlider.ShowWindow(SW_SHOW);

	mLevelMeter.Create("",
					   WS_CHILD|WS_VISIBLE,
					   sLevelMeterLeftTop,
					   this,
					   IDI_LEVELMETER, 
					   LEVELMETER_NUM_LEDS,
					   LEVELMETER_LED_WIDTH,
					   LEVELMETER_DELIMITER_WIDTH,
					   LEVELMETER_HEIGHT);
	mThreshold.Create("",
				      WS_CHILD | WS_VISIBLE,
				      this,
				      IDI_TRITHRESHOLD,
					  &mLevelMeter);
	mSoundLight.Create("",
					   WS_CHILD | WS_VISIBLE,
					   this,
					   IDI_SOUNDLIGHT,
					   &mLevelMeter,
					   RGB(0, 255, 0),
					   FALSE);


	mPFoneFrame = (CPGPFoneFrame *) pParentWnd;
	
	mPFoneFrame->GetWindowRect(&frame);
	
	mFTP.CreateEx( 		WS_EX_WINDOWEDGE| WS_EX_ACCEPTFILES |WS_EX_DLGMODALFRAME,
						MAKEINTRESOURCE(32770), 
						"PGPfone - Secure File Transfer",
						WS_BORDER | WS_DLGFRAME |
						WS_SYSMENU |WS_CAPTION| DS_MODALFRAME|
						WS_MINIMIZEBOX | WS_CAPTION,
						frame.left,
						frame.bottom + 5,
						sFTPWindowWidth,
						sFTPWindowHeight,
						NULL,
						0,
						NULL);
	
	// create the two sound threads
	mSoundInput  = new CSoundInput(this, &mLevelMeter,
		(void**)&mSoundInputResult);
	mSoundOutput = new CSoundOutput(this, (void**)&mSoundOutputResult);

	//gPacketWatch->SetSoundOutput(mSoundOutput);

	mControlQueue = new CMessageQueue();
	mControlThread = new CControlThread(mControlQueue, this,
		mSoundInput, mSoundOutput, (void**)&mControlResult);
	mSoundOutput->SetControlThread(mControlThread);

	// create the coder timing thread
	mCoderRegistry = new CCoderRegistry(mControlThread, (void**)&coderResult);
	mCoderRegistry->Resume();
	while(!coderResult)
		Sleep(0);

	mControlThread->SetSpeaker(TRUE);
	mControlThread->SetXferWindow(&mFTP);
 
	mSoundInput->Resume();
	mSoundOutput->Resume();
	mControlThread->Resume();

	mThreshold.SetSoundInput(mSoundInput);
	SetState(_cs_uninitialized);
	if(gPGFOpts.popt.alwaysListen)
		mControlQueue->Send(_mt_waitCall);
	//SetDefID(IDC_DIAL);

	mPFoneFrame->GetWindowRect(&frame);
	
	mAuthWindow.CreateEx(	WS_EX_WINDOWEDGE| WS_EX_ACCEPTFILES |
							WS_EX_DLGMODALFRAME |WS_EX_TOOLWINDOW, 
							MAKEINTRESOURCE(32770), 
							"Authenticate",
							WS_POPUPWINDOW|WS_CAPTION,
							frame.right+2,
							frame.top,
							sAuthWindowWidth,
							sAuthWindowHeight,
							GetSafeHwnd(),0,NULL);
							
	// keep track of the volume for the sound card
	mVolume = 7;
	
	mPFont2.CreateFont(11,0,0,0,0,0,0,0,0,0,0,0,0,"Small Fonts");
	
	mPBFont2.CreateFont(11,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Small Fonts");
	
	mTitles1.Create(	"Packets Sent:\rGood Rcvd:\rBad Rcvd:\r"
						"Round Trip:\rGSM Load:",
						WS_CHILD|WS_VISIBLE,
						sTitles1Rect,this,IDI_TITLES1);
							
	mTitles1.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont2, FALSE);
	
	mTitles2.Create(	"Samples Queued:\rSound Underflows:\rSound Overflows:\r"
						"Jitter:\rEst. Bandwidth:",
						WS_CHILD|WS_VISIBLE,
						sTitles2Rect,this,IDI_TITLES2);
							
	mTitles2.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont2, FALSE);
	
	mInfo1.Create(	"", 
						WS_CHILD|WS_VISIBLE,
						sInfo1Rect,this,IDI_INFO1);
							
	mInfo1.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont2, FALSE);
	
	mInfo2.Create(	"", 
						WS_CHILD|WS_VISIBLE,
						sInfo2Rect,this,IDI_INFO2);
							
	mInfo2.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont2, FALSE);
	
	mDivider.Create(	"",
						WS_CHILD|WS_VISIBLE|SS_ETCHEDFRAME,
						sDividerRect,this,IDI_DIVIDER);
						
	mStatusPane.CreateEx(	WS_EX_CLIENTEDGE,"EDIT","",
							WS_VISIBLE|WS_CHILD|
							WS_VSCROLL|ES_READONLY|ES_MULTILINE,
							sStatusPaneRect.left, sStatusPaneRect.top,
							sStatusPaneRect.right-sStatusPaneRect.left,
							sStatusPaneRect.bottom-sStatusPaneRect.top,
							GetSafeHwnd(),0);
						
	mStatusPane.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont2, FALSE);
	
	if(!SetTimer(UPDATE_TIMER, 500, NULL))
	{
		pgp_errstring("error creating timer");
	}
	
	UpdateViewOptions();
		
	return TRUE;
}

void CPFWindow::OnTransferFile() 
{
	mFTP.ShowWindow(SW_SHOW);
}

void CPFWindow::OnUpdateTransferFile(CCmdUI* pCmdUI) 
{
	if((mState == _cs_uninitialized || mState == _cs_listening) &&
		(gPGFOpts.popt.connection == _cme_Serial))
	{
		pCmdUI->Enable(1);
	}
	else
	{
		pCmdUI->Enable(0);
	}
	
#if PGP_DEBUG
	// for testing... always allow
	pCmdUI->Enable(1);
#endif
	
}

void CPFWindow::OnUpdateEditPreferences(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(1);
}

void CPFWindow::OnEditPreferences() 
{
	CPropertySheet		sheet("Preferences");
	CPhoneDialog		phoneDialog;
	CModemDialog		modemDialog;
	CEncryptionDialog	encryptDialog;
	CFileTransferDialog	ftpDialog;

	phoneDialog.SetParameters(&gPGFOpts);
	modemDialog.SetParameters(&gPGFOpts);
	encryptDialog.SetParameters(&gPGFOpts);
	ftpDialog.SetParameters(&gPGFOpts);

	sheet.AddPage(&phoneDialog);
	sheet.AddPage(&modemDialog);
	sheet.AddPage(&encryptDialog);
	sheet.AddPage(&ftpDialog);

	if(sheet.DoModal() == IDOK)
	{
		phoneDialog.GetParameters(&gPGFOpts);
		modemDialog.GetParameters(&gPGFOpts);
		encryptDialog.GetParameters(&gPGFOpts);
		ftpDialog.GetParameters(&gPGFOpts);

		mPFoneFrame->DoPrefs(1);
	}

//	GotoDlgCtrl(GetDlgItem(IDC_PHONENUMBER));
	if(mState == _cs_uninitialized || mState == _cs_none ||
		mState == _cs_listening	|| mState == _cs_initializing)
		mControlThread->ResetTransport();
}

void CPFWindow::OnViewStatusInfo() 
{
	mViewStatusInfo = !mViewStatusInfo;
	
	gPGFOpts.sysopt.viewStatusInfo = mViewStatusInfo;

	UpdateViewOptions();
}


void CPFWindow::OnUpdateViewStatusInfo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(1);
	
	pCmdUI->SetCheck(mViewStatusInfo);
}

void CPFWindow::OnViewEncodingDetails() 
{
	mViewEncodingDetails = !mViewEncodingDetails;

	gPGFOpts.sysopt.viewEncodingDetails = mViewEncodingDetails;
	
	UpdateViewOptions();
	
}

void CPFWindow::OnUpdateViewEncodingDetails(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(1);
	
	pCmdUI->SetCheck(mViewEncodingDetails);
	
}

void CPFWindow::UpdateViewOptions()
{
	RECT title1Rect;
	RECT title2Rect;
	RECT info1Rect;
	RECT info2Rect;
	RECT statusRect;
	RECT dividerRect;


	if( mViewEncodingDetails )
	{
		title1Rect	= sTitles1Rect;
		title2Rect	= sTitles2Rect;
		info1Rect	= sInfo1Rect;
		info2Rect	= sInfo2Rect;
		statusRect	= sStatusPaneRect;
		dividerRect = sDividerRect;

		mCryptorTitleStatic.ShowWindow(SW_SHOW);
		mCryptorStatic.ShowWindow(SW_SHOW);
		mPrimeTitleStatic.ShowWindow(SW_SHOW);
		mPrimeStatic.ShowWindow(SW_SHOW);
		mCoderTitleStatic.ShowWindow(SW_SHOW);
		mCoderComboBox.ShowWindow(SW_SHOW);
		mDecoderTitleStatic.ShowWindow(SW_SHOW);
		mDecoderComboBox.ShowWindow(SW_SHOW);
		
	}
	else
	{
		title1Rect	= sTitles1Rect2;
		title2Rect	= sTitles2Rect2;
		info1Rect	= sInfo1Rect2;
		info2Rect	= sInfo2Rect2;
		statusRect	= sStatusPaneRect2;
		dividerRect = sDividerRect2;

		mCryptorTitleStatic.ShowWindow(SW_HIDE);
		mCryptorStatic.ShowWindow(SW_HIDE);
		mPrimeTitleStatic.ShowWindow(SW_HIDE);
		mPrimeStatic.ShowWindow(SW_HIDE);
		mCoderTitleStatic.ShowWindow(SW_HIDE);
		mCoderComboBox.ShowWindow(SW_HIDE);
		mDecoderTitleStatic.ShowWindow(SW_HIDE);
		mDecoderComboBox.ShowWindow(SW_HIDE);
	}
	

	mTitles1.MoveWindow(&title1Rect);
	mTitles2.MoveWindow(&title2Rect);
	mInfo1.MoveWindow(&info1Rect);
	mInfo2.MoveWindow(&info2Rect);
	mStatusPane.MoveWindow(&statusRect);
	mDivider.MoveWindow(&dividerRect);
	mDivider.ShowWindow(SW_SHOW);
	
	//RedrawWindow();

	if( mViewStatusInfo  && mViewEncodingDetails )
	{
		GetParent()->SetWindowPos(	NULL,
									0,
									0,
									410,
									350,
									SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	}
	else if( mViewStatusInfo )
	{
		GetParent()->SetWindowPos(	NULL,
									0,
									0,
									410,
									292,
									SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	}
	else if( mViewEncodingDetails )
	{
		GetParent()->SetWindowPos(	NULL,
									0,
									0,
									410,
									210,
									SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
									
		mDivider.ShowWindow(SW_HIDE);
	}
	else
	{
		GetParent()->SetWindowPos(	NULL,
									0,
									0,
									410,
									154,
									SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
									
		mDivider.ShowWindow(SW_HIDE);
	}

}


BOOL CPFWindow::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(wParam & 0x0000FFFF)
	{
		case IDI_CODERCOMBOBOX:
			if((wParam >> 16) == CBN_SELCHANGE)
			{
				CString	str;
				ulong newEncoder;

				// get the string for the current selection
				mCoderComboBox.GetLBText(mCoderComboBox.GetCurSel(), str);

					 if(!strcmp(str, "GSM Lite 4410 hz"))	newEncoder ='GS4L';
				else if(!strcmp(str, "GSM Lite 6000 hz"))	newEncoder ='GS6L';
				else if(!strcmp(str, "GSM Lite 7350 hz"))	newEncoder ='GS7L';
				else if(!strcmp(str, "GSM Lite 8000 hz"))	newEncoder ='GL80';
				else if(!strcmp(str, "GSM Lite 11025 hz"))	newEncoder ='GS1L';
				else if(!strcmp(str, "GSM 4410 hz"))		newEncoder ='GSM4';
				else if(!strcmp(str, "GSM 6000 hz"))		newEncoder ='GSM6';
				else if(!strcmp(str, "GSM 7350 hz"))		newEncoder ='GSM7';
				else if(!strcmp(str, "GSM 8000 hz"))		newEncoder ='GS80';
				else if(!strcmp(str, "GSM 11025 hz"))		newEncoder ='GSM1';
				else if(!strcmp(str, "ADPCM 8000 hz"))		newEncoder ='ADP8';
				else
					pgp_errstring("unknown encoder");

				mControlQueue->Send(_mt_changeEncoder, (void*)newEncoder);
			}
			break;
		case IDI_DECODERCOMBOBOX:
			if((wParam >> 16) == CBN_SELCHANGE)
			{
				CString	str;
				ulong newDecoder;

				// get the string for the current selection
				mDecoderComboBox.GetLBText(mDecoderComboBox.GetCurSel(), str);

					 if(!strcmp(str, "GSM Lite 4410 hz"))	newDecoder ='GS4L';
				else if(!strcmp(str, "GSM Lite 6000 hz"))	newDecoder ='GS6L';
				else if(!strcmp(str, "GSM Lite 7350 hz"))	newDecoder ='GS7L';
				else if(!strcmp(str, "GSM Lite 8000 hz"))	newDecoder ='GL80';
				else if(!strcmp(str, "GSM Lite 11025 hz"))	newDecoder ='GS1L';
				else if(!strcmp(str, "GSM 4410 hz"))		newDecoder ='GSM4';
				else if(!strcmp(str, "GSM 6000 hz"))		newDecoder ='GSM6';
				else if(!strcmp(str, "GSM 7350 hz"))		newDecoder ='GSM7';
				else if(!strcmp(str, "GSM 8000 hz"))		newDecoder ='GS80';
				else if(!strcmp(str, "GSM 11025 hz"))		newDecoder ='GSM1';
				else if(!strcmp(str, "ADPCM 8000 hz"))		newDecoder ='ADP8';
				else
					pgp_errstring("unknown decoder");

				mControlQueue->Send(_mt_changeDecoder, (void*)newDecoder);
			}
			break;
		case IDI_CONNECTBUTTON:{
			char s[256];

			if((wParam >> 16) == BN_CLICKED)
			{
				switch(mState)
				{
					case _cs_none:
					case _cs_uninitialized:
					case _cs_listening:
						ContactEntry *entry;
					
						if(mState == _cs_listening)
							mControlThread->GetTransport()->AbortSync();

						mCallComboBox.GetWindowText(s, 256);
						if(mCallComboBox.FindStringExact(-1,s) == CB_ERR)
							mCallComboBox.AddString(s);
						entry = (ContactEntry*)pgp_mallocclear(sizeof(ContactEntry));
						switch(gPGFOpts.popt.connection)
						{
							case _cme_Serial:
								entry->method = _cme_Serial;
								strncpy(entry->phoneNumber, s, PHONENUMBERLEN);
								entry->modemInit[0] = 0;
								strcat(entry->modemInit, "AT");
								strcat(entry->modemInit, gPGFOpts.sopt.modemInit);
								break;
							case _cme_Internet:
								entry->method = _cme_Internet;
								entry->useIPSearch = 0;
								strncpy(entry->internetAddress, s, PHONENUMBERLEN);
								break;
						}

						CStatusPane::GetStatusPane()->Clear();
						mControlQueue->Send(_mt_call, entry);
						break;
					case _cs_calldetected:
						mControlThread->GetTransport()->Answer();
						break;
					case _cs_connecting:
					case _cs_disconnecting:
					case _cs_initializing:
					case _cs_connected:
						mControlThread->AbortSync(FALSE, FALSE);
						mFTP.ShowWindow(SW_HIDE);
						break;
				}
			}
			break;}
		case IDI_TALKBUTTON:
			if((wParam >> 16) == BN_CLICKED)
			{
				switch(mState)
				{
					case _cs_none:
					case _cs_uninitialized:
					case _cs_listening:
						if(mSoundInput->Recording())
						{
							mSoundInput->Record(FALSE);
							mSoundOutput->Play(FALSE);
							mTalkButton.SetWindowText("Test");
						}
						else
						{
							mSoundInput->SetCodec(gPGFOpts.popt.prefCoder);
							mSoundOutput->SetCodec(gPGFOpts.popt.prefCoder);
							mSoundInput->SetOutput(mSoundOutput, mSoundOutput->GetPlayQueue());
							mSoundInput->Record(TRUE);
							mSoundOutput->Play(TRUE);
							mTalkButton.SetWindowText("Stop Test");
						}
						break;
					case _cs_connected:
						if(mTalkFlag)
							mControlQueue->SendUnique(_mt_listen);
						else
							mControlQueue->SendUnique(_mt_talk);
						break;
					case _cs_calldetected:
					case _cs_connecting:
					case _cs_disconnecting:
					case _cs_initializing:
						break;
				}
			}
			break;
	}
	return CView::OnCommand(wParam, lParam);
}

void 
CPFWindow::OnTimer(UINT nIDEvent) 
{

	switch(nIDEvent)
	{
		case RANDOM_DATA_TIMER:
		{
			ulong ticks;
			static BOOL performanceCounterAvailable;
			static BOOL	checkedPerformanceCounterAvailability = FALSE;
			LARGE_INTEGER performanceCount;
	
			/*First, get clock tick info...*/
			ticks = GetTickCount();
			randPoolAddWord(ticks);
	
			/*The performance counter is available on some platforms.  If it is
			*available, it can have a precision of about one microsecond.  If it     
			*is available, we use it as a random bit source.  Since it seems
			*silly to check if the thing is available every time we get a timer
			*message, we only check our first run through.
			*/
			if(!checkedPerformanceCounterAvailability)
			{
				checkedPerformanceCounterAvailability = TRUE;
				performanceCounterAvailable = 
					QueryPerformanceCounter(&performanceCount);
			}
	
			if(performanceCounterAvailable)
			{
				if(QueryPerformanceCounter(&performanceCount))
					randPoolAddBytes((uchar *) &performanceCount,
					sizeof(performanceCount));
				else
				{
				/*This is a very bad thing: we said it was available before,
				 *but now MS says it's not!
				 */
					pgp_errstring(
						"performanceCounterAvailable contains inconsistant value!");
				}
			}
			
			break;
		}

#if 0
		case DEBUG_TIMER:
		{
			ShowStatus();
			break;
		}
#endif

		case UPDATE_TIMER:
		{
			char s[128], t[128];
			long samp, inx;
		
			if(mHPPOutQueue)
			{
				samp = mHPPOutQueue->GetSize();
				
				if(samp > mMaxQueueSize)
				{
					mMaxQueueSize = samp;
				}
				
				if(mLastSampleTime + 1000 <= pgp_getticks())
				{
					mOutQueueSizes[mEntryPos] = samp;
					mEntryPos++;
					mEntryPos %= OUTQUEUEENTRIES;
					mLastSampleTime = pgp_getticks();
					
					for(inx=mAvgHPP=0;inx<OUTQUEUEENTRIES;inx++)
					{
						mAvgHPP += mOutQueueSizes[inx];
					}
					
					mAvgHPP /= OUTQUEUEENTRIES;
					
					if(mAvgHPP >= SOUNDINPUTQUEUEAVGMAX)
					{	
						// we have become very backed up
						mOverflows += samp;
						
						for(inx=mAvgHPP=0;inx<OUTQUEUEENTRIES;inx++)
						{
							mOutQueueSizes[inx]=0;
						}
						
						// so ditch outgoing
						mHPPOutQueue->FreeType(_mt_voicePacket);
						
						InterlockedIncrement(&mDirty);
					}
				}
			}
			
			if(!mDirty)
			{
				return;
			}
			
			InterlockedExchange(&mDirty, 0);
		
			_itoa(mRTTms, t, 10);
			
			sprintf(s, "%d\r%d\r%d\r%sms\r%d%%",
						mPacketsSent, mGoodPackets, mBadPackets,
						t, mGSMfull);
						
			mInfo1.SetWindowText(s);
		
			_itoa(mSoundOutput->GetNumPlaying(), t, 10);
			
			sprintf(s, "%s\r%d\r%d\r%dms\r%d",
						t, mOutputUnderflows, mOverflows, mJitter, mBandwidth);
						
			mInfo2.SetWindowText(s);
		
			mStatusPane.UpdateStatus();
		
			break;
		}
	}

	CView::OnTimer(nIDEvent);
}

void 
CPFWindow::OnMouseMove(UINT nFlags, CPoint point) 
{
	//This uses the mouse movements for random data.  The nFlags
	//as a general rule will equal 0, but will allow an extra little
	//bit of randomness (no pun intended) if the user is clicking or
	//shifting or what-have-you-ing.
	randPoolAddBytes((uchar *) &nFlags, sizeof(nFlags));
	randPoolAddBytes((uchar *) &point, sizeof(point));
	
	CView::OnMouseMove(nFlags, point);
}

int 
CPFWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	ModifyStyleEx(0, WS_EX_CONTROLPARENT, SWP_NOSIZE);

	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	//Set up the Timer so that we can get random data from the system timers
	if(!SetTimer(RANDOM_DATA_TIMER, 350, NULL))
		pgp_errstring("Error Creating Random Data Timer");
	if(!SetTimer(DEBUG_TIMER, 1000, NULL))
		pgp_errstring("Error Creating Debug Timer");
	
	return 0;
}

void CPFWindow::OnFileAnswer() 
{
	if((mState == _cs_uninitialized || mState == _cs_listening) &&
		(gPGFOpts.popt.connection == _cme_Serial))
	{
		if(mState == _cs_listening)
			mControlThread->GetTransport()->AbortSync();
		mControlQueue->SendUnique(_mt_answer);
	}
}

void CPFWindow::OnUpdateFileAnswer(CCmdUI* pCmdUI) 
{
	if((mState == _cs_uninitialized || mState == _cs_listening) &&
		(gPGFOpts.popt.connection == _cme_Serial))
		pCmdUI->Enable(1);
	else
		pCmdUI->Enable(0);
}


void CPFWindow::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnClose();
}

void CPFWindow::Die(void)
{
	ulong start;

	mDead = TRUE;
	mControlThread->AbortSync(TRUE);
	while(!mControlResult)
		  Sleep(0);
	delete mControlQueue;

	mSoundInput->AbortSync();
	mSoundOutput->GetPlayQueue()->SendUnique(_mt_quit);

	start = pgp_getticks();
	while((!mSoundInputResult || !mSoundOutputResult) &&
			pgp_getticks() - start < 15000)
		Sleep(0);
	pgpAssert(mControlResult);
 	pgpAssert(mSoundInputResult);
	pgpAssert(mSoundOutputResult);
	mAuthWindow.DestroyWindow();
	mFTP.DestroyWindow();
}

//This handler is needed to enable dialog-like handling of things like
//tabs.
BOOL CPFWindow::PreTranslateMessage(MSG* msg) 
{
	//Ugly hack:  We catch WM_KEY here when they hit "ENTER" and use it to fake
	//like the Connect button has been hit.
	if(msg->message == WM_KEYDOWN)
	{
		if(msg->wParam == '\n' ||
			msg->wParam == '\r')
		{
			this->PostMessage(WM_COMMAND, IDI_CONNECTBUTTON);
			return(TRUE);
		}
			
	}

	//Basically, IsDialogMessage handles any dialog messages.
    if(IsDialogMessage(msg))
        return TRUE; //If it was a dialog message, it's been handled, we're done.
    else
        return CWnd::PreTranslateMessage(msg); //Else do the normal thing
}

void
CPFWindow::SetRTTime(ulong rttVal)
{
	InterlockedExchange(&mRTTms, rttVal);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::SetJitter(long jitter)
{
	InterlockedExchange(&mJitter, jitter);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::SetBandwidth(long bandwidth)
{
	InterlockedExchange(&mBandwidth, bandwidth);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::GotPacket(Boolean good)
{
	if(!good)
		InterlockedIncrement(&mBadPackets);
	else
		InterlockedIncrement(&mGoodPackets);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::SentPacket(void)
{
	InterlockedIncrement(&mPacketsSent);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::SetGSMTimes(long full, long lite, long fullDec, long liteDec)
{
	InterlockedExchange(&mGSMfull, full/10);
	InterlockedExchange(&mGSMlite, lite/10);
	InterlockedExchange(&mGSMfullDec, fullDec/10);
	InterlockedExchange(&mGSMliteDec, liteDec/10);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::InitCall(void)
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
	for(inx=0;inx<OUTQUEUEENTRIES;inx++)
		mOutQueueSizes[inx] = 0;
}

void
CPFWindow::Underflow()
{
	InterlockedIncrement(&mOutputUnderflows);
	InterlockedExchange(&mDirty, 1);
}

void
CPFWindow::OverflowOut(short numLost)
{
	mOverflows += numLost;
	InterlockedExchange(&mDirty, 1);
}

BOOL CPFWindow::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	int pos;

	switch( wParam )
	{
		case IDI_VOLUMESLIDER:
			pos = mVolumeSlider.GetPos();
			mSoundOutput->SetVolume( (unsigned long)pos * (unsigned long)100 );
			break;
		case IDI_GAINSLIDER:
			pos = mGainSlider.GetPos();
			mSoundInput->SetGain( pos );
			break;
	}
	
	return CView::OnNotify(wParam, lParam, pResult);
}

BEGIN_MESSAGE_MAP(CPFWindow, CView)
	//{{AFX_MSG_MAP(CPFWindow)
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_FILE_ANSWER, OnFileAnswer)
	ON_UPDATE_COMMAND_UI(ID_FILE_ANSWER, OnUpdateFileAnswer)
	ON_COMMAND(ID_VIEW_STATUS_INFO, OnViewStatusInfo)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_INFO, OnUpdateViewStatusInfo)
	ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PREFERENCES, OnUpdateEditPreferences)
	ON_COMMAND(ID_VIEW_ENCODING_DETAILS, OnViewEncodingDetails)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENCODING_DETAILS, OnUpdateViewEncodingDetails)
	ON_COMMAND(ID_TRANSFER_FILE, OnTransferFile)
	ON_UPDATE_COMMAND_UI(ID_TRANSFER_FILE, OnUpdateTransferFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

