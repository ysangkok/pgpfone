/*____________________________________________________________________________
	Copyright (C) 1996-1999 Network Associates, Inc.
	All rights reserved.

	$Id: bytefifo.cpp,v 1.3 1999/03/10 02:31:39 heller Exp $
____________________________________________________________________________*/
#ifdef PGP_WIN32
#include "StdAfx.h"
#endif
#include "PGPFoneUtils.h"
#include <stdlib.h>	/* For malloc */
#include <string.h>	/* For memset */

#include "bytefifo.h"

struct ByteFifoPage {
	struct ByteFifoPage *next;
	uchar data[1];	/* Actually longer, by the usual hack */
};

#define PAGESIZE 1020	/* Default page size */

/* Do entire init except set the pagesize */
static void
byteFifoInternalInit(struct ByteFifo *fifo)
{
	fifo->head = 0;
	fifo->tail = &fifo->head;
	fifo->headptr = 0;
	fifo->headspace = 0;
	fifo->tailptr = 0;
	fifo->tailspace = 0;
	fifo->pagespace = 0;
}

/* Initialize a byte FIFO with the default pagesize */
void
byteFifoInit(struct ByteFifo *fifo)
{
	byteFifoInternalInit(fifo);
	fifo->pagesize = PAGESIZE;
}

/* Destroy a byte FIFO */
void
byteFifoDestroy(struct ByteFifo *fifo)
{
	struct ByteFifoPage *page;

	while ((page = fifo->head) != 0) {
		fifo->head = page->next;
		memset(page, 0, sizeof(*page) + fifo->pagesize - 1);
		pgp_free(page);
	}
	byteFifoInternalInit(fifo);
}

/*
 * Return total number of bytes in a byte FIFO.
 * This is the total allocated space (fifo->pagespace), less
 * the amount of unwritten data at the tail (fifo->tailspace),
 * and the amount of already-read space at the head
 * (fifo->pagesize - fifo->headspace).
 */
ulong
byteFifoSize(struct ByteFifo *fifo)
{
	ulong size = fifo->pagespace;
	if (size)
		size = size - fifo->tailspace + \
		fifo->headspace - fifo->pagesize; 
	return size;
}

/*
 * Get some unwritten space to write data into.
 * If fifo->tailspace in non-zero, there is some space preallocated,
 * so return that.  Otherwise, allocate a new page, and return a pointer
 * to that.  (After necessary bookkeeping.)
 */
uchar *
byteFifoGetSpace(struct ByteFifo *fifo, ulong *len)
{
	struct ByteFifoPage *page;

	if (fifo->tailspace) {
		*len = fifo->tailspace;
		return fifo->tailptr;
	}

	page = (ByteFifoPage *)pgp_malloc(sizeof(*page) + fifo->pagesize - 1);
	memset(page ,0 , sizeof(*page) + fifo->pagesize - 1);

	if (!page) {
		*len = 0;
		return (uchar *)0;
	}
	*fifo->tail = page;
	fifo->tail = &page->next;
	page->next = 0;

	if (!fifo->headspace) {
		fifo->headptr = page->data;
		fifo->headspace = fifo->pagesize;
	}

	fifo->pagespace += *len = fifo->tailspace = fifo->pagesize;
	return fifo->tailptr = page->data;
}

/*
 * Skip over some space after it has (presumably) been written to.
 * Advance the pointer and decrease the space available.  The amount
 * skipped cannot exceed the amount of space available for writing.
 * (The return value from the last call to byteFifoGetSpace.)
 */
void
byteFifoSkipSpace(struct ByteFifo *fifo, ulong len)
{
	pgpAssert(len <= fifo->tailspace);

	/* Extra pages are allocated when GetSpace is called */
	fifo->tailspace -= len;
	fifo->tailptr += len;
}

/*
 * Get a pointer to some data at the head of the fifo to read.
 * Just return the head pointers.  The only complexity arises
 * if the head and tail pointers are on the same page, in which case
 * the unwritten data (fifo->tailspace) must be subtracted from the
 * length available.
 */
uchar *
byteFifoGetBytes(struct ByteFifo *fifo, ulong *len)
{
	if (fifo->pagespace == fifo->pagesize)	/* Only one page in use */
		*len = fifo->headspace - fifo->tailspace;
	else
		*len = fifo->headspace;

	return fifo->headptr;
}

/*
 * Advance the read pointer by the given number of bytes, which must not
 * exceed the number of bytes returned from byte FifoGetBytes.
 * Frees skipped-over pages as soon as possible.  There are two cases:
 * - the FIFO becomes empty with the head and tail pointers meeting
 *   in the middle of a page somewhere.
 * - fifo->headspace drops to 0, indicating that the page being read
 *   is finished with and can be freed.
 
 */
void
byteFifoSkipBytes(struct ByteFifo *fifo, ulong len)
{
	struct ByteFifoPage *page;

	pgpAssert(len <= fifo->headspace);

	if (!len)
		return;

	/* Wipe data as soon as possible */
	memset(fifo->headptr, 255, len);

	/* If we've only advanced part of page, it's simple */
	if ((fifo->headspace -= len) != 0) {
		fifo->headptr += len;
		if (fifo->headptr == fifo->tailptr)
			byteFifoDestroy(fifo);
		return;
	}

	/* Entire head page has been read - free it */
	fifo->pagespace -= fifo->pagesize;
	page = fifo->head->next;	/* New head page */
	memset(fifo->head, 0, sizeof(*fifo->head) + fifo->pagesize - 1);
	pgp_free(fifo->head);
	fifo->head = page;

	if (page) {	/* Fifo non-empty */
		fifo->headptr = fifo->head->data;
		fifo->headspace = fifo->pagesize;
	} else {	/* Fifo now empty */
		fifo->tail = &fifo->head;
		fifo->headptr = (uchar *)0;
		fifo->headspace = 0;
	}
}

/*
 * Write data to the byte Fifo.  Return the number of bytes written.
 * This equals len unless a memoryu allocation error has occurred.
 */
ulong
byteFifoWrite(struct ByteFifo *fifo, uchar const *data, ulong len)
{
	uchar *p;
	ulong avail;
	ulong left = len;

	while ((p = byteFifoGetSpace(fifo, &avail)), avail < left) {
		/* Write as much data as there is space available */
		memcpy(p, data, avail);
		data += avail;
		left -= avail;
		byteFifoSkipSpace(fifo, avail);
	}
	if (!p)
		return len - left;

	/* Write final partial block */
	memcpy(p, data, left);
	byteFifoSkipSpace(fifo, left);

	return len;	/* All written */
}

