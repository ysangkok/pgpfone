/*
 * bnprint.c - Print a bignum, for debugging purposes.
 *
 * Written by Colin Plumb.
 *
 * $Id: bnprint.c,v 1.1 1997/12/14 11:30:37 wprice Exp $
 */
#include "pgpConfig.h"
#include "pgpMem.h"

/*
 * Some compilers complain about #if FOO if FOO isn't defined,
 * so do the ANSI-mandated thing explicitly...
 */
#ifndef NO_STRING_H
#define NO_STRING_H 0
#endif
#ifndef HAVE_STRINGS_H
#define HAVE_STRINGS_H 0
#endif

#include <stdio.h>

#if !NO_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

#include "bn.h"
#include "bnprint.h"

#include "bnkludge.h"

int
bnPrint(FILE *f, char const *prefix, BigNum const *bn,
	char const *suffix)
{
	unsigned char temp[32];	/* How much to print on one line */
	unsigned len;
	size_t i;

	if (prefix && fputs(prefix, f) < 0)
		return EOF;

	len = (bnBits(bn) + 7)/ 8;

	if (!len) {
		if (putc('0', f) < 0)
			return EOF;
	} else {
		while (len > sizeof(temp)) {
			len -= sizeof(temp);
			bnExtractBigBytes(bn, temp, len, sizeof(temp));
			for (i = 0; i < sizeof(temp); i++)
				if (fprintf(f, "%02X", temp[i]) < 0)
					return EOF;
			if (putc('\\', f) < 0 || putc('\n', f) < 0)
				return EOF;
			if (prefix) {
				i = strlen(prefix);
				while (i--)
					if (putc(' ', f) < 0)
						return EOF;
			}
		}
		bnExtractBigBytes(bn, temp, 0, len);
		for (i = 0; i < len; i++)
			if (fprintf(f, "%02X", temp[i]) < 0)
				return EOF;
	}
	return suffix ?	fputs(suffix, f) : 0;
}


int
bnPrint10(FILE *f, char const *prefix, BigNum const *bn,
	char const *suffix)
{
	BigNum pbig, pbig1;
	BigNum ten, zero, rem;
	char buf[3000];			/* up to 9000 bits */
	int bufi = sizeof(buf)-1;
	int n;

	buf[bufi] = '\0';
	bnBegin (&pbig);
	bnBegin (&pbig1);
	bnBegin (&rem);
	
	bnBegin (&ten);
	bnAddQ (&ten, 10);
	bnBegin (&zero);

	bnCopy (&pbig, bn);
	
	while (bnCmp (&pbig, &zero) != 0) {
		bnDivMod (&pbig1, &rem, &pbig, &ten);
		bnCopy (&pbig, &pbig1);
		n = bnLSWord (&rem);
		buf[--bufi] = n + '0';
	}

	bnEnd (&pbig);
	bnEnd (&pbig1);
	bnEnd (&rem);

	if (prefix && fputs(prefix, f) < 0)
		return EOF;

	fputs (buf+bufi, f);
	pgpClearMemory (buf, sizeof(buf));

	return suffix ?	fputs(suffix, f) : 0;
}
