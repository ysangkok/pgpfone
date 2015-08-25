/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CTriThreshold.cpp,v 1.4 1999/03/10 02:41:32 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "PGPFone.h"
#include "CTriThreshold.h"
#include "CLevelMeter.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TRIANGLE_TIMER_ID             2
#define DRAG_THRESHOLD_AMOUNT         6
//How many milliseconds between simulated clicks if the user holds down
//the left mouse button:
#define MS_BETWEEN_CONSTANT_CLICKS  250

static void 
MakeTriangleMaskBitmap(CBitmap *bitmap, CPaintDC *dc, CRect *rect);

void DrawMaskedBitmap(CDC* pDC, UINT bitmapID, UINT maskID, POINT leftTop);

CTriThreshold::CTriThreshold()
{
	mPosition = 0;
	mSoundInput = NULL;
	mInDrag = mInClick = FALSE;
	mOldCaptureWindow = NULL;
}

CTriThreshold::~CTriThreshold()
{
}

BEGIN_MESSAGE_MAP(CTriThreshold, CButton)
	//{{AFX_MSG_MAP(CTriThreshold)
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

BOOL 
CTriThreshold::Create(LPCTSTR lpszCaption, 
					  DWORD dwStyle, 
					  CWnd* pParentWnd, 
					  UINT nID,
					  CLevelMeter *levelMeter)
{
	CRect workRectangle = levelMeter->LevelMeterRectangle();
	BITMAP bm;
	CBitmap triangleBitmap;

	//Load the triangle bitmap:		
	triangleBitmap.LoadBitmap(IDB_TRIANGLE);

	//Figure out how big the triangle bitmap is so that we can properly size
	//ourselves:
	triangleBitmap.GetObject(sizeof(bm), &bm);
	mTriangleWidth = bm.bmWidth;
	mTriangleHeight = bm.bmHeight;
	
	//We want to be wider than, but much thinner than, the meter:
	//(Wider than because we need to hang a little off the left and right
	//edges so that the "point" of the triangle can hit the ends of the
	//meter)
	CSize rightBottomOffset((workRectangle.Width() + (mTriangleWidth / 2)), 
							mTriangleHeight);

	//Move us to 1 pixel below the meter, and half our width to the left of 
	//it:
	workRectangle.OffsetRect((-(mTriangleWidth / 2)) + 2, 
							 workRectangle.Height() + 1);

	//Finally, add them all together:
	CRect ourRectangle(workRectangle.TopLeft(), rightBottomOffset);

	//Additionally, we need to know how far right we can go so that the user
	//doesn't exceed that value.
	mMaxPosition = ourRectangle.Width() - mTriangleWidth;

	//Set it up so that one click to the right of the slider makes it move
	//20% of its length (more or less):
	mClickMoveAmount = mMaxPosition / 5;

	return(CButton::Create(lpszCaption, 
						   dwStyle, 
						   ourRectangle, 
						   pParentWnd, 
						   nID));
}

void 
CTriThreshold::OnPaint() 
{
	CPaintDC dc(this);
	CRect rect;
	CDC memDC;
	CPoint leftTop(mPosition, 0);

	//Get the rectangle we're supposed to paint:
	GetClientRect(&rect);
	dc.IntersectClipRect(&rect);

	memDC.CreateCompatibleDC(&dc);

	//Fill in the background with the same color as the dialog:
	dc.FillSolidRect(&rect, dc.GetNearestColor(GetSysColor(COLOR_3DFACE)));

	//Actually draw the triangle:
	//Who WAS that masked Bitmap?
	DrawMaskedBitmap(&dc, IDB_TRIANGLE, IDB_TRIANGLE_MASK, leftTop);
}


//Takes an argument less than mMaxPosition and uses it to move the triangle
//(IE, it validates stuff and invalidates the region)
void CTriThreshold::MoveTriangle(long position)
{
	pgpAssert(position >= 0);
	pgpAssert(position <= mMaxPosition);

	if(mPosition != position)
	{
		mPosition = position;
		Invalidate(FALSE);
	}
	//We actually give the new level to SoundInput when the user releases
	//the right mouse button.
}

void CTriThreshold::OnMouseMove(UINT nFlags, CPoint point) 
{
	long NewPosition;
	//If they're holding down the left mouse button:
	if(mInDrag)
	{
		//It shouldn't be possible for the left button to be up and mInDrag
		//to be true...
		pgpAssert(nFlags & MK_LBUTTON);

		//Get the new position:
		NewPosition = point.x - (mTriangleWidth / 2);

		//Force it within legal values:
		if(NewPosition < 0)
			NewPosition = 0;
		if(NewPosition > mMaxPosition)
			NewPosition = mMaxPosition;

		MoveTriangle(NewPosition);
	}

	//Save the last known mouse position:
	mLastKnownMousePosition = point;
		
	CButton::OnMouseMove(nFlags, point);
}

long 
CTriThreshold::CalculateTrianglePositionFromDelta(long delta,
												   long MaxMoveAmount)
{
	long NewPosition;

	//Force the delta into legal values:
	//(This isn't an assertion because the delta is probably derrived from
	//user input, and may be outside the legal range)
	if(abs(delta) > MaxMoveAmount)
	{
		if(delta > 0)
			delta = MaxMoveAmount;
		else
			delta = -MaxMoveAmount;
	}

	//Calculate the new position:
	NewPosition = mPosition + delta;

	//If we've overflowed the slider, correct:
	if(NewPosition > mMaxPosition)
		NewPosition = mMaxPosition;
	else if(NewPosition < 0)
		NewPosition = 0;

	return(NewPosition);	
}

