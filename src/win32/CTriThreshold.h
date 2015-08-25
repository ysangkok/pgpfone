/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CTriThreshold.h,v 1.4 1999/03/10 02:41:35 heller Exp $
____________________________________________________________________________*/
#ifndef CTRITHRESHOLD_H
#define CTRITHRESHOLD_H

#include "CSoundInput.h"

class CTriThreshold : public CButton
{
// Construction
public:
	CTriThreshold();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTriThreshold)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTriThreshold();

	void SetSoundInput(CSoundInput *si)	{	mSoundInput = si;	}
	virtual BOOL 	Create(LPCTSTR lpszCaption, 
									   DWORD dwStyle, 
									   CWnd* pParentWnd, 
									   UINT nID,
									   CLevelMeter *levelMeter);
	// Generated message map functions
protected:
	//{{AFX_MSG(CTriThreshold)
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	long		mPosition;			//Triangle's position, in pixels
	CSoundInput	*mSoundInput;		//Object we line up off of
	CWnd        *mOldCaptureWindow; //Window that had the mouse captured
	CPoint      mClickPosition;     //Place the user clicked for click-and-hold
	CPoint      mLastKnownMousePosition;
									//Last place the mouse was known to be for
									//the simulated clicks of click-and-hold
	BOOL        mInDrag;			//Set if the slider is being dragged
	BOOL        mInClick;           //Set if we're "in" a click (ie, between
									//button down and button up)
	long		mMaxPosition;       //Maximum valid rightward position for the
									//slider
	long        mClickMoveAmount;	//Number of pixels a click to one side of
									//slider causes it to jump

	//Info about the bitmap:
	long		mTriangleWidth, mTriangleHeight;

	//Takes an argument between MIN_SLIDER_VAL and MAX_SLIDER_VAL and
	//uses it to move the triangle:
	void 		MoveTriangle(long position);

	//Internal funcion to allow you to do things, basically, like
	//"Move the triangle right 10 pixels".  MaxMoveAmount is so that you
	//don't have to do data validation if you only want the thing to
	//move a maxmimum of MaxMoveAmount pixels.
	long 		CalculateTrianglePositionFromDelta(long delta,
													   long MaxMoveAmount);

};

#endif

