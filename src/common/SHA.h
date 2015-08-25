/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: SHA.h,v 1.4 1999/03/10 03:03:19 heller Exp $
____________________________________________________________________________*/
#ifndef SHA_H
#define SHA_H

#include "CHash.h"

/* NIST proposed Secure Hash Standard.

   Written 2 September 1992, Peter C. Gutmann.
   This implementation placed in the public domain.

   Modified 1 June 1993, Colin Plumb.
   These modifications placed in the public domain.

   Comments to pgut1@cs.aukuni.ac.nz */

// The SHS block size and message digest sizes, in bytes
#define SHS_BLOCKSIZE	64
#define SHS_DIGESTSIZE	20

class SHA : public CHash
{
public:
				SHA(void);
	virtual		~SHA(void)	{ }

	virtual void	Init(void);
	virtual void	Update(uchar *buf, short len);
	virtual void	Final(uchar *digest);
	virtual SHA		*Clone();
protected:
	void	Transform(void);

	ulong	mData[16];				// SHS data buffer
	ulong	mDigest[5];				// Message digest
	ulong	mCountHi, mCountLo;		// 64-bit byte count
};

#endif

