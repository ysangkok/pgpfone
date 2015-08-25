#ifndef FASTPOOL_H
#define FASTPOOL_H

#include "PGPFone.h"	/* For uchar typedef */

void randPoolAddWord(ulong w);
void randPoolAddBytes(uchar const *p, unsigned len);
void randPoolGetBytes(uchar *p, unsigned len);

#endif	// FASTPOOL_H

