/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CModemDialog.cpp,v 1.5 1999/03/10 02:35:14 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CPGPFone.h"

#include "CModemDialog.h"
#include "PGPFone.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
#undef new
#endif	// DEBUG

IMPLEMENT_DYNCREATE(CModemDialog, CPropertyPage)

#ifdef DEBUG
#define new DEBUG_NEW
#endif	// DEBUG

CModemDialog::CModemDialog() : CPropertyPage(CModemDialog::IDD)
{
	//{{AFX_DATA_INIT(CModemDialog)
	mPort = 0;
	mInitString = _T("Z0");
	mBaudRate = 0;
	mPortSpeed = 0;
	//}}AFX_DATA_INIT
}

CModemDialog::~CModemDialog()
{
}

void CModemDialog::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModemDialog)
	DDX_Radio(pDX, IDC_COM1, mPort);
	DDX_Text(pDX, IDC_INIT_STRING, mInitString);
	DDX_CBIndex(pDX, IDC_BAUD_RATE, mBaudRate);
	DDX_CBIndex(pDX, IDC_PORT_SPEED, mPortSpeed);
	DDX_Control(pDX, IDC_BAUD_RATE, mBaudRateList);
	DDX_Control(pDX, IDC_PORT_SPEED, mPortSpeedList);
	//}}AFX_DATA_MAP
}

void
CModemDialog::GetParameters(PGPFoneOptions *options)
{
	options->sopt.maxBaud = mBaudRate;
	switch (mPortSpeed)
	{
		case 0:	options->sopt.baud =  9600; break;
		case 1: options->sopt.baud = 14400; break;
		case 2: options->sopt.baud = 19200; break;
		case 3: options->sopt.baud = 28800; break;
		case 4: options->sopt.baud = 38400; break;
	}
	sprintf(options->sopt.port, "COM%d", mPort + 1);
	strcpy(options->sopt.modemInit, (LPCTSTR)mInitString);
}

void
CModemDialog::SetParameters(PGPFoneOptions *options)
{
	mBaudRate  = options->sopt.maxBaud;
	switch (options->sopt.baud)
	{
		case 9600:	mPortSpeed = 0;	break;
		case 14400:	mPortSpeed = 1;	break;
		case 19200:	mPortSpeed = 2; break;
		case 28800:	mPortSpeed = 3;	break;
		case 38400:	mPortSpeed = 4;	break;
		default:	mPortSpeed = 1;	break;
	}
	mPort		= atoi(options->sopt.port+3) - 1;
	mInitString = options->sopt.modemInit;
}

BEGIN_MESSAGE_MAP(CModemDialog, CPropertyPage)
	//{{AFX_MSG_MAP(CModemDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CModemDialog::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	mBaudRateList.AddString("9600");
	mBaudRateList.AddString("14400");
	mBaudRateList.AddString("28800 or more");
	switch (mBaudRate)
	{
		case 9600:	mBaudRateList.SetCurSel(0);	break;
		default:
		case 14400:	mBaudRateList.SetCurSel(1);	break;
		case 28800:	mBaudRateList.SetCurSel(2);	break;
	}

	mPortSpeedList.AddString("9600");	// item 0
	mPortSpeedList.AddString("14400");	// item 1
	mPortSpeedList.AddString("19200");	// item 2
	mPortSpeedList.AddString("28800");	// item 3
	mPortSpeedList.AddString("38400");	// item 4
	
	UpdateData(FALSE);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

