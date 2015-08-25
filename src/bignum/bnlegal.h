/*
 * ANSI C stadard, section 3.5.3: "An object that has volatile-qualified
 * type may be modified in ways unknown to the implementation or have
 * other unknown side effects."  Yes, we can't expect a compiler to
 * understand law...
 *
 * $Id: bnlegal.h,v 1.2 1998/09/20 12:03:42 wprice Exp $
 */


extern volatile const char bnCopyright[];

