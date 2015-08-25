/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CStatusPane.cpp,v 1.4 1999/03/10 02:41:16 heller Exp $
____________________________________________________________________________*/
#include "StdAfx.h"
#include "CPGPFone.h"

#include "CStatusPane.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CStatusPane	*CStatusPane::sStatusPane = NULL;

CStatusPane::CStatusPane()
	:	LMutexSemaphore()
{
	if (!sStatusPane)
		sStatusPane = this;

	mStatusString.Empty();
	mDirty = 0;
}

CStatusPane::~CStatusPane()
{
}

void
CStatusPane::AddStatus(Boolean urgent, char *fmt, ...)
{
	va_list	ap;
	char	buf[1024];
	long	len;
	StMutex		mutex(*this);

	va_start(ap, fmt);
	len = vsprintf(buf, fmt, ap);
	va_end(ap);

	mStatusString += buf;
	mStatusString += "\r\n";

	InterlockedExchange(&mDirty, 1);
}

void
CStatusPane::Clear(void)
{
	StMutex		mutex(*this);

	mStatusString.Empty();
	InterlockedExchange(&mDirty, 1);
}

void
CStatusPane::UpdateStatus(void)
{
	StMutex		mutex(*this);

	if (!mDirty)
		return;
	InterlockedExchange(&mDirty, 0);

	ReplaceSel(mStatusString);
	mStatusString.Empty();
}

BEGIN_MESSAGE_MAP(CStatusPane, CEdit)
	//{{AFX_MSG_MAP(CStatusPane)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

