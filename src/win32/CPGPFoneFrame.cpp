/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPGPFoneFrame.cpp,v 1.11 1999/03/10 02:36:52 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CPGPFone.h"

#include "CPGPFoneFrame.h"
#include "CPFWindow.h"
#include "CPacketWatch.h"
#include "CEncryptionStream.h"
#include "fastpool.h"
#include "CMessageQueue.h"
#include "CControlThread.h"
#include "CPFTransport.h"

#include <string.h>
#include <winreg.h>
#include <afxcmn.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const int cFoneFrameHeight =			350;
const int cFoneFrameWidth =				410;


/////////////////////////////////////////////////////////////////////////////
// CPGPFoneFrame

IMPLEMENT_DYNCREATE(CPGPFoneFrame, CFrameWnd)


/////////////////////////////////////////////////////////////////////////////
// CPGPFoneFrame construction/destruction

CPGPFoneFrame::CPGPFoneFrame()
{
}

CPGPFoneFrame::~CPGPFoneFrame()
{
	DoPrefs(1);
}

int CPGPFoneFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

BOOL CPGPFoneFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	//Set up the status bar:
	mStatusBar.Create(this);
	mStatusBar.setPhoneIconID(0);
	mStatusBar.setSecureIconID(0);

	CCreateContext cc;
	
	cc.m_pNewViewClass = RUNTIME_CLASS(CPFWindow);

	mCallView = (CPFWindow *)CreateView(&cc);
	
	SetActiveView((CView*)mCallView);
	
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

BOOL CPGPFoneFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	DoPrefs(0);

	cs.x = gPGFOpts.sysopt.windowPosX;
	cs.y = gPGFOpts.sysopt.windowPosY;
	cs.cx = cFoneFrameWidth;
	cs.cy = cFoneFrameHeight;
	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPGPFoneFrame diagnostics

#ifdef _DEBUG
void CPGPFoneFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CPGPFoneFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

