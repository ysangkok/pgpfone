/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPGPFone.h,v 1.4 1999/03/10 02:36:48 heller Exp $
____________________________________________________________________________*/
#ifndef CPGPFONE_H
#define CPGPFONE_H

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

class CPGPFoneFrame;

class CPGPFoneApp : public CWinApp
{
public:
	CPGPFoneApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGPFoneApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPGPFoneApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	CPGPFoneFrame	*mPGPFWindow;
};

extern BOOLEAN			gHasWinsock;


#endif

