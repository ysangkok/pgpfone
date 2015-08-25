/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPGPFone.cpp,v 1.8 1999/03/10 02:36:45 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "afxole.h"
#include "CPGPFone.h"

#include "CPGPFoneFrame.h"
#include "PGPFoneUtils.h"
#include "CPacketWatch.h"

#include "winsock2.h"
#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PGPFoneOptions	gPGFOpts;
BOOLEAN			gHasWinsock;

/////////////////////////////////////////////////////////////////////////////
// CPGPFoneApp

BEGIN_MESSAGE_MAP(CPGPFoneApp, CWinApp)
	//{{AFX_MSG_MAP(CPGPFoneApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGPFoneApp construction

CPGPFoneApp::CPGPFoneApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPGPFoneApp object

CPGPFoneApp theApp;

const RECT sDummyRect =		{0,0,0,0};

/////////////////////////////////////////////////////////////////////////////
// CPGPFoneApp initialization

BOOL CPGPFoneApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		gHasWinsock = FALSE;
	}
	else
		gHasWinsock = TRUE;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	pgp_meminit();
	InitSafeAlloc();
/*#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif*/

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	AfxOleInit();  //Initialize Ole DLLs so drag-n-drop will work

	m_pMainWnd = mPGPFWindow = (CPGPFoneFrame *)
		RUNTIME_CLASS(CPGPFoneFrame)->CreateObject();

	if (!mPGPFWindow->LoadFrame(IDR_MAINFRAME,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			NULL, NULL))
	{
		TRACE0("Warning: couldn't load a frame.\n");
		// frame will be deleted in PostNcDestroy cleanup
		return NULL;
	}
	
	/*if(!mPGPFWindow->Create(NULL, "PGPfone",
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			sDummyRect, NULL, MAKEINTRESOURCE(IDR_MAINFRAME),
			WS_EX_CLIENTEDGE, NULL))
	{
		TRACE0("Warning: couldn't load a frame.\n");
		// frame will be deleted in PostNcDestroy cleanup
		return NULL;
	}*/
	
	mPGPFWindow->ShowWindow(TRUE);
	mPGPFWindow->UpdateWindow();
	
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

#define ADDHEIGHT	0
#define XINCREMENT	6
#define YINCREMENT	10
#define EDGEWIDTH	6

#define VERSIONX	32
#define VERSIONY	26


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual void OnOK();
	afx_msg void OnWwwlink();
	afx_msg void OnCredits();
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL 		showCredits;
	CBitmap		aboutBitmap;
	CBitmap		creditsBitmap;
	CBitmap*	displayBitmap;
	CButton*    creditsButton;
	CString 	weblink;
	INT 		numBits;
	COLORREF	crVersionTextColor;
	INT			aboutIndex;
	INT			creditsIndex;
	INT			xSize;
	INT 		ySize;
	INT			xPos;
	INT			yPos;
	
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	showCredits = FALSE;
	numBits = 0;
	
	weblink.LoadString(IDS_WEBLINK);
	
	CDC* dc;
	
	dc = GetDC();
	
	if(dc)
	{
		numBits = dc->GetDeviceCaps(BITSPIXEL) * 
					dc->GetDeviceCaps (PLANES);
					
		ReleaseDC(dc);
	}
	
	if (numBits <= 1) 
	{
		aboutIndex = IDB_ABOUT1;
		creditsIndex = IDB_CREDITS1;
		crVersionTextColor = RGB (255,255,255);
	}
	else if (numBits <= 4) 
	{
		aboutIndex = IDB_ABOUT4;
		creditsIndex = IDB_CREDITS4;
		crVersionTextColor = RGB (0,0,0);
	}
	else 
	{
		aboutIndex = IDB_ABOUT8;
		creditsIndex = IDB_CREDITS8;
		crVersionTextColor = RGB (0,0,0);
	}

	if(!aboutBitmap.LoadBitmap(aboutIndex))
	{
		AfxMessageBox("Failed to load About Bitmap!");
	}
	
	displayBitmap = &aboutBitmap;
	
	if(!creditsBitmap.LoadBitmap(creditsIndex))
	{
		AfxMessageBox("Failed to load Credits Bitmap!");
	}
	
	BITMAP bmp;
	
	aboutBitmap.GetBitmap(&bmp);
	
	xSize = bmp.bmWidth;
	ySize = bmp.bmHeight;
	
	xPos = (GetSystemMetrics (SM_CXFULLSCREEN) - xSize - EDGEWIDTH) / 2;
	yPos = (GetSystemMetrics (SM_CYFULLSCREEN) - ySize) / 2;
		
	SetWindowPos (	&wndTop, 
					xPos, 
					yPos, 
					xSize + 6, 
					ySize + ADDHEIGHT, 
					0);
					
	CButton* button;
	RECT rect;
	CFont* font = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	int x,y;
	
	button = (CButton*) GetDlgItem(IDOK);
	
	button->SetFont(font);
	
	button->GetClientRect(&rect);
	
	x = xSize - XINCREMENT - rect.right;
	y = ySize + ADDHEIGHT - YINCREMENT - rect.bottom;
	
	button->SetWindowPos (	NULL, 
							x, 
							y, 
							0, 
							0, 
							SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	
	button = (CButton*) GetDlgItem(IDC_WWWLINK);
	
	button->SetFont(font);
	
	button->GetClientRect(&rect);
	
	button->SetWindowText("&" + weblink);
	
	x = x - XINCREMENT - rect.right;
	y = ySize + ADDHEIGHT - YINCREMENT - rect.bottom;
	
	button->SetWindowPos (	NULL, 
							x, 
							y, 
							0, 
							0, 
							SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	
	button = (CButton*) GetDlgItem(IDC_CREDITS);
	
	creditsButton = button;
	
	button->SetFont(font);
	
	button->GetClientRect(&rect);
	
	x = x - XINCREMENT - rect.right;
	y = ySize + ADDHEIGHT - YINCREMENT - rect.bottom;
	
	button->SetWindowPos (	NULL, 
							x, 
							y, 
							0, 
							0, 
							SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
					
	
							
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnPaint() 
{
	//AfxMessageBox("Here!");
	CPaintDC dc(this); // device context for painting
	
	CDC memdc;
	
	memdc.CreateCompatibleDC((CDC*)&dc);
	
	memdc.SelectObject(displayBitmap);
	
	dc.BitBlt (0, 0, xSize, ySize, &memdc, 0, 0, SRCCOPY);

	// Do not call CDialog::OnPaint() for painting messages
}

void CAboutDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CAboutDlg::OnWwwlink() 
{
	OnOK();
	
	if ((int)ShellExecute (NULL, "open", weblink, NULL, 
				  "C:\\", SW_SHOWNORMAL) <= 32) 
	{
		AfxMessageBox (IDS_BROWSERERROR, MB_OK|MB_ICONHAND);
	}
	
}

void CAboutDlg::OnCredits() 
{
	CString text;
	
	showCredits = !showCredits;
	
	displayBitmap = (showCredits ? &creditsBitmap : &aboutBitmap);

	
	text.LoadString((showCredits ? IDS_INFOTEXT : IDS_CREDITSTEXT));
	
	creditsButton->SetWindowText(text);
	
	RedrawWindow();
	
}


void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_WWWLINK, OnWwwlink)
	ON_BN_CLICKED(IDC_CREDITS, OnCredits)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CPGPFoneApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

int CPGPFoneApp::ExitInstance() 
{
	if(gHasWinsock)
		WSACleanup();
	DisposeSafeAlloc();
	
	return CWinApp::ExitInstance();
}

