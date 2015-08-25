/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: PGPFoneUtils.h,v 1.4 1999/03/10 02:51:20 heller Exp $
____________________________________________________________________________*/
#ifndef PGPFONEUTILS_H
#define PGPFONEUTILS_H

#include "PGPFone.h"

#ifdef	MACINTOSH
enum {
	env_HasOpenTransport	= 0x10000000,
	env_HasMacTCP			= 0x20000000
};
#endif	/* WIN32 */


void	InitSafeAlloc();
void	DisposeSafeAlloc();
void	*safe_malloc(long len);
void	safe_free(void *data);
void	safe_increaseCount(void *data);

#ifdef	PGP_WIN32
void	pgp_memdone(void);
void	pgp_meminit(void);
uchar	pgp_memvalidate(void *ptr);
#endif
void	*pgp_malloc(long len);
void	*pgp_mallocclear(long len);
void	*pgp_realloc(void *orig, long newLen);
void	pgp_free(void *freePtr);
void	pgp_memcpy(void *dest, void *src, long len);

short	PGFAlert(char *inCStr, short allowCancel);
void	DebugLog(char *fmt, ...);
void	DebugLogData(void *s, long len);

ulong	pgp_milliseconds();
ulong	pgp_getticks();

void	num2str(uchar *dest, long num, short len, short max);
void	num2strpad(uchar *dest, register long num, short len,
				short max, char padChar);

long	minl(long x, long y);
long	maxl(long x, long y);
short	mins(short x, short y);
short	maxs(short x, short y);
short	maxi(short x, short y);

uchar	CalcChecksum8(void *data, long len);
ushort	CalcCRC16(void *data, long len);
ulong	CalcCRC32(void *data, long len);

#endif

