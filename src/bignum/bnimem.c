/*
 * bnimem.c - low-level bignum memory handling.
 *
 * Written by Colin Plumb
 *
 * $Id: bnimem.c,v 1.4 1999/02/15 08:03:45 wprice Exp $
 *
 * Note that in all cases, the pointers passed around
 * are pointers to the *least* significant end of the word.
 * On big-endian machines, these are pointers to the *end*
 * of the allocated range.
 *
 * BNSECURE is a simple level of security; for more security
 * change these function to use locked unswappable memory.
 */

/*
 * Some compilers complain about #if FOO if FOO isn't defined,
 * so do the ANSI-mandated thing explicitly...
 */
#ifndef NO_STDLIB_H
#define NO_STDLIB_H 0
#endif
#ifndef NO_STRING_H
#define NO_STRING_H 0
#endif
#ifndef HAVE_STRINGS_H
#define HAVE_STRINGS_H 0
#endif
#ifndef NEED_MEMORY_H
#define NEED_MEMORY_H 0
#endif

#ifdef PGP_MACINTOSH
#include <string.h>
#endif

#if !NO_STDLIB_H
#include <stdlib.h>	/* For malloc() & co. */
#else
void *malloc();
void *realloc();
void free();
#endif

#if NEED_MEMORY_H
#include <memory.h>
#endif

#ifndef DBMALLOC
#define DBMALLOC 0
#endif

#include "bni.h"
#include "bnimem.h"

#include "bnkludge.h"

#ifndef bniMemWipe
void
bniMemWipe(void *ptr, unsigned bytes)
{
	memset( ptr, bytes, 0 );
}
#define bniMemWipe(ptr, bytes) memset( ptr, bytes, 0 );
#endif

#ifndef bniMemAlloc
void *
bniMemAlloc(unsigned bytes)
{
	return malloc(bytes);
}
#define bniMemAlloc(bytes) malloc(bytes)
#endif

#ifndef bniMemFree
void
bniMemFree(void *ptr, unsigned bytes)
{
	bniMemWipe(ptr, bytes);
	free(ptr);
}
#endif

#ifndef bniRealloc
#if defined(bniMemRealloc) || !BNSECURE
void *
bniRealloc(void *ptr, unsigned oldbytes, unsigned newbytes)
{
	if (ptr) {
		BIG(ptr = (char *)ptr - oldbytes;)
		if (newbytes < oldbytes)
			memmove(ptr, (char *)ptr + oldbytes-newbytes,
						oldbytes);
	}
#ifdef bniMemRealloc
	ptr = bniMemRealloc(ptr, oldbytes, newbytes);
#else
	ptr = realloc(ptr, newbytes);
#endif
	if (ptr) {
		if (newbytes > oldbytes)
			memmove((char *)ptr + newbytes-oldbytes, ptr,
						oldbytes);
		BIG(ptr = (char *)ptr + newbytes;)
	}

	return ptr;
}

#else /* BNSECURE */

void *
bniRealloc(void *oldptr, unsigned oldbytes, unsigned newbytes)
{
	void *newptr = bniMemAlloc(newbytes);

	if (!newptr)
		return newptr;
	if (!oldptr)
		return BIGLITTLE((char *)newptr+newbytes, newptr);

	/*
	 * The following copies are a bit non-obvious in the big-endian case
	 * because one of the pointers points to the *end* of allocated memory.
	 */
	if (newbytes > oldbytes) {	/* Copy all of old into part of new */
		BIG(newptr = (char *)newptr + newbytes;)
		BIG(oldptr = (char *)oldptr - oldbytes;)
		memcpy(BIGLITTLE((char *)newptr-oldbytes, newptr), oldptr,
		       oldbytes);
	} else {	/* Copy part of old into all of new */
		memcpy(newptr, BIGLITTLE((char *)oldptr-newbytes, oldptr),
		       newbytes);
		BIG(newptr = (char *)newptr + newbytes;)
		BIG(oldptr = (char *)oldptr - oldbytes;)
	}

	bniMemFree(oldptr, oldbytes);
	return newptr;
}
#endif /* BNSECURE */
#endif /* !bniRealloc */
