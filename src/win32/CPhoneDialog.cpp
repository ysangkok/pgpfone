/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CPhoneDialog.cpp,v 1.4 1999/03/10 02:38:54 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CPGPFone.h"
#include "resource.h"

#include "CPhoneDialog.h"
#include "PGPFone.h"
#include "CSoundInput.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
#undef new
#endif	// DEBUG
IMPLEMENT_DYNCREATE(CPhoneDialog, CPropertyPage)
#ifdef DEBUG
#define new DEBUG_NEW
#endif	// DEBUG

#define NUM_CODERS	11

static long	coderList[NUM_CODERS] =
{	'GS4L', 'GS6L', 'GS7L', 'GL80', 'GS1L',
	'GSM4', 'GSM6', 'GSM7', 'GS80', 'GSM1', 'ADP8'	};

CPhoneDialog::CPhoneDialog() : CPropertyPage(CPhoneDialog::IDD)
{
	//{{AFX_DATA_INIT(CPhoneDialog)
	mListenFlag = FALSE;
	mPlayRing = FALSE;
	mAltDecoder = -1;
	mPrefDecoder = -1;
	mConnection = 0;
	mFullDuplex = FALSE;
	mIDIncoming = FALSE;
	mIDOutgoing = FALSE;
	mIDUnencrypted = FALSE;
	mNameString = _T("");
	//}}AFX_DATA_INIT

}

CPhoneDialog::~CPhoneDialog()
{
}

void CPhoneDialog::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPhoneDialog)
	DDX_Check(pDX, IDC_LISTEN_FOR_CALLS, mListenFlag);
	DDX_Check(pDX, IDC_PLAY_RING, mPlayRing);
	DDX_CBIndex(pDX, IDC_ALT_DECOMPRESSOR, mAltDecoder);
	DDX_CBIndex(pDX, IDC_PREF_DECOMPRESSOR, mPrefDecoder);
	DDX_Control(pDX, IDC_ALT_DECOMPRESSOR, mAltDecoderList);
	DDX_Control(pDX, IDC_PREF_DECOMPRESSOR, mPrefDecoderList);
	DDX_Radio(pDX, IDC_MODEM_CONNECTION, mConnection);
	DDX_Check(pDX, IDC_FULLDUPLEX, mFullDuplex);
	DDX_Check(pDX, IDC_IDINCOMING, mIDIncoming);
	DDX_Check(pDX, IDC_IDOUTGOING, mIDOutgoing);
	DDX_Check(pDX, IDC_IDUNENCRYPTED, mIDUnencrypted);
	DDX_Text(pDX, IDC_NAMEEDIT, mNameString);
	DDV_MaxChars(pDX, mNameString, 31);
	//}}AFX_DATA_MAP
}

void
CPhoneDialog::SetParameters(PGPFoneOptions *options)
{
	short	i;

	for(i=0;i<NUM_CODERS;i++)
		if(options->popt.prefCoder == coderList[i])
			break;
	if (i == NUM_CODERS)
		mPrefDecoder = 0;
	else
		mPrefDecoder = i;

	for(i=0;i<NUM_CODERS;i++)
		if(options->popt.altCoder == coderList[i])
			break;
	if(i==NUM_CODERS)
		mAltDecoder = 0;
	else
		mAltDecoder = i;

	mListenFlag = options->popt.alwaysListen;
	mPlayRing = options->popt.playRing;
	mIDIncoming = options->popt.idIncoming;
	mIDOutgoing = options->popt.idOutgoing;
	mIDUnencrypted = options->popt.idUnencrypted;
	mFullDuplex = options->popt.fullDuplex;
	mNameString = options->popt.identity;
	if(options->popt.connection == _cme_Serial)
		mConnection = 0;
	else
		mConnection = 1;
}

void
CPhoneDialog::GetParameters(PGPFoneOptions *options)
{
	options->popt.prefCoder = coderList[mPrefDecoder];
	options->popt.altCoder  = coderList[mAltDecoder];
	options->popt.alwaysListen = mListenFlag;
	options->popt.playRing = mPlayRing;
	options->popt.idIncoming = mIDIncoming;
	options->popt.idOutgoing = mIDOutgoing;
	options->popt.idUnencrypted = mIDUnencrypted;
	strcpy(options->popt.identity, (LPCTSTR)mNameString);
	options->popt.fullDuplex = mFullDuplex;
	if(!mConnection)
		options->popt.connection = _cme_Serial;
	else
		options->popt.connection = _cme_Internet;
}

BEGIN_MESSAGE_MAP(CPhoneDialog, CPropertyPage)
	//{{AFX_MSG_MAP(CPhoneDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL
CPhoneDialog::OnInitDialog() 
{
	int index = 0;

	CPropertyPage::OnInitDialog();

	mPrefDecoderList.AddString("GSM 4410Hz Lite");
	mPrefDecoderList.AddString("GSM 6000Hz Lite");
	mPrefDecoderList.AddString("GSM 7350Hz Lite");
	mPrefDecoderList.AddString("GSM 8000Hz Lite");
	mPrefDecoderList.AddString("GSM 11025Hz Lite");
	mPrefDecoderList.AddString("GSM 4410Hz");
	mPrefDecoderList.AddString("GSM 6000Hz");
	mPrefDecoderList.AddString("GSM 7350Hz");
	mPrefDecoderList.AddString("GSM 8000Hz");
	mPrefDecoderList.AddString("GSM 11025Hz");
	mPrefDecoderList.AddString("ADPCM 8000Hz");

	mAltDecoderList.AddString("GSM 4410Hz Lite");
	mAltDecoderList.AddString("GSM 6000Hz Lite");
	mAltDecoderList.AddString("GSM 7350Hz Lite");
	mAltDecoderList.AddString("GSM 8000Hz Lite");
	mAltDecoderList.AddString("GSM 11025Hz Lite");
	mAltDecoderList.AddString("GSM 4410Hz");
	mAltDecoderList.AddString("GSM 6000Hz");
	mAltDecoderList.AddString("GSM 7350Hz");
	mAltDecoderList.AddString("GSM 8000Hz");
	mAltDecoderList.AddString("GSM 11025Hz");
	mAltDecoderList.AddString("ADPCM 8000Hz");

	if(!gHardwareIsFullDuplex)
	{
		// If the hardware does not support full duplex,
		// disable that button.
		CButton *fdButton = (CButton *)GetDlgItem(IDC_FULLDUPLEX);
		fdButton->ModifyStyle(0, WS_DISABLED);
	}
	UpdateData(FALSE);	// save the new lists

	return TRUE;
}

