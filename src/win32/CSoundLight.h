/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CSoundLight.h,v 1.5 1999/03/10 02:41:15 heller Exp $
____________________________________________________________________________*/
#ifndef CSOUNDLIGHT_H
#define CSOUNDLIGHT_H

#include "CLevelMeter.h"

class CSoundLight : public CButton
{
// Construction
public:
	CSoundLight();

// Attributes
public:

// Operations
public:
	void Switch(BOOLEAN isOn); //Sets the light on or off
	void Toggle(void);		//Switches the current status
	void Flicker(long msToFlicker); //Equivilant to:
									//Toggle(); 
									//Sleep(msToFlicker);
									//Toggle();
	BOOLEAN GetStatus(void);	//Returns TRUE if the light is on

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSoundLight)
	//}}AFX_VIRTUAL

public:
	virtual ~CSoundLight();

	virtual BOOL 	Create(LPCTSTR lpszCaption, 
									 DWORD dwStyle, 
									 CWnd* pParentWnd, 
									 UINT nID,
									 CLevelMeter *levelMeter,
									 COLORREF litColor,
									 BOOLEAN isOn);
	// Generated message map functions
protected:
	//{{AFX_MSG(CTriThreshold)
	afx_msg void OnPaint();
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
	CBitmap		mOnBitmap, mOffBitmap;
	BOOL		mLightIsOn;
	COLORREF    mLightColor;
};

#endif

