/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CLevelMeter.cpp,v 1.4 1999/03/10 02:34:32 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CLevelMeter.h"
#include "CPGPFone.h"
#include "PGPfone.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static void MakeBaseMeter(CBitmap *bitmap, CPaintDC *dc, CRect *rect);

#undef RANDOM_TEST
#undef TIMER_TEST

/////////////////////////////////////////////////////////////////////////////
// CLevelMeter
CLevelMeter::CLevelMeter()
{
	mInit = FALSE;
	mLevel = 0;
	mOn = FALSE;
}

CLevelMeter::~CLevelMeter()
{
}

BOOL 
CLevelMeter::Create(LPCTSTR lpszCaption, 
						 DWORD dwStyle, 
						 const POINT &leftTop, 
						 CWnd* pParentWnd, 
						 UINT nID,
						 short numLEDs,
						 short LEDWidth,
						 short delimiterWidth,
						 short LEDHeight)
{
	mNumLEDs = numLEDs;
	mLEDWidth = LEDWidth;
	mDelimiterWidth = delimiterWidth;
	mLEDHeight = LEDHeight;
	mLeftTop = leftTop;

	return(CButton::Create(lpszCaption, 
						   dwStyle, 
						   LevelMeterRectangle(), 
						   pParentWnd, 
						   nID));
}

void
CLevelMeter::SetLevel(long level)
{
	pgpAssert(level <= mNumLEDs);

	//Prevent run-time non-debug crashes:
	if(level > mNumLEDs)
		level = mNumLEDs;

	mLevel = level;
	Invalidate(TRUE);
}

void
CLevelMeter::SetStatus(short on)
{
	mOn = (BOOLEAN) on;
}

BEGIN_MESSAGE_MAP(CLevelMeter, CButton)
	//{{AFX_MSG_MAP(CLevelMeter)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CREATE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void 
CLevelMeter::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	CBitmap outputBitmap;
	CDC outputDC, workDC;

	// get the dimensions of the area we have to work with
	GetClientRect(&rect);
	dc.IntersectClipRect(&rect);

	//Create a bitmap and DC to do our work in
	outputDC.CreateCompatibleDC(&dc);
	workDC.CreateCompatibleDC(&dc);
	outputBitmap.CreateCompatibleBitmap(&dc, 
										rect.Width(), 
										rect.Height());
	outputDC.SelectObject(&outputBitmap);

	if(!mInit)
	{
		//We create all the possible bitmaps we need here, so that we
		//don't have to keep doing that over and over.
		MakeBaseMeter(&mBaseMeter, &dc, &rect);
		MakeLEDMeter(&mDarkMeter, 
					 &dc, 
					 &rect, 
					 dc.GetNearestColor(RGB(0, 0, 0)),
					 dc.GetNearestColor(RGB(0, 0, 0)),
					 dc.GetNearestColor(GetSysColor(COLOR_3DSHADOW)));
		MakeLEDMeter(&mLitMeter, 
					 &dc, 
					 &rect, 
					 dc.GetNearestColor(RGB(0, 255, 0)),
					 dc.GetNearestColor(RGB(255, 0, 0)),
					 dc.GetNearestColor(GetSysColor(COLOR_3DSHADOW)));
		MakeLEDMeter(&mGreyMeter, 
					 &dc, 
					 &rect, 
					 dc.GetNearestColor(GetSysColor(COLOR_3DFACE)),
					 dc.GetNearestColor(GetSysColor(COLOR_3DFACE)),
					 dc.GetNearestColor(GetSysColor(COLOR_3DSHADOW)));

#ifdef TIMER_TEST
		SetTimer(5, 1000, NULL);
		mOn = TRUE;
#endif
		mInit = TRUE;
	}

	/*There are three possible cases - a disabled meter, an empty
	 *meter (on but no sound input) and a lit meter (on with some
	 *sound input).  The second two cases are almost identical.
	 *The basic plan is to copy the dark meter in first (since we have
	 *to do that, anyway - the meter spends a lot more time empty than
	 *full), and then copy part of the lit meter (up to 100% of it) in
	 *as appropriate for the level.
	 */

	//First, put out the base meter, which we need for all cases:

	if(workDC.SelectObject(&mBaseMeter))
		outputDC.BitBlt(0, 
						0, 
						rect.Width(), 
						rect.Height(), 
						&workDC, 
						0, 
						0, 
						SRCCOPY);
	else
		pgp_errstring("SelectObject failed on mBaseMeter");

	if(mOn)
	{
		if(workDC.SelectObject(&mDarkMeter))
		{
			outputDC.BitBlt(2, 
							2, 
							rect.Width() - 4, 
							rect.Height() - 4, 
							&workDC, 
							0, 
							0, 
							SRCCOPY);
			if(mLevel)
				if(workDC.SelectObject(&mLitMeter))
				{
					outputDC.BitBlt(2,
									2,
									(mLevel * (mDelimiterWidth + mLEDWidth)),
									rect.Height() - 4,
									&workDC,
									0,
									0,
									SRCCOPY);
				}
				else
					pgp_errstring("SelectObject failed on mLitMeter");
		}
		else
			pgp_errstring("SelectObject failed on mDimMeter");
	}
	else //Display the greyed out meter.
	{
		if(workDC.SelectObject(&mGreyMeter))
		{
			outputDC.BitBlt(2, 
							2, 
							rect.Width() - 3, 
							rect.Height() - 4, 
							&workDC, 
							0, 
							0, 
							SRCCOPY);
		}
	}

	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &outputDC, 0, 0, SRCCOPY);
}

