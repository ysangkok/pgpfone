/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: bytefifo.h,v 1.4 1999/03/10 02:31:41 heller Exp $
____________________________________________________________________________*/
#ifndef BYTEFIFO_H
#define BYTEFIFO_H

#include "PGPFone.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ByteFifoPage;	/* Opaque structure for internal use only */

struct ByteFifo {
	struct ByteFifoPage *head;	/* Read end of FIFO */
	struct ByteFifoPage **tail;	/* Write end of FIFO */

	ulong pagespace;	/* Total bytes allocated */

	uchar *headptr;		/* Pointer to next byte to read */
	uchar *tailptr;		/* Pointer to next byte to write */

	ulong headspace;	/* Number of unread bytes in head page */
	ulong tailspace;	/* Number of unwritten bytes in tail page */

	ulong pagesize;	/* Number of data bytes per page */
				/* DO NOT CHANGE WHILE FIFO IS NON-EMPTY */
};

void byteFifoInit(struct ByteFifo *fifo);
void byteFifoDestroy(struct ByteFifo *fifo);

ulong byteFifoSize(struct ByteFifo *fifo);

uchar *byteFifoGetSpace(struct ByteFifo *fifo, ulong *len);
void byteFifoSkipSpace(struct ByteFifo *fifo, ulong len);

uchar *byteFifoGetBytes(struct ByteFifo *fifo, ulong *len);
void byteFifoSkipBytes(struct ByteFifo *fifo, ulong len);

ulong
byteFifoWrite(struct ByteFifo *fifo, uchar const *data, ulong len);

#ifdef __cplusplus
}
#endif

#endif

