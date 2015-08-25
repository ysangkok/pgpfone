/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CCounterEncryptor.h,v 1.4 1999/03/10 02:32:26 heller Exp $
____________________________________________________________________________*/
#ifndef CCOUNTERENCRYPTOR_H
#define CCOUNTERENCRYPTOR_H

#include "CEncryptionStream.h"
#include "PGPFone.h"

class CCounterEncryptor : public CEncryptionStream
{
public:
					CCounterEncryptor(uchar *key, uchar *alg);
	virtual			~CCounterEncryptor(void);

	void			Decrypt(uchar *buffer, long length, uchar *iv);
	void			Encrypt(uchar *buffer, long length, uchar *iv);
private:
	short			mAlgorithm;	// encryption algorithm
	uchar			*mKS1;		// key-schedule (most algorithms only need one part)
	uchar			*mKS2;		// more key-schedule material (for algorithms which have
								// two pieces like Blowfish)
	Boolean			mDecrypting;
};


#endif

