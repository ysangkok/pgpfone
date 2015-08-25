/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CStatusPane.h,v 1.4 1999/03/10 02:41:17 heller Exp $
____________________________________________________________________________*/
#ifndef CSTATUSPANE_H
#define CSTATUSPANE_H

#include "LMutexSemaphore.h"

class CStatusPane 	:	public CEdit,
						public LMutexSemaphore
{
public:
				CStatusPane();
	virtual 	~CStatusPane();

	void		AddStatus(Boolean urgent, char *fmt, ...);
	void		Clear(void);
	void		UpdateStatus(void);

	static CStatusPane	*sStatusPane;
	static CStatusPane	*GetStatusPane(void)	{	return sStatusPane;	}

	//{{AFX_VIRTUAL(CStatusPane)
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CStatusPane)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	LMutexSemaphore		mMutex;
	CString				mStatusString;
	long				mDirty;			// whether the pane needs updating
};

#endif

