/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: samplerate.cpp,v 1.3 1999/03/10 03:02:27 heller Exp $
____________________________________________________________________________*/
#include "PGPFoneUtils.h"
#include "samplerate.h"

#ifdef PGP_MACINTOSH
#include <FixMath.h>
#endif
		 
long
RateChange(short *src, short *dest, long srcLen,
				RATE_VAL srcRate, RATE_VAL destRate)
{
	float		nSrcRate,
				nDestRate;
	
#ifdef PGP_MACINTOSH
	nSrcRate = (ulong)(((ulong)srcRate) >> 16);
	nDestRate = (ulong)(((ulong)destRate) >> 16);
#else
	nSrcRate = srcRate;
	nDestRate = destRate;
#endif

	// All samples are 16 bit signed values. Pass in the number of source samples,
	// the sample rate of the source data, and the required destination sample
	// rate.
	if(!srcLen)
		return 0;
	// If the sample rates are identical, just copy the data over
	if(srcRate == destRate)
	{
		pgp_memcpy((Ptr)dest, (Ptr)src, srcLen*2);
		return srcLen;
	}
	short *sourceShortPtr = src;
	short *destShortPtr = dest;
	
	// Downsample
	if(srcRate > destRate)
	{
		float destStep = nDestRate / nSrcRate;
		float position = 0;
		
		while(srcLen)
		{
			long destSample = 0;
			long count = 0;
			
			// Accumulate source samples, until the fractional destination position
			// crosses a boundary (or we run out of source data)
			while(srcLen && (position < 1.0))
			{
				destSample += *sourceShortPtr++;
				srcLen--;
				position += destStep;
				count++;
			}
			position = position - 1.0;
			*destShortPtr++ = (destSample/count);
		}
	}
	else // Upsample
	{
		float sourceStep = nSrcRate / nDestRate;
		float position = 0;

		while(--srcLen)
		{
			long leftSample = *sourceShortPtr++;
			long sampleDifference = *sourceShortPtr-leftSample;

			while(position < 1.0)
			{
				*destShortPtr++ = leftSample + ((float)sampleDifference * position);
				position += sourceStep;
			}
			position = position - 1.0;
		}
	}
	// Return the number of samples written
	// Note that this code will sometimes (often?) write one sample too many, so make
	// sure your destination buffer is oversized by one.
	return destShortPtr - dest;
}

