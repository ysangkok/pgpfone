/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CAuthWindow.cpp,v 1.4 1999/03/10 02:31:59 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"

#include "CPGPFone.h"
#include "PGPFone.h"
#include "CAuthWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const RECT sTitleRect =		{10,8,140,53};
const RECT sAuthRect =		{10,57,140,120};

CAuthWindow::CAuthWindow()
{
}

CAuthWindow::~CAuthWindow()
{
}

BEGIN_MESSAGE_MAP(CAuthWindow, CWnd)
	//{{AFX_MSG_MAP(CAuthWindow)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int
CAuthWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	mPFont.CreateFont(-9,0,0,0,0,0,0,0,0,0,0,0,0,"Small Fonts");
	mPBFont.CreateFont(-10,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"MS Sans Serif");
	mTitleStatic.Create("To assure yourself of privacy, "
						"read aloud to confirm that the "
						"following words match those on "
						"the other end.",
						WS_CHILD|WS_VISIBLE|SS_CENTER,
						sTitleRect,this,
						1000);
	mTitleStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPFont, FALSE);
	mAuthStatic.CreateEx(WS_EX_CLIENTEDGE,"STATIC","",
							WS_CHILD|WS_VISIBLE|SS_CENTER,
							sAuthRect.left,
							sAuthRect.top,
							sAuthRect.right-sAuthRect.left,
							sAuthRect.bottom-sAuthRect.top,
							GetSafeHwnd(),0);
	mAuthStatic.SendMessage(WM_SETFONT, (ulong)(HFONT)mPBFont, FALSE);
	return 0;
}


void
CAuthWindow::OnClose() 
{
	ShowWindow(SW_HIDE);
	CWnd::OnClose();
}

void
CAuthWindow::SetAuthWords(char *words)
{
	mAuthStatic.SetWindowText(words);
}

