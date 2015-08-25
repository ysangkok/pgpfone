/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: crc.h,v 1.3 1999/03/10 02:39:20 heller Exp $
____________________________________________________________________________*/

//	This 32 bit CRC follows FIPS PUB 71 and FED-STD-1003

#ifndef CRC_H
#define CRC_H

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* 16-bit big-endian CRC table */
extern unsigned short const crctab[256];

/* 16-bit big-endian CRC update */
#define updatecrc16(c, crc) (crctab[((crc) >> 8) ^ (c)] ^ ((crc) << 8))

/* 32-bit little-endian CRC table */
extern unsigned long const cr3tab[256];

/* 32-bit little-endian CRC update */
#define updatecrc32(c, crc) \
	(cr3tab[((crc)^(c)) & 0xFF] ^ ((unsigned long)(crc) >> 8))

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif /* CRC_H */

