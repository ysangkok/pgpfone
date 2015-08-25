/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CModemDialog.h,v 1.4 1999/03/10 02:35:19 heller Exp $
____________________________________________________________________________*/
#ifndef CMODEMDIALOG_H
#define CMODEMDIALOG_H

#include "PGPFone.h"

class CModemDialog : public CPropertyPage
{
	DECLARE_DYNCREATE(CModemDialog)

public:
	CModemDialog();
	~CModemDialog();

	//{{AFX_DATA(CModemDialog)
	enum { IDD = IDD_MODEM };
	int		mPort;
	CString	mInitString;
	int		mBaudRate;
	int		mPortSpeed;
	CComboBox	mBaudRateList;
	CComboBox	mPortSpeedList;
	//}}AFX_DATA

	void	GetParameters(PGPFoneOptions *options);
	void	SetParameters(PGPFoneOptions *options);

	//{{AFX_VIRTUAL(CModemDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CModemDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif

