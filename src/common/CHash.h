/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CHash.h,v 1.4 1999/03/10 02:34:05 heller Exp $
____________________________________________________________________________*/
#ifndef CHASH_H
#define CHASH_H

#include "PGPFone.h"

// abstract class for hash algorithms

class CHash
{
public:
					CHash(void)		{ }
	virtual			~CHash(void)	{ }

	virtual void	Init(void) = 0;
	virtual void	Update(uchar *buffer, short len) = 0;
	virtual void	Final(uchar *buffer) = 0;

	short	mHashSize;
};

#endif

