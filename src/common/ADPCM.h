/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: ADPCM.h,v 1.3 1999/03/10 02:31:34 heller Exp $
____________________________________________________________________________*/
/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/

#ifndef ADPCM_H
#define ADPCM_H

typedef struct adpcm_state
{
    short	valprev;	/* Previous output value */
    char	index;		/* Index into stepsize table */
} adpcm_state;

void adpcm_coder(short [], char [], int, adpcm_state *);
void adpcm_decoder(char [], short [], int, adpcm_state *);

#endif

