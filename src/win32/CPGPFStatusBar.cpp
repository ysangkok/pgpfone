/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPGPFStatusBar.cpp,v 1.5 1999/03/10 02:36:54 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "PGPFone.h"
#include "CPGPFStatusBar.h"
#include "resource.h"
#include "PGPFWinUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ICON_WINDOW_WIDTH       30
#define ICON_WIDTH              16

#define STATUS_BAR_TEXT          0
#define STATUS_BAR_ICONS         1
#if 0
#define STATUS_BAR_ICON_PHONE    1
#define STATUS_BAR_ICON_SECURE   2
#endif

//These are place-holder values; the real ones are set in OnSize()
static unsigned int barIDs[] =
{
	ID_SEPARATOR,
	0,
};


/////////////////////////////////////////////////////////////////////////////
// CPGPFStatusBar

CPGPFStatusBar::CPGPFStatusBar()
{
	mPhoneIconID = mSecureIconID = 0;
}

CPGPFStatusBar::~CPGPFStatusBar()
{
}

BEGIN_MESSAGE_MAP(CPGPFStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CPGPFStatusBar)
	ON_WM_CREATE()
	ON_WM_NCHITTEST()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGPFStatusBar message handlers
BOOL 
CPGPFStatusBar::Create(CWnd* pParentWnd,
					   DWORD dwStyle, 
					   UINT nID)
{
	BOOL returnCode;
	if((returnCode = CStatusBar::Create(pParentWnd, dwStyle | CBRS_TOOLTIPS, nID)))
	{
		SetIndicators(barIDs, sizeof(barIDs) / sizeof(UINT));
		SetPaneInfo(STATUS_BAR_TEXT,
					ID_SEPARATOR,
					 GetPaneStyle(STATUS_BAR_TEXT) | SBPS_STRETCH,
				 0);
		SetPaneInfo(STATUS_BAR_ICONS, 
					0, 
					SBPS_OWNERDRAW,
					ICON_WINDOW_WIDTH);
		mPhoneTextID = IDS_STATUS_SECURE;
		mSecureTextID = IDS_STATUS_SECURE;
		mToolTipPhone.Create(this);
		mToolTipSecure.Create(this);

	}

	return(returnCode);
}

void CPGPFStatusBar::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CPoint leftTop;
	CRect rect(lpDrawItemStruct->rcItem);
	CRect toolRect;
	CDC dc;
	HICON hicon;
	static int is_first = 1;

	if(mPhoneIconID || mSecureIconID)
	{
		dc.Attach(lpDrawItemStruct->hDC);

		//First, draw the phone icon, if any:
		if(mPhoneIconID)
		{
			leftTop.x = rect.left + 2;
			leftTop.y = rect.top + 1;
						
			if((hicon = AfxGetApp()->LoadIcon(mPhoneIconID)))
				dc.DrawIcon(leftTop, hicon);

			//Register the tool-tip stuff:
			if(is_first)
			{
				is_first = 0;
#if 0
				toolRect.SetRect(leftTop.x, 
								 leftTop.y, 
								 leftTop.x + ICON_WIDTH,
								 rect.bottom - 1);
				mToolTipPhone.AddTool(this, 
									  mPhoneTextID, 
									  toolRect, 
									  STATUS_BAR_ICONS);
#endif
			}
			mToolTipPhone.Activate(TRUE);
		}
		else
			mToolTipPhone.Activate(FALSE);

		//Second, draw the secure icon, if any:
		if(mSecureIconID)
		{	
			leftTop.x = rect.left + 2 + ICON_WIDTH;
			leftTop.y = rect.top + 1;

			if((hicon = AfxGetApp()->LoadIcon(mSecureIconID)))
				dc.DrawIcon(leftTop, hicon);

			//Register the tool-tip stuff:
			toolRect.SetRect(leftTop.x, 
							 leftTop.y, 
							 leftTop.x + ICON_WIDTH,
							 rect.bottom - 1);
			mToolTipSecure.AddTool(this, 
								   mSecureTextID, 
								   toolRect, 
								   STATUS_BAR_ICONS);
			mToolTipSecure.Activate(TRUE);
		}
		else
			mToolTipSecure.Activate(FALSE);
		dc.Detach();
	}
}

void 
CPGPFStatusBar::setPhoneIconID(int phoneIconID) 
{
	mPhoneIconID = phoneIconID;
	Invalidate();
}

void 
CPGPFStatusBar::setSecureIconID(int secureIconID)
{
	mSecureIconID = secureIconID;
	Invalidate();
}

int 
CPGPFStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CStatusBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	EnableToolTips(TRUE);
	
	return 0;
}

int 
CPGPFStatusBar::OnToolHitTest(CPoint point, TOOLINFO* pTI)
{
	return(-1);
}


UINT CPGPFStatusBar::OnNcHitTest(CPoint point) 
{
	UINT area;

	area = CStatusBar::OnNcHitTest(point);

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
	
	return HTBORDER;
}
