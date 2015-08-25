/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPGPFStatusBar.h,v 1.5 1999/03/10 02:36:56 heller Exp $
____________________________________________________________________________*/
#ifndef CPGPFSTATUSBAR_H
#define CPGPFSTATUSBAR_H

class CPGPFStatusBar : public CStatusBar
{
// Construction
public:
	CPGPFStatusBar();
// Attributes
public:

private:
	int mPhoneIconID, mSecureIconID, mPhoneTextID, mSecureTextID;
	CToolTipCtrl mToolTipPhone, mToolTipSecure;

// Operations
public:
	virtual BOOL Create(CWnd* pParentWnd, 
						DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, 
						UINT nID = AFX_IDW_STATUS_BAR);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void setPhoneIconID(int phoneIconID);
	void setSecureIconID(int phoneIconID);
	virtual int OnToolHitTest(CPoint point, TOOLINFO* pTI);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGPFStatusBar)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPGPFStatusBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPGPFStatusBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg UINT OnNcHitTest(CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif

