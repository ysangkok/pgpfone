/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CEncryptionStream.h,v 1.4 1999/03/10 02:33:18 heller Exp $
____________________________________________________________________________*/
#ifndef CENCRYPTIONSTREAM_H
#define CENCRYPTIONSTREAM_H

#include "PGPFone.h"
#include "PGPFoneUtils.h"

#define NUMCRYPTORS		4

enum EncryptionAlgs {_enc_cast, _enc_tripleDES, _enc_blowfish, _enc_none};

class CEncryptionStream
{
public:
					CEncryptionStream(void)		{	}
	virtual			~CEncryptionStream(void)	{	}
	virtual void	Decrypt(uchar *buffer, long length, uchar *iv) = 0;
	virtual void	Encrypt(uchar *buffer, long length, uchar *iv) = 0;
};

short KeySize(uchar *algorithm);
extern uchar sCryptorHash[4][5];

#endif

