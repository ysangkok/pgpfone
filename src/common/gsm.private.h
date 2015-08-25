/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

#ifndef GSMPRIVATE_H
#define GSMPRIVATE_H

typedef short                   word;           /* 16 bit signed int    */
typedef long                    longword;       /* 32 bit signed int    */

typedef unsigned short          uword;          /* unsigned word        */
typedef unsigned long           ulongword;      /* unsigned longword    */

struct gsm_state {
	word            dp0[ 280 ];
	word            z1;             /* preprocessing.c, Offset_com. */
	longword        L_z2;           /*                  Offset_com. */
	int             mp;             /*                  Preemphasis */
	word            u[8];           /* short_term_aly_filter.c      */
	word            LARpp[2][8];    /*                              */
	word            j;              /*                              */
	word            ltp_cut;        /* long_term.c, LTP crosscorr.  */
	word            nrp; /* 40 */   /* long_term.c, synthesis       */
	word            v[9];           /* short_term.c, synthesis      */
	word            msr;            /* decoder.c,   Postprocessing  */
	char            verbose;        /* only used if !NDEBUG         */
	char            fast;           /* only used if FAST            */
};

#endif