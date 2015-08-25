/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CEncryptionDialog.h,v 1.4 1999/03/10 02:33:17 heller Exp $
____________________________________________________________________________*/
#ifndef CENCRYPTIONDIALOG_H
#define CENCRYPTIONDIALOG_H

#include "PGPFone.h"

class CEncryptionDialog : public CPropertyPage
{
	DECLARE_DYNCREATE(CEncryptionDialog)

public:
				CEncryptionDialog();
				~CEncryptionDialog();

	void		GetParameters(PGPFoneOptions *options);
	void		SetParameters(PGPFoneOptions *options);

	//{{AFX_DATA(CEncryptionDialog)
	enum { IDD = IDD_ENCRYPTION };
	CComboBox	mPrefPrimeList;
	CComboBox	mPrefEncryptorList;
	int		mPreferredPrime;
	int		mPrefEncryptor;
	BOOL	m1024bits;
	BOOL	m1536bits;
	BOOL	m2048bits;
	BOOL	m768bits;
	BOOL	m3072bits;
	BOOL	m4096bits;
	BOOL	mX3DES;
	BOOL	mXBlowfish;
	BOOL	mXCAST;
	BOOL	mXNone;
	//}}AFX_DATA

protected:
	//{{AFX_VIRTUAL(CEncryptionDialog)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CEncryptionDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif

