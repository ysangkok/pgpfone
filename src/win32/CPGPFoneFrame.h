/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPGPFoneFrame.h,v 1.7 1999/03/10 02:36:53 heller Exp $
____________________________________________________________________________*/
#ifndef CPGPFONEFRAME_H
#define CPGPFONEFRAME_H

#include "CPGPFStatusBar.h"

class CPFWindow;

class CPGPFoneFrame : public CFrameWnd
{
public: // create from serialization only
	CPGPFoneFrame();
	DECLARE_DYNCREATE(CPGPFoneFrame)

public:
	void DoPrefs(short save);
	void setStatusSecureIconID(int ID) {mStatusBar.setSecureIconID(ID);}
	void setStatusPhoneIconID(int ID) {mStatusBar.setPhoneIconID(ID);}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGPFoneFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	// Implementation
	virtual ~CPGPFoneFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CPGPFStatusBar          mStatusBar;
	CPFWindow				*mCallView;

	//{{AFX_MSG(CPGPFoneFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

