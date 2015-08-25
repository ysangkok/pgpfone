/*
 * bni.h - Low-level bignum header.
 * Defines various word sizes and useful macros.
 *
 * Written by Colin Plumb
 *
 * $Id: bni.h,v 1.1 1997/12/14 11:30:15 wprice Exp $
 */
#ifndef Included_bni_h
#define Included_bni_h

/*
 * Some compilers complain about #if FOO if FOO isn't defined,
 * so do the ANSI-mandated thing explicitly...
 */
#ifndef NO_LIMITS_H
#define NO_LIMITS_H 0
#endif

/* Make sure we have 8-bit bytes */
#if !NO_LIMITS_H
#include <limits.h>
#if UCHAR_MAX != 0xff || CHAR_BIT != 8
#error The bignum library requires 8-bit unsigned characters.
#endif
#endif /* !NO_LIMITS_H */

#ifdef BNINCLUDE	/* If this is defined as, say, foo.h */
#define STR(x) #x	/* STR(BNINCLUDE) -> "BNINCLUDE" */
#define XSTR(x) STR(x)	/* XSTR(BNINCLUDE) -> STR(foo.h) -> "foo.h" */
#include XSTR(BNINCLUDE)	/* #include "foo.h" */
#undef XSTR
#undef STR
#endif

/* Do we want bnYield()? */
#ifndef BNYIELD
#define BNYIELD 0
#endif

/* Figure out the endianness */
/* Error if more than one is defined */
#if BN_BIG_ENDIAN && BN_LITTLE_ENDIAN
#error Only one of BN_BIG_ENDIAN or BN_LITTLE_ENDIAN may be defined
#endif

/*
 * If no preference is stated, little-endian C code is slightly more
 * efficient, so prefer that.  (The endianness here does NOT have to
 * match the machine's native byte sex; the library's C code will work
 * either way.  The flexibility is allowed for assembly routines
 * that do care.
 */
#if !defined(BN_BIG_ENDIAN) && !defined(BN_LITTLE_ENDIAN)
#define BN_LITTLE_ENDIAN 1
#endif /* !BN_BIG_ENDIAN && !BN_LITTLE_ENDIAN */

/* Macros to choose between big and little endian */
#if BN_BIG_ENDIAN
#define BIG(b) b
#define LITTLE(l) /*nothing*/
#define BIGLITTLE(b,l) b
#elif BN_LITTLE_ENDIAN
#define BIG(b) /*nothing*/
#define LITTLE(l) l
#define BIGLITTLE(b,l) l
#else
#error One of BN_BIG_ENDIAN or BN_LITTLE_ENDIAN must be defined as 1
#endif


/*
 * Find a 16-bit unsigned type.
 * Unsigned short is preferred over unsigned int to make the type chosen
 * by this file more stable on platforms (such as many 68000 compilers)
 * which support both 16- and 32-bit ints.
 */
#ifndef BNWORD16
#ifndef USHRT_MAX	/* No <limits.h> available - guess */
typedef unsigned short bnword16;
#define BNWORD16 bnword16
#elif USHRT_MAX == 0xffff
typedef unsigned short bnword16;
#define BNWORD16 bnword16
#elif UINT_MAX == 0xffff
typedef unsigned bnword16;
#define BNWORD16 bnword16
#endif
#endif /* BNWORD16 */

/*
 * Find a 32-bit unsigned type.
 * Unsigned long is preferred over unsigned int to make the type chosen
 * by this file more stable on platforms (such as many 68000 compilers)
 * which support both 16- and 32-bit ints.
 */
#ifndef BNWORD32
#ifndef ULONG_MAX	/* No <limits.h> available - guess */
typedef unsigned long bnword32;
#define BNWORD32 bnword32
#elif ULONG_MAX == 0xfffffffful
typedef unsigned long bnword32;
#define BNWORD32 bnword32
#elif UINT_MAX == 0xffffffff
typedef unsigned bnword32;
#define BNWORD32 bnword32
#elif USHRT_MAX == 0xffffffff
typedef unsigned short bnword32;
#define BNWORD32 bnword32
#endif
#endif /* BNWORD16 */

/*
 * Find a 64-bit unsigned type.
 * The conditions here are more complicated to avoid using numbers that
 * will choke lesser preprocessors (like 0xffffffffffffffff) unless
 * we're reasonably certain that they'll be acceptable.
 */
#if !defined(BNWORD64) && ULONG_MAX > 0xfffffffful
#if ULONG_MAX == 0xffffffffffffffff
typedef unsigned long bnword64;
#define BNWORD64 bnword64
#endif
#endif

/*
 * I would test the value of unsigned long long, but some *preprocessors*
 * can't parse constants that long even if the compiler can accept them, so it
 * doesn't work reliably.  So cross our fingers and hope that it's a 64-bit
 * type.
 *
 * GCC uses ULONG_LONG_MAX.  Solaris uses ULLONG_MAX.  IRIX and Metrowerks use
 * ULONGLONG_MAX.  Are there any other names for this?
 */

#ifndef BNWORD64
#if defined(ULONG_LONG_MAX)	/* GCC uses this */
typedef unsigned long long bnword64;
#define BNWORD64 bnword64
#elif defined(ULLONG_MAX)	/* Solaris */
/* Solaris defines ULLONG_MAX with ..ll, which its preprocessor chokes on.
 * So we can't test the value.  Just assume that it's 64 bits... */
typedef unsigned long long bnword64;
#define BNWORD64 bnword64
#elif defined(ULONGLONG_MAX) /* IRIX and Metrowerks */
/*
 * Metrowerks always defines ULONGLONG_MAX, but it may be
 * equal to ULONG_MAX in some cases, so we have to test its value.
 * Unformtunately, its preprocessor silently truncates to 32 bits!
 */
#if ULONGLONG_MAX > ULONG_MAX
/* Case 1: preprocessor works */
typedef unsigned long long bnword64;
#define BNWORD64 bnword64
#elif LONGLONG_MAX == LONGLONG_MIN
/* Case 2: Preprocessor silently truncates to 32 bits */
typedef unsigned long long bnword64;
#define BNWORD64 bnword64
#endif
#endif /* ULONGLONG_MAX */
#endif /* !BNWORD64 */

/* We don't even try to find a 128-bit type at the moment */

#endif /* !Included_bni_h */
