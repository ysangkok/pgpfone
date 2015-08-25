/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: PGPfoneCommon.h,v 1.5 1999/03/10 02:50:55 heller Exp $
____________________________________________________________________________*/

#define USE_WHACKFREESPACE			PGP_DEBUG

#define USE_PGP_LEAKS				PGP_DEBUG
#define PGP_DEBUG_FIND_LEAKS		PGP_DEBUG

#define noVC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		// MFC socket extensions

#include "pgpLeaks.h"
#include "pgpMem.h"

//#define BNINCLUDE	bni80386c.h
#define PGPXFER				// define to include file transfer features
#define BETA				// set for betas only
#define NIL					0L

