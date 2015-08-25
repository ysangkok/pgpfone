/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: HashWordList.h,v 1.4 1999/03/10 02:42:57 heller Exp $
____________________________________________________________________________*/
#ifndef HASHWORDLIST_H
#define HASHWORDLIST_H

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

#include "PGPFone.h"

extern uchar hashWordListOdd[256][12];
extern uchar hashWordListEven[256][10];

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif

