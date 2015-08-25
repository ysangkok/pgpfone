/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CSoundLight.cpp,v 1.4 1999/03/10 02:41:13 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "PGPFone.h"
#include "CSoundLight.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FLICKER_TIMER      1000

#define LIGHT_HEIGHT          12
#define LIGHT_WIDTH           12

static void 
MakeLightBitmap(CBitmap *bitmap, CPaintDC *dc, CRect *rect, COLORREF color);


/////////////////////////////////////////////////////////////////////////////
// CSoundLight

CSoundLight::CSoundLight()
{
}

CSoundLight::~CSoundLight()
{
}

void 
CSoundLight::Switch(BOOLEAN isOn)
{
	if(mLightIsOn != isOn) //Don't do anything if we're being set to what we  
						   //already are
	{
		mLightIsOn = isOn;
		Invalidate(TRUE);
	}
}
	
void 
CSoundLight::Toggle(void)
{
	mLightIsOn = !mLightIsOn;
	Invalidate(TRUE);
}

void 
CSoundLight::Flicker(long msToFlicker)
{
	Toggle();
	Invalidate(TRUE);
	SetTimer(FLICKER_TIMER, msToFlicker, NULL);
}

BOOLEAN 
CSoundLight::GetStatus(void)
{
	return(mLightIsOn);
};	

BEGIN_MESSAGE_MAP(CSoundLight, CButton)
	//{{AFX_MSG_MAP(CSoundLight)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSoundLight message handlers

BOOL CSoundLight::Create(LPCTSTR lpszCaption, 
						 DWORD dwStyle, 
						 CWnd* pParentWnd, 
						 UINT nID,
						 CLevelMeter *levelMeter,
						 COLORREF litColor,
						 BOOLEAN isOn)
{
	CRect workRectangle = levelMeter->LevelMeterRectangle();

	//Our rectangle should be 5 pixels right of the end of the level meter,
	//and halfway between the top and the bottom.
	workRectangle.OffsetRect(workRectangle.Width() + 5, 
							 (workRectangle.Height() / 2) - 
								(LIGHT_HEIGHT / 2));

	CSize rightBottomOffset(LIGHT_WIDTH, LIGHT_HEIGHT); 
							
	CRect ourRectangle(workRectangle.TopLeft(), rightBottomOffset);

	mLightIsOn = isOn;
	mLightColor = litColor;

	return(CButton::Create(lpszCaption, 
						   dwStyle, 
						   ourRectangle, 
						   pParentWnd, 
						   nID));
}

void 
CSoundLight::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	CDC workDC;
	static BOOLEAN mInit = FALSE;

	// get the dimensions of the area we have to work with
	GetClientRect(&rect);
	dc.IntersectClipRect(&rect);

	//Create a DC to do our work in
	workDC.CreateCompatibleDC(&dc);

	if(!mInit)
	{
		//We create the bitmap we need here, so that we don't have to keep
		//doing it every repaint.
		MakeLightBitmap(&mOnBitmap, &dc, &rect, mLightColor);
		MakeLightBitmap(&mOffBitmap, 
						&dc, 
						&rect, 
						RGB(0, 0, 0));

#ifdef TIMER_TEST
		SetTimer(5, 1000, NULL);
		mOn = TRUE;
#endif
		mInit = TRUE;
	}

	if(mLightIsOn)
		workDC.SelectObject(mOnBitmap);
	else
		workDC.SelectObject(mOffBitmap);

	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &workDC, 0, 0, SRCCOPY);
}

//The only times we should have timers is for testing or if the user
//has requested a flicker.
void 
CSoundLight::OnTimer(UINT nIDEvent) 
{
	switch (nIDEvent)
	{
		case FLICKER_TIMER:
			Toggle();
			KillTimer(FLICKER_TIMER);
			break;
	}
}

//None of these button handlers are actually used, but we need them to keep
//the defaults from being called, which will repaint us as a button:
void CSoundLight::OnLButtonDown(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnLButtonUp(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnMButtonDblClk(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnMButtonDown(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnMButtonUp(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnRButtonDown(UINT nFlags, CPoint point) 
{
}

void 
CSoundLight::OnRButtonUp(UINT nFlags, CPoint point) 
{
}

static void 
MakeLightBitmap(CBitmap *bitmap, CPaintDC *dc, CRect *rect, COLORREF color)
{
	CBrush brush(dc->GetNearestColor(GetSysColor(COLOR_3DFACE)));
	CBrush colorBrush(dc->GetNearestColor(color));
	CPen pen(PS_SOLID, 1, dc->GetNearestColor(GetSysColor(COLOR_3DFACE)));
	CDC lightDC;
	CRect workRect;

	//Make the bitmap we'll work on:
	lightDC.CreateCompatibleDC(dc);
	bitmap->CreateCompatibleBitmap(dc,
							rect->Width(), 
							rect->Height());
	lightDC.SelectObject(bitmap);

	//The first step is to blank out the background
	lightDC.SelectObject(&brush);
	lightDC.SelectObject(&pen);

	lightDC.Rectangle(0, 0, rect->Width(), rect->Height());

	//Next, get a black pen and make a circle...
	pen.DeleteObject();
	pen.CreatePen(PS_SOLID, 1, dc->GetNearestColor(RGB(0, 0, 0)));
	lightDC.SelectObject(&pen);
	lightDC.Ellipse(rect);
	workRect = *rect;
	workRect.DeflateRect(LIGHT_WIDTH / 4, LIGHT_HEIGHT / 4);
	lightDC.Ellipse(workRect);

	//Last, fill it with the appropriate color:
	//Next, get a black pen and make a circle...
	lightDC.SelectObject(colorBrush);
	lightDC.FloodFill(LIGHT_WIDTH / 2, 
					  LIGHT_HEIGHT / 2, 
					  dc->GetNearestColor(RGB(0, 0, 0)));
}

