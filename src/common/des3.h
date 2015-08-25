/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: des3.h,v 1.4 1999/03/10 02:42:03 heller Exp $
____________________________________________________________________________*/
#ifndef DES3_H
#define DES3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "PGPfone.h"

#define EN0		0		/* MODE == encrypt */
#define DE1		1		/* MODE == decrypt */

void DESDKeyDone (void *);
void D3des (uchar *, uchar *, void *);
void des3key(uchar *, short, void **);

#ifdef __cplusplus
}
#endif

#endif

