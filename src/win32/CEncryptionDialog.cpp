/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CEncryptionDialog.cpp,v 1.4 1999/03/10 02:33:15 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CPGPFone.h"

#include "CEncryptionDialog.h"

#include "CEncryptionStream.h"
#include "PGPFone.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
#undef new
#endif	// DEBUG
IMPLEMENT_DYNCREATE(CEncryptionDialog, CPropertyPage)
#ifdef DEBUG
#define new DEBUG_NEW
#endif	// DEBUG

CEncryptionDialog::CEncryptionDialog()
	: CPropertyPage(CEncryptionDialog::IDD)
{
	//{{AFX_DATA_INIT(CEncryptionDialog)
	m1024bits = FALSE;
	m1536bits = FALSE;
	m2048bits = FALSE;
	m768bits = FALSE;
	m3072bits = FALSE;
	m4096bits = FALSE;
	mX3DES = FALSE;
	mXBlowfish = FALSE;
	mXCAST = FALSE;
	mXNone = FALSE;
	//}}AFX_DATA_INIT
}

CEncryptionDialog::~CEncryptionDialog()
{
}

void CEncryptionDialog::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEncryptionDialog)
	DDX_Control(pDX, IDC_PREFERRED_PRIME, mPrefPrimeList);
	DDX_Control(pDX, IDC_PREF_ENCRYPTION, mPrefEncryptorList);
	DDX_CBIndex(pDX, IDC_PREFERRED_PRIME, mPreferredPrime);
	DDX_CBIndex(pDX, IDC_PREF_ENCRYPTION, mPrefEncryptor);
	DDX_Check(pDX, IDC_1024_BITS, m1024bits);
	DDX_Check(pDX, IDC_1536_BITS, m1536bits);
	DDX_Check(pDX, IDC_2048_BITS, m2048bits);
	DDX_Check(pDX, IDC_768_BITS, m768bits);
	DDX_Check(pDX, IDC_3072_BITS, m3072bits);
	DDX_Check(pDX, IDC_4096_BITS, m4096bits);
	DDX_Check(pDX, IDC_3DESENCRYPTOR, mX3DES);
	DDX_Check(pDX, IDC_BLOWFISHENCRYPTOR, mXBlowfish);
	DDX_Check(pDX, IDC_CASTENCRYPTOR, mXCAST);
	DDX_Check(pDX, IDC_NOENCRYPTOR, mXNone);
	//}}AFX_DATA_MAP
}

void
CEncryptionDialog::SetParameters(PGPFoneOptions *options)
{
	mPrefEncryptor = options->popt.prefEncryptor;
	mPreferredPrime = options->popt.prefPrime;

	mX3DES = mXBlowfish = mXCAST = mXNone = FALSE;
	if(options->popt.cryptorMask & 0x0001)	mXCAST  = TRUE;
	if(options->popt.cryptorMask & 0x0002)	mX3DES  = TRUE;
	if(options->popt.cryptorMask & 0x0004)	mXBlowfish  = TRUE;
	if(options->popt.cryptorMask & 0x0008)	mXNone  = TRUE;
	
	m768bits = m1024bits = m1536bits = m2048bits = m3072bits = m4096bits = FALSE;
	if(options->popt.primeMask & 0x0001)	m768bits  = TRUE;
	if(options->popt.primeMask & 0x0002)	m1024bits  = TRUE;
	if(options->popt.primeMask & 0x0004)	m1536bits = TRUE;
	if(options->popt.primeMask & 0x0008)	m2048bits = TRUE;
	if(options->popt.primeMask & 0x0010)	m3072bits = TRUE;
	if(options->popt.primeMask & 0x0020)	m4096bits = TRUE;
}

void
CEncryptionDialog::GetParameters(PGPFoneOptions *options)
{
	options->popt.prefEncryptor = mPrefEncryptor;
	options->popt.prefPrime = mPreferredPrime;

	gPGFOpts.popt.cryptorMask = 0;
	if(mXCAST)		gPGFOpts.popt.cryptorMask |= 0x0001;
	if(mX3DES)		gPGFOpts.popt.cryptorMask |= 0x0002;
	if(mXBlowfish)	gPGFOpts.popt.cryptorMask |= 0x0004;
	if(mXNone)		gPGFOpts.popt.cryptorMask |= 0x0008;
	if(gPGFOpts.popt.prefEncryptor == _enc_cast)
		gPGFOpts.popt.cryptorMask |= (1<<_enc_cast);
	if(gPGFOpts.popt.prefEncryptor == _enc_blowfish)
		gPGFOpts.popt.cryptorMask |= (1<<_enc_blowfish);
	if(gPGFOpts.popt.prefEncryptor == _enc_tripleDES)
		gPGFOpts.popt.cryptorMask |= (1<<_enc_tripleDES);
	if(gPGFOpts.popt.prefEncryptor == _enc_none)
		gPGFOpts.popt.cryptorMask |= (1<<_enc_none);
	
	gPGFOpts.popt.primeMask = 0;
	if(m768bits)	gPGFOpts.popt.primeMask |= 0x0001;
	if(m1024bits)	gPGFOpts.popt.primeMask |= 0x0002;
	if(m1536bits)	gPGFOpts.popt.primeMask |= 0x0004;
	if(m2048bits)	gPGFOpts.popt.primeMask |= 0x0008;
	if(m3072bits)	gPGFOpts.popt.primeMask |= 0x0010;
	if(m4096bits)	gPGFOpts.popt.primeMask |= 0x0020;
}

BEGIN_MESSAGE_MAP(CEncryptionDialog, CPropertyPage)
	//{{AFX_MSG_MAP(CEncryptionDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CEncryptionDialog::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	mPrefEncryptorList.AddString("CAST");
	mPrefEncryptorList.AddString("Triple-DES");
	mPrefEncryptorList.AddString("Blowfish");
	mPrefEncryptorList.AddString("None");

	mPrefPrimeList.AddString("768 bits");
	mPrefPrimeList.AddString("1024 bits");
	mPrefPrimeList.AddString("1536 bits");
	mPrefPrimeList.AddString("2048 bits");
	mPrefPrimeList.AddString("3072 bits");
	mPrefPrimeList.AddString("4096 bits");

	UpdateData(FALSE);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

