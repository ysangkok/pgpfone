/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: samplerate.h,v 1.4 1999/03/10 03:02:28 heller Exp $
____________________________________________________________________________*/
#ifndef SAMPLERATE_H
#define SAMPLERATE_H

#ifndef PGP_MACINTOSH
typedef float RATE_VAL;
typedef void * Ptr;
#else
typedef Fixed RATE_VAL;
#endif
 	   
long RateChange(short *src, short *dest, long srcLen,
				RATE_VAL srcRate, RATE_VAL destRate);


#endif

