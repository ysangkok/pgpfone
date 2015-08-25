/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CAuthWindow.h,v 1.5 1999/03/10 02:32:02 heller Exp $
____________________________________________________________________________*/
#ifndef CAUTHWINDOW_H
#define CAUTHWINDOW_H

class CAuthWindow : public CWnd
{
public:
	CAuthWindow();

	void	SetAuthWords(char *words);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAuthWindow)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAuthWindow();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAuthWindow)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CFont		mPFont, mPBFont;
	CStatic		mTitleStatic, mAuthStatic;
};

#endif

