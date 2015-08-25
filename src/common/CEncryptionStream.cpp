/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: CEncryptionStream.cpp,v 1.3 1999/03/10 02:33:15 heller Exp $
____________________________________________________________________________*/
#include <string.h>		// for memcmp()

#include "CEncryptionStream.h"
#include "PGPFoneUtils.h"

uchar sCryptorHash[4][5] = {"CAST", "TDEA", "BLOW", "NONE"};

short
KeySize(uchar *algorithm)
{
	if(!memcmp(algorithm, sCryptorHash[_enc_cast], 4))
		return 16;
	else if(!memcmp(algorithm, sCryptorHash[_enc_tripleDES], 4))
		return 24;
	else if(!memcmp(algorithm, sCryptorHash[_enc_blowfish], 4))
		return 24;
	else if(!memcmp(algorithm, sCryptorHash[_enc_none], 4))
		return 0;

	return 0;
}