/*This handles the left-button down clicking.  Basically, if they click
 *within ten pixels of the current location, we move straight to the pointer.
 *If not, we move ten pixels closer to the pointer.  This behaviour is
 *modeled on the sliders.
 */
void CTriThreshold::OnLButtonDown(UINT nFlags, CPoint point) 
{
	long ClickPosition, ClickDelta, NewPosition;
	CWnd *oldCaptureWindow;

	//We should never receive a button_down message while we're dragging -
	//if so, the timer has probably gone awry:
	pgpAssert(!mInDrag);

	mClickPosition = point;
	//First, figure out where they clicked:
	ClickPosition = point.x - (mTriangleWidth / 2);

	//Figure out what the change was:
	ClickDelta = ClickPosition - mPosition;

	NewPosition = CalculateTrianglePositionFromDelta(ClickDelta, 
													 mClickMoveAmount);

	//Capture the mouse so that we still get the messages if the user moves
	//the mouse off the slider:
	//(We check oldCaptureWindow so that, if we get multiple clicks from a
	//click-and-hold, we don't make OURSELVES the old capture window, which
	//ends up with nothing else in the application getting mouse moves)
	oldCaptureWindow = SetCapture();
	if(oldCaptureWindow != this)
		mOldCaptureWindow = oldCaptureWindow;

	//If it's moved more than a couple of pixels, it's a "click."
	//If it's moved less, it's a "drag."
	if((abs(ClickDelta) <= DRAG_THRESHOLD_AMOUNT) && !mInClick)
	{
		mInDrag = TRUE;
		mInClick = FALSE;
	}
	else if(!mInClick) //It's a click; if mInClick, we've already done this
	{
		//This sets up a timer so that a "click and hold" can move the
		//slider multiple times.  We update every quarter second in this
		//case.  The LButtonUp does the KillTimer.
		SetTimer(TRIANGLE_TIMER_ID, MS_BETWEEN_CONSTANT_CLICKS, NULL);
		mInClick = TRUE;
		mInDrag = FALSE;
	}

	//Do the deed:
	MoveTriangle(NewPosition);
}

void 
CTriThreshold::OnLButtonUp(UINT nFlags, CPoint point) 
{
	//In case we were doing a "click and hold", kill the timer:
	KillTimer(TRIANGLE_TIMER_ID);

	//Give up our mouse capture, if we were capturing it:
	ReleaseCapture();

	//If someone else had the capture on the lbuttondown, give it back:
	if(mOldCaptureWindow)
	{
		mOldCaptureWindow->SetCapture();
		mOldCaptureWindow = NULL;
	}

	//Turn the percent the triangle is across the valid space into
	//a number between 0 and MAX_THRESHOLD (from CSoundInput.h):
	gPGFOpts.popt.siThreshold = (short)
		(mPosition * (MAX_THRESHOLD / mMaxPosition));

	//Now, set the internal threshold:
	mSoundInput->SetThreshold(gPGFOpts.popt.siThreshold);

	//Mouse is up; we're not clicking or dragging:
	mInDrag = mInClick = FALSE;
}

//Only time we should have a timer is if the user is holding down the
//mouse button.  We want to move the slider in the direction of the pointer
//in this case.
void 
CTriThreshold::OnTimer(UINT nIDEvent) 
{
	//Basically, just pretend they clicked, again:
	OnLButtonDown(MK_LBUTTON, mClickPosition);
	
	CButton::OnTimer(nIDEvent);
}

//Basically, this makes a black bitmap as big as rect.
static void 
MakeTriangleMaskBitmap(CBitmap *bitmap, CPaintDC *dc, CRect *rect)
{
	CBrush brush(dc->GetNearestColor(RGB(0, 0, 0)));
	CPen pen(PS_SOLID, 1, dc->GetNearestColor(RGB(0, 0, 0)));
	CDC maskDC;

	//Make the bitmap we'll work on:
	pgpAssert(maskDC.CreateCompatibleDC(dc));
	pgpAssert(bitmap->CreateCompatibleBitmap(dc, 
											  rect->Width(), 
											  rect->Height()));
	pgpAssert(maskDC.SelectObject(bitmap));

	//Basically, all we want is a black bitmap:
	maskDC.SelectObject(&brush);
	maskDC.SelectObject(&pen);
	maskDC.Rectangle(0, 0, rect->Width(), rect->Height());
}

//This is so that rapid clicking moves the pointer quickly instead of stalling
//on all the double-click messages.
void 
CTriThreshold::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	OnLButtonDown(nFlags, point);
	OnLButtonUp(nFlags, point);
}

//None of these button handlers are actually used, but we need them to keep
//the defaults from being called, which will repaint us as a button:
void 
CTriThreshold::OnMButtonDblClk(UINT nFlags, CPoint point) 
{
}

void 
CTriThreshold::OnMButtonDown(UINT nFlags, CPoint point) 
{
}

void 
CTriThreshold::OnMButtonUp(UINT nFlags, CPoint point) 
{
}

void 
CTriThreshold::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
}

void 
CTriThreshold::OnRButtonDown(UINT nFlags, CPoint point) 
{
}

void 
CTriThreshold::OnRButtonUp(UINT nFlags, CPoint point) 
{
}

