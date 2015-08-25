/*******************************************************************************
 *					C A S T 5   S Y M M E T R I C   C I P H E R
 *		Copyright (c) 1996 Northern Telecom Ltd. All rights reserved.
 *******************************************************************************
 *
 * FILE:		cast5.h
 *
 * AUTHOR(S):	R.T.Lockhart, Dept. 9C40, Nortel Ltd.
 *				C. Adams, Dept 9C21, Nortel Ltd.
 *
 * DESCRIPTION: CAST5 header file. This file defines the interface for
 * the CAST5 symmetric-key encryption/decryption/MACing code. The code
 * supports key lengths from 8 through 128 bits, in multiples of 8.
 *
 *
 * Error handling:
 * Most functions return an int that indicates the success or failure of the
 * operation. See the C5E_xxx #defines for a list of error conditions.
 *	
 * Data size assumptions: 	BYTE	- unsigned 8 bits
 *				UINT32	- unsigned 32 bits
 *
 ******************************************************************************/
 
#ifndef CAST5_H
#define CAST5_H

typedef unsigned long word32;
typedef unsigned char byte; 

void CAST5schedule(word32 xkey[32], byte const *k);
void CAST5encrypt(byte const *in, byte *out, word32 const *xkey);
void CAST5decrypt(byte const *in, byte *out, word32 const *xkey);

#endif /* CAST5_H */