void
CPGPFoneFrame::DoPrefs(short save)
{
	CString csPhoneOpts = "Phone";
	CString csSerialOpts = "Serial";
	CString csSystemOpts = "System";
	CString	csFileOpts = "File";
	
	CString csPIdentity = "Identity", csPEncryptor = "Encryptor",
			csPAEncryptor = "AltEncryptor", csPCoder = "Coder",
			csPACoder = "AltCoder", csPPrime = "Prime",
			csPPrimeMask = "AllowedPrimes", csPListen = "Listen",
			csPRing = "Ring", csPFullDuplex = "FullDuplex",
			csPIncoming = "IDIncoming", csPOutgoing = "IDOutgoing",
			csPUnencrypted = "IDUnencrypted", csPConnection = "Connect",
			csPCryptMask = "AllowedCryptors", csPsiGain = "Gain",
			csPsiThreshold = "Threshold";
			
	CString csSModemInit = "ModemInit", csSBaud = "Baud",
			csSPort = "Port", csSDTR = "DTR",
			csSMaxBaud = "MaxBaud";

	CString	csWindowPosX = "WindowPosX",
			csWindowPosY = "WindowPosY",
			csViewStatusInfo = "ViewStatusInfo",
			csViewEncodingDetails = "ViewEncodingDetails";
			
	CString csRecvDir = "RecvDir";
	
	CString csT;
	
	CWinApp* pa = AfxGetApp();
	
	if(save)
	{
		pa->WriteProfileString(csPhoneOpts, csPIdentity, gPGFOpts.popt.identity);
		pa->WriteProfileInt(csPhoneOpts, csPEncryptor, gPGFOpts.popt.prefEncryptor);
		pa->WriteProfileInt(csPhoneOpts, csPCryptMask, gPGFOpts.popt.cryptorMask);
		pa->WriteProfileInt(csPhoneOpts, csPCoder, gPGFOpts.popt.prefCoder);
		pa->WriteProfileInt(csPhoneOpts, csPACoder, gPGFOpts.popt.altCoder);
		pa->WriteProfileInt(csPhoneOpts, csPPrime, gPGFOpts.popt.prefPrime);
		pa->WriteProfileInt(csPhoneOpts, csPPrimeMask, gPGFOpts.popt.primeMask);
		pa->WriteProfileInt(csPhoneOpts, csPListen, gPGFOpts.popt.alwaysListen);
		pa->WriteProfileInt(csPhoneOpts, csPRing, gPGFOpts.popt.playRing);
		pa->WriteProfileInt(csPhoneOpts, csPFullDuplex, gPGFOpts.popt.fullDuplex);
		pa->WriteProfileInt(csPhoneOpts, csPIncoming, gPGFOpts.popt.idIncoming);
		pa->WriteProfileInt(csPhoneOpts, csPOutgoing, gPGFOpts.popt.idOutgoing);
		pa->WriteProfileInt(csPhoneOpts, csPUnencrypted, gPGFOpts.popt.idUnencrypted);
		if(gPGFOpts.popt.connection == _cme_Internet)
			pa->WriteProfileString(csPhoneOpts, csPConnection, "Internet");
		else
			pa->WriteProfileString(csPhoneOpts, csPConnection, "Serial");
		pa->WriteProfileInt(csPhoneOpts, csPsiGain, gPGFOpts.popt.siGain);
		pa->WriteProfileInt(csPhoneOpts, csPsiThreshold, gPGFOpts.popt.siThreshold);

		pa->WriteProfileString(csSerialOpts, csSModemInit, gPGFOpts.sopt.modemInit);
		pa->WriteProfileString(csSerialOpts, csSPort, gPGFOpts.sopt.port);
		pa->WriteProfileInt(csSerialOpts, csSBaud, gPGFOpts.sopt.baud);
		pa->WriteProfileInt(csSerialOpts, csSDTR, gPGFOpts.sopt.dtrHangup);
		pa->WriteProfileInt(csSerialOpts, csSMaxBaud, gPGFOpts.sopt.maxBaud);

		pa->WriteProfileInt(csSystemOpts, csWindowPosX, gPGFOpts.sysopt.windowPosX);
		pa->WriteProfileInt(csSystemOpts, csWindowPosY, gPGFOpts.sysopt.windowPosY);
		
		pa->WriteProfileInt(csSystemOpts, csViewStatusInfo, gPGFOpts.sysopt.viewStatusInfo);
		pa->WriteProfileInt(csSystemOpts, csViewEncodingDetails, gPGFOpts.sysopt.viewEncodingDetails);
		
		pa->WriteProfileString(csFileOpts, csRecvDir, gPGFOpts.fopt.recvDir);
	}
	else
	{
		strcpy(gPGFOpts.popt.identity, (LPCTSTR)pa->GetProfileString(csPhoneOpts, csPIdentity, "Unknown"));
		gPGFOpts.popt.prefEncryptor = pa->GetProfileInt(csPhoneOpts, csPEncryptor, _enc_cast);
		gPGFOpts.popt.cryptorMask = pa->GetProfileInt(csPhoneOpts, csPCryptMask, 0x0007);
		gPGFOpts.popt.prefCoder = pa->GetProfileInt(csPhoneOpts, csPCoder, 'GSM7');
		gPGFOpts.popt.altCoder = pa->GetProfileInt(csPhoneOpts, csPACoder, 'GS6L');
		gPGFOpts.popt.prefPrime = pa->GetProfileInt(csPhoneOpts, csPPrime, 3);
		gPGFOpts.popt.primeMask = pa->GetProfileInt(csPhoneOpts, csPPrimeMask, 0x3e);
		gPGFOpts.popt.alwaysListen = pa->GetProfileInt(csPhoneOpts, csPListen, 1);
		gPGFOpts.popt.playRing = pa->GetProfileInt(csPhoneOpts, csPRing, 1);
		gPGFOpts.popt.fullDuplex = pa->GetProfileInt(csPhoneOpts, csPFullDuplex, 0);
		gPGFOpts.popt.idIncoming = pa->GetProfileInt(csPhoneOpts, csPIncoming, 1);
		gPGFOpts.popt.idOutgoing = pa->GetProfileInt(csPhoneOpts, csPOutgoing, 1);
		gPGFOpts.popt.idUnencrypted = pa->GetProfileInt(csPhoneOpts, csPUnencrypted, 1);
		csT = pa->GetProfileString(csPhoneOpts, csPConnection, "Internet");
		if(csT == "Internet")
			gPGFOpts.popt.connection = _cme_Internet;
		else
			gPGFOpts.popt.connection = _cme_Serial;
		gPGFOpts.popt.siGain = pa->GetProfileInt(csPhoneOpts, csPsiGain, 0);
		gPGFOpts.popt.siThreshold = pa->GetProfileInt(csPhoneOpts, csPsiThreshold, 0);

		strcpy(gPGFOpts.sopt.modemInit, (LPCSTR)pa->GetProfileString(csSerialOpts, csSModemInit, "S0=0"));
		strcpy(gPGFOpts.sopt.port, (LPCSTR)pa->GetProfileString(csSerialOpts, csSPort, "COM1"));
		gPGFOpts.sopt.baud = pa->GetProfileInt(csSerialOpts, csSBaud, 38400);
		gPGFOpts.sopt.dtrHangup = pa->GetProfileInt(csSerialOpts, csSDTR, 1);
		gPGFOpts.sopt.maxBaud = pa->GetProfileInt(csSerialOpts, csSMaxBaud, 28800);

		gPGFOpts.sysopt.windowPosX = pa->GetProfileInt(csSystemOpts, csWindowPosX, 90);
		gPGFOpts.sysopt.windowPosY = pa->GetProfileInt(csSystemOpts,csWindowPosY, 80);
		
		gPGFOpts.sysopt.viewStatusInfo = pa->GetProfileInt(csSystemOpts, csViewStatusInfo, 0);
		gPGFOpts.sysopt.viewEncodingDetails = pa->GetProfileInt(csSystemOpts,csViewEncodingDetails, 0);
		
		char path[MAX_PATH];
		
		GetCurrentDirectory(sizeof(path), path);
		
		strcpy(gPGFOpts.fopt.recvDir, (LPCSTR)pa->GetProfileString(csFileOpts, csRecvDir, path));
	}
}

void
CPGPFoneFrame::OnClose() 
{
	RECT frame;
	int yCur, yMin;

	mCallView->Die();

	GetWindowRect(&frame);
	
	gPGFOpts.sysopt.windowPosX = frame.left;
	gPGFOpts.sysopt.windowPosY = frame.top;

	CFrameWnd::OnClose();
}


UINT CPGPFoneFrame::OnNcHitTest(CPoint point) 
{
	UINT area;

	area = CFrameWnd::OnNcHitTest(point);

	if(	area == HTLEFT			||
		area == HTRIGHT			||
		area == HTTOP			||
		area == HTBOTTOM		||
		area == HTBOTTOMLEFT	||
		area == HTBOTTOMRIGHT	||
		area == HTTOPLEFT		||
		area == HTTOPRIGHT		)

	{
		//AfxMessageBox("Here");
		area = HTBORDER;
	}
	
	return area;
}

BEGIN_MESSAGE_MAP(CPGPFoneFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CPGPFoneFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_NCHITTEST()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
END_MESSAGE_MAP()