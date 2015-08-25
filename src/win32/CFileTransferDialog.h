/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CFileTransferDialog.h,v 1.4 1999/03/10 02:33:20 heller Exp $
____________________________________________________________________________*/
#ifndef CFILETRANSFERDIALOG_H
#define CFILETRANSFERDIALOG_H

#include "PGPFone.h"

class CFileTransferDialog : public CPropertyPage
{
	DECLARE_DYNCREATE(CFileTransferDialog)

public:
				CFileTransferDialog();
				~CFileTransferDialog();

	void		GetParameters(PGPFoneOptions *options);
	void		SetParameters(PGPFoneOptions *options);

	//{{AFX_DATA(CFileTransferDialog)
	enum { IDD = IDD_FILETRANSFER };
	CString	mReceiveFolderString;
	//}}AFX_DATA

protected:
	//{{AFX_VIRTUAL(CFileTransferDialog)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CFileTransferDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

#endif