/*This makes the base meter, which is actually just a black rectangle
 *with the grey lines around it to make it 3D.
 */
static void 
MakeBaseMeter(CBitmap *bitmap, CPaintDC *dc, CRect *rect)
{
	CBrush brush(dc->GetNearestColor(GetSysColor(COLOR_3DFACE)));
	CPen pen(PS_SOLID, 1, dc->GetNearestColor(GetSysColor(COLOR_3DFACE)));

	CDC meterDC;
	CRect tempRect = *rect;

	//Make the bitmap we'll work on:

	meterDC.CreateCompatibleDC(dc);
	bitmap->CreateCompatibleBitmap(dc,rect->Width(),rect->Height());
	meterDC.SelectObject(bitmap);

	//The first step is to black out the background
	meterDC.SelectObject(&brush);
	meterDC.SelectObject(&pen);

	meterDC.Rectangle(0, 0, rect->Width(), rect->Height());

	//3D Controls have a left/top line which is dark grey, and a right/bottom
	//line which is white.  Inside of these is a left/top line that is black
	//and a right/bottom which is light grey.
	meterDC.Draw3dRect(rect, 
					   dc->GetNearestColor(GetSysColor(COLOR_3DSHADOW)), 
					   dc->GetNearestColor(GetSysColor(COLOR_3DHILIGHT)));

	//Shrink the rectangle by one so that we can do the next colors:
	tempRect.DeflateRect(1, 1);
	meterDC.Draw3dRect(tempRect,
					   dc->GetNearestColor(GetSysColor(COLOR_3DDKSHADOW)),
					   dc->GetNearestColor(GetSysColor(COLOR_3DLIGHT)));
}

/*This is a generic "make an LED meter" function.  All three meters
 *(grey, black and lit) are made by this function.
 */
void 
CLevelMeter::MakeLEDMeter(CBitmap *bitmap, 
						  CPaintDC *dc, 
						  CRect *rect,
						  COLORREF LEDColor,
						  COLORREF highLEDColor,
						  COLORREF delimiterColor)
{
	CBrush brush(delimiterColor);
	CPen pen(PS_SOLID, 1, delimiterColor), 
		 highPen(PS_SOLID, mLEDWidth, highLEDColor);
	CDC meterDC;
	short i;

	//Create the bitmap:
	meterDC.CreateCompatibleDC(dc);
	bitmap->CreateCompatibleBitmap(dc,rect->Width() - 3,rect->Height() - 4);
	// -4 is so we don't overwrite the decorations on the top and
	//bottom.

	meterDC.SelectObject(*bitmap);
	meterDC.SelectObject(&brush);
	meterDC.SelectObject(&pen);

	//First of all, make the background the color of the delimiters...
	meterDC.Rectangle(0, 0, rect->Width() - 3, rect->Height() - 4);

	//Create a pen in the color of the low LEDs (the high is taken care
	//of in the constructor, above) and select it
	pen.DeleteObject();
	pen.CreatePen(PS_SOLID, mLEDWidth, LEDColor);
	meterDC.SelectObject(&pen);

	for(i = 0; i <= mNumLEDs; ++i)
	{
		if(i == (mNumLEDs - 2))
			meterDC.SelectObject(&highPen); //Go "red" if we're at the top

		meterDC.MoveTo((i * (mLEDWidth + mDelimiterWidth)) + mDelimiterWidth + 1, 0);
		meterDC.LineTo((i * (mLEDWidth + mDelimiterWidth)) + mDelimiterWidth + 1,
					   mLEDHeight);	
						
	}
}

/*Calculates the size of the current rectangle of the level meter, alone (not
 *including the triangle slider, etc.)
 */
CRect
CLevelMeter::LevelMeterRectangle(void)
{
	/*The +s are for the decorations on the edges.  The width is +3
	 *and not +4 as you'd expect because we don't want a trailing
	 *delimiter.
	 */
	CSize rightBottomOffset((mNumLEDs * (mLEDWidth + mDelimiterWidth) + 4 +
								mDelimiterWidth),
							mLEDHeight + 4);
	CRect rect(mLeftTop, rightBottomOffset);
	
	return(rect);
}

void CLevelMeter::OnTimer(UINT nIDEvent) 
{
#ifdef RANDOM_TEST
	static BOOLEAN firstRun = TRUE;

	if(firstRun)
	{
		srand((unsigned) time(NULL));
		firstRun = FALSE;
	}

	SetLevel(rand() % mNumLEDs);
#else
	SetLevel((mLevel + 1) % (mNumLEDs + 1));
#endif
	
	CButton::OnTimer(nIDEvent);
}

/*None of these button down/up handlers do anything - we just want to trap the
 *messages so the default handling doesn't occur.
 */
void CLevelMeter::OnLButtonDown(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnLButtonUp(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnMButtonDown(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnMButtonUp(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnRButtonDown(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnRButtonUp(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnMButtonDblClk(UINT nFlags, CPoint point) 
{
}

void CLevelMeter::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
}

