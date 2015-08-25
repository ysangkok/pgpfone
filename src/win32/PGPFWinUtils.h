/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: PGPFWinUtils.h,v 1.4 1999/03/10 02:51:22 heller Exp $
____________________________________________________________________________*/
#ifndef PGPFWINUTILS_H
#define PGPFWINUTILS_H


BOOL GetLongFileName(	char	*longFileName, 
					 	long	longFileNameLength, 
					 	char	*fileAlias);
void DrawMaskedBitmap(	CDC		*pDC,
						UINT	bitmapID,
						UINT	maskID,
						POINT	leftTop);


#endif

