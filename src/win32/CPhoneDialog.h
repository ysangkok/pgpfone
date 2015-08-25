/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPhoneDialog.h,v 1.4 1999/03/10 02:38:57 heller Exp $
____________________________________________________________________________*/
#ifndef CPHONEDIALOG_H
#define CPHONEDIALOG_H

#include "PGPFone.h"

class CPhoneDialog : public CPropertyPage
{
	DECLARE_DYNCREATE(CPhoneDialog)

public:
	CPhoneDialog();
	~CPhoneDialog();

	void	GetParameters(PGPFoneOptions *options);
	void	SetParameters(PGPFoneOptions *options);

	//{{AFX_DATA(CPhoneDialog)
	enum { IDD = IDD_PHONE };
	BOOL	mListenFlag;
	BOOL	mPlayRing;
	int		mAltDecoder;
	int		mPrefDecoder;
	CComboBox	mAltDecoderList;
	CComboBox	mPrefDecoderList;
	int		mConnection;
	BOOL	mFullDuplex;
	BOOL	mIDIncoming;
	BOOL	mIDOutgoing;
	BOOL	mIDUnencrypted;
	CString	mNameString;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CPhoneDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CPhoneDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif

