/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: PGPFWinUtils.cpp,v 1.3 1999/03/10 02:51:20 heller Exp $
____________________________________________________________________________*/
#include <string.h>
#include <stdlib.h>

#include "PGPFone.h"

//This function, given a full path in fileAlias, gives you back the long
//component (only) of the file.  Ie:
//
//char longFileName[_MAX_PATH + 1];
//getLongFileName(longFileName, sizeof(longFileName), "C:\\FOOBAR~1");
//
//Would put something like "FOOBARBAZ" (or whatever the long fn is) in
//longFileName.
//
//Note that, as far as I'm aware, if you're actually going to open the file,
//you need to use the alias (short) version; opening "C:\\FOOBARBAZ" won't
//work, while opening "C:\\FOOBAR~1" will.
BOOL GetLongFileName(	char	*longFileName, 
						long	longFileNameLength, 
						char	*fileAlias)
{
#if 0
	LPTSTR tempFileName;
	char path[_MAX_PATH];
#endif
	WIN32_FIND_DATA fileInfo;
	HANDLE FindFileHandle;
	BOOL returnCode = FALSE;

	pgpAssert(longFileName);
	pgpAssert(fileAlias);
	pgpAssert(*fileAlias);

	if((FindFileHandle = FindFirstFile(fileAlias, &fileInfo))
		!= INVALID_HANDLE_VALUE)
	{
		strncpy(longFileName, fileInfo.cFileName, longFileNameLength);
		FindClose(FindFileHandle);
		returnCode = TRUE;
	}

#if 0
	if(GetFullPathName(fileAlias, 
						sizeof(path), 
						path, 
						 &tempFileName))
	{
		pgp_assert(longFileNameLength > 
					(long) (strlen(tempFileName) + strlen(path)));
		strncpy(longFileName, path, longFileNameLength - 1);
		strncat(longFileName, tempFileName, longFileNameLength - 1);
		returnCode = TRUE;
	}
#endif
	return(returnCode);
}

void DrawMaskedBitmap(	CDC		*pDC,
						UINT	bitmapID,
						UINT	maskID,
						POINT	leftTop)
{
	CBrush		newBrush(RGB(192,192,192));
	CPen		newPen(PS_SOLID, 1, RGB(192,192,192));
	CBitmap		*oldBitmap, *newBitmap = new CBitmap;
	BITMAP		bm;
	CDC			memDC;

	// Logo display

	// Need to ditch the background and put in the user's bkg color
	memDC.CreateCompatibleDC(pDC);

	// mask out the areas we will affect
	newBitmap->LoadBitmap(bitmapID);
	newBitmap->GetObject(sizeof(bm), &bm);
	oldBitmap = memDC.SelectObject(newBitmap);
	pDC->BitBlt(leftTop.x, leftTop.y, bm.bmWidth, bm.bmHeight,
				&memDC, 0, 0, SRCAND);
	memDC.SelectObject(oldBitmap);
	delete newBitmap;

	// draw the logo
	newBitmap = new CBitmap;

	newBitmap->LoadBitmap(maskID);
	newBitmap->GetObject(sizeof(bm), &bm);
	oldBitmap = memDC.SelectObject(newBitmap);
	pDC->BitBlt(leftTop.x, leftTop.y, bm.bmWidth, bm.bmHeight,
				&memDC, 0, 0, SRCPAINT);
	memDC.SelectObject(oldBitmap);
	delete newBitmap;
}

