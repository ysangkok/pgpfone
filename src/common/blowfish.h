/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: blowfish.h,v 1.3 1999/03/10 02:31:38 heller Exp $
____________________________________________________________________________*/
#ifndef BLOWFISH_H
#define BLOWFISH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "PGPfone.h"

#define MAXKEYBYTES 56		/* 448 bits */

void	BlowfishDecipher(ulong *sxl, ulong *sxr, ulong *dxl, ulong *dxr, ulong *P, ulong **S);
void	BlowfishEncipher(ulong *sxl, ulong *sxr, ulong *dxl, ulong *dxr, ulong *P, ulong **S);
void	BlowfishInit(uchar *key, short keybytes, ulong **newP, ulong ***newS);
void	BlowfishDone(ulong *P, ulong **S);

#ifdef __cplusplus
}
#endif

#endif	// BLOWFISH_H
