/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 *
 * 68K assembler routines by Curve Software, 6/95. To turn on 68K assembler,
 * #define TARGET_68K.
 */
#include        <stdio.h>
#define NDEBUG
#include        <assert.h>
#include        <stdlib.h>
#include		<string.h>

#include        "gsm.private.h"
#include        "gsm.h"

#ifdef PGP_MACINTOSH
#include "PGPfone.h"
#endif

/*
 * #define BUG_FIX to correct bug in the C version of Short_term_synthesis_filtering().
 */
#define BUG_FIX

/*
 * Set TARGET_X86 for 80x86 assembler routines.
 */
#ifdef PGP_WIN32
//#define TARGET_X86
#endif

#define MIN_WORD        ((-32767)-1)
#define MAX_WORD        ( 32767)

#define MIN_LONGWORD    ((-2147483647L)-1L)
#define MAX_LONGWORD    ( 2147483647L)

#define SASR(x, by)     ((x) >> (by))

#define GSM_MULT_R(a, b) /* word a, word b, !(a == b == MIN_WORD) */ \
        (SASR( ((longword)(a) * (longword)(b) + 16384), 15 ))

# define GSM_MULT(a,b)   /* word a, word b, !(a == b == MIN_WORD) */  \
        (SASR( ((longword)(a) * (longword)(b)), 15 ))

# define GSM_L_MULT(a, b) /* word a, word b     */                       \
        (((longword)(a) * (longword)(b)) << 1)

#ifdef PGP_WIN32
#define pgpAssert assert
#endif

#ifdef TARGET_X86
# include        "asmcode.h"*/
# define GSM_L_ADD(a, b)        Fgsm_L_add(a, b)
# define GSM_L_SUB(a, b)        Fgsm_L_sub(a,b)
# define GSM_ADD(a, b)          Fgsm_add(a, b)
# define GSM_SUB(a, b)          Fgsm_sub(a, b)
# define GSM_ABS(a)             Fgsm_abs(a)
# define GSM_ST_ANAL_FILT       FShort_term_analysis_filtering
# define GSM_ST_SYNTH_FILT      FShort_term_synthesis_filtering
# define GSM_LTP_LOOP           FLTP_loop_internal
#else
# define GSM_L_ADD(a,b)         gsm_l_add(a,b)
# define GSM_L_SUB(a,b)         gsm_l_sub(a,b)
# define GSM_ADD(a,b)           gsm_add(a,b)
# define GSM_SUB(a,b)           gsm_sub(a,b)
# define GSM_ABS(a)             gsm_abs(a)
# define GSM_ST_ANAL_FILT       Short_term_analysis_filtering
# define GSM_ST_SYNTH_FILT      Short_term_synthesis_filtering
# define GSM_LTP_LOOP           LTP_loop_internal

#ifdef __MC68K__

asm word  gsm_add(word warg1, word warg2)
{
    fralloc+
    move.w warg1,d0
    add.w warg2,d0
    bvc @end
    bmi @pos
    move.w #-0x8000,d0
    bra @end
@pos:
    move.w #0x7FFF, d0
@end:
    frfree
    rts
}

asm word  gsm_sub(word warg1, word warg2)
{
	fralloc+
    move.w warg1,d0
    sub.w warg2, d0
    bvc @end
    bmi @pos
    move.w #-0x8000,d0
    bra @end
@pos:
    move.w #0x7FFF, d0
@end:
	frfree
	rts
}

asm longword  gsm_l_add(longword warg1, longword warg2)
{
	fralloc+
    move.l warg1,d0
    add.l warg2,d0
    bvc @end
    bmi @pos
    move.l #0x8000000,d0
    bra @end
@pos:
    move.l #0x7FFFFFFF, d0
@end:
	frfree
	rts
}

asm longword  gsm_l_sub(longword warg1, longword warg2)
{
    fralloc+
    move.l warg1,d0
    sub.l warg2, d0
    bvc @end
    bmi @pos
    move.l #0x80000000,d0
    bra @end
@pos:
    move.l #0x7FFFFFFF, d0
@end:
    frfree
    rts
}

#else	/*	__MC68K__	*/

static word  gsm_add(word warg1, word warg2) {
  long x;
  x = ((long) warg1) + ((long) warg2);
  if (x<MIN_WORD) return MIN_WORD;
  if (x>MAX_WORD) return MAX_WORD;
  return x;
}

static word  gsm_sub(word warg1, word warg2) {
  long x;
  x = ((long) warg1) - ((long) warg2);
  if (x<MIN_WORD) return MIN_WORD;
  if (x>MAX_WORD) return MAX_WORD;
  return x;
}

static longword  gsm_l_add(longword a, longword b)
{
        if (a < 0) {
                if (b >= 0) return a + b;
                else {
                        ulongword A = (ulongword)-(a + 1) + (ulongword)-(b + 1);
                        return A >= MAX_LONGWORD ? MIN_LONGWORD :-(longword)A-2;
                }
        }
        else if (b <= 0) return a + b;
        else {
                ulongword A = (ulongword)a + (ulongword)b;
                return A > MAX_LONGWORD ? MAX_LONGWORD : A;
        }
}

static longword  gsm_l_sub(longword a, longword b) {
        if (a >= 0) {
                if (b >= 0) return a - b;
                else {
                        /* a>=0, b<0 */

                        ulongword A = (ulongword)a + -(b + 1);
                        return A >= MAX_LONGWORD ? MAX_LONGWORD : (A + 1);
                }
        }
        else if (b <= 0) return a - b;
        else {
                /* a<0, b>0 */

                ulongword A = (ulongword)-(a + 1) + b;
                return A >= MAX_LONGWORD ? MIN_LONGWORD : -(longword)A - 1;
        }
}


#endif	/*	__MC68K__	*/

static word  gsm_abs(word a)
{
        return a < 0 ? (a == MIN_WORD ? MAX_WORD : -a) : a;
}

#endif	/*	TARGET_X86	*/

static void Gsm_LPC_Analysis(
        word             * s,           /* 0..159 signals       IN/OUT  */
        word             * LARc);       /* 0..7   LARc's        OUT     */
static void Gsm_Short_Term_Analysis_Filter(
        struct gsm_state * S,
        word    * LARc,         /* coded log area ratio [0..7]  IN      */
        word    * s);           /* signal [0..159]              IN/OUT  */
static void Gsm_Long_Term_Predictor(
        struct gsm_state        * S,
        word    * d,    /* [0..39]   residual signal    IN      */
        word    * dp,   /* [-120..-1] d'                IN      */
        word    * e,    /* [0..39]                      OUT     */
        word    * dpp,  /* [0..39]                      OUT     */
        word    * Nc,   /* correlation lag              OUT     */
        word    * bc);  /* gain factor                  OUT     */
static void Gsm_RPE_Encoding(
        word    * e,            /* -5..-1][0..39][40..44        IN/OUT  */
        word    * xmaxc,        /*                              OUT */
        word    * Mc,           /*                              OUT */
        word    * xMc);         /* [0..12]                      OUT */
static void Gsm_RPE_Decoding(
        word            xmaxcr,
        word            Mcr,
        word            * xMcr,  /* [0..12], 3 bits             IN      */
        word            * erp ); /* [0..39]                     OUT     */
static void Gsm_Long_Term_Synthesis_Filtering(
        struct gsm_state        * S,
        word                    Ncr,
        word                    bcr,
        register word           * erp,          /* [0..39]                IN */
        register word           * drp           /* [-120..-1] IN, [0..40] OUT */
);
static void Gsm_Short_Term_Synthesis_Filter(
        struct gsm_state * S,
        word    * LARcr,        /* received log area ratios [0..7] IN  */
        word    * wt,           /* received d [0..159]             IN  */
        word    * s             /* signal   s [0..159]            OUT  */
);
static void Quantization_and_coding(
        register word * LAR     /* [0..7]       IN/OUT  */
);

static unsigned char bitoff[ 256 ] = {
         8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
         2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
         2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

short gsm_option( gsm r, short opt, short * val)
{
	short 	result = -1;

	switch (opt) {
	case GSM_OPT_LTP_CUT:
#ifdef 	LTP_CUT
		result = r->ltp_cut;
		if (val) r->ltp_cut = *val;
#endif
		break;

	case GSM_OPT_VERBOSE:
#ifndef	NDEBUG
		result = r->verbose;
		if (val) r->verbose = *val;
#endif
		break;

	case GSM_OPT_FAST:

#if	defined(FAST) && defined(USE_FLOAT_MUL)
		result = r->fast;
		if (val) r->fast = !!*val;
#endif
		break;

	default:
		break;
	}
	return result;
}

static word  gsm_norm(longword a )
/*
 * the number of left shifts needed to normalize the 32 bit
 * variable L_var1 for positive values on the interval
 *
 * with minimum of
 * minimum of 1073741824  (01000000000000000000000000000000) and
 * maximum of 2147483647  (01111111111111111111111111111111)
 *
 *
 * and for negative values on the interval with
 * minimum of -2147483648 (-10000000000000000000000000000000) and
 * maximum of -1073741824 ( -1000000000000000000000000000000).
 *
 * in order to normalize the result, the following
 * operation must be done: L_norm_var1 = L_var1 << norm( L_var1 );
 *
 * (That's 'ffs', only from the left, not the right..)
 */
{
        pgpAssert(a != 0);

        if (a < 0) {
                if (a <= -1073741824L) return 0;
                a = ~a;
        }

        return    a & 0xffff0000L
                ? ( a & 0xff000000L
                  ?  -1 + bitoff[ 0xFF & (a >> 24) ]
                  :   7 + bitoff[ 0xFF & (a >> 16) ] )
                : ( a & 0xff00
                  ?  15 + bitoff[ 0xFF & (a >> 8) ]
                  :  23 + bitoff[ 0xFF & a ] );
}

static word  gsm_asr( word a, int n)
{
        if (n >= 16) return -(a < 0);
        if (n <= -16) return 0;
        if (n < 0) return a << -n;
        return a >> n;
}

static word  gsm_asl(word a, int n)
{
        if (n >= 16) return 0;
        if (n <= -16) return -(a < 0);
        if (n < 0) return gsm_asr(a, -n);
        return a << n;
}

/*
 *  (From p. 46, end of section 4.2.5)
 *
 *  NOTE: The following lines gives [sic] one correct implementation
 *        of the div(num, denum) arithmetic operation.  Compute div
 *        which is the integer division of num by denum: with denum
 *        >= num > 0
 */

static word  gsm_div(word num, word denum)
{
        longword        L_num   = num;
        longword        L_denum = denum;
        word            div     = 0;
        int             k       = 15;

        /* The parameter num sometimes becomes zero.
         * Although this is explicitly guarded against in 4.2.5,
         * we assume that the result should then be zero as well.
         */

        /* assert(num != 0); */

        pgpAssert(num >= 0 && denum >= num);
        if (num == 0)
            return 0;

        while (k--) {
                div   <<= 1;
                L_num <<= 1;

                if (L_num >= L_denum) {
                        L_num -= L_denum;
                        div++;
                }
        }

        return div;
}

/*  4.4 TABLES USED IN THE FIXED POINT IMPLEMENTATION OF THE RPE-LTP
 *      CODER AND DECODER
 *
 *      (Most of them inlined, so watch out.)
 */

/*   Table 4.3a  Decision level of the LTP gain quantizer
 */
/*  bc                0         1         2          3                  */
word gsm_DLB[4] = {  6554,    16384,    26214,     32767        };


/*   Table 4.3b   Quantization levels of the LTP gain quantizer
 */
/* bc                 0          1        2          3                  */
static word gsm_QLB[4] = {  3277,    11469,     21299,     32767        };

/*   Table 4.5   Normalized inverse mantissa used to compute xM/xmax
 */
/* i                    0        1    2      3      4      5     6      7   */
static word gsm_NRFAC[8] = { 29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384 };


/*   Table 4.6   Normalized direct mantissa used to compute xM/xmax
 */
/* i                  0      1       2      3      4      5      6      7   */
static word gsm_FAC[8]  = { 18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767 };

/*      4.2.0 .. 4.2.3  PREPROCESSING SECTION
 *  
 *      After A-law to linear conversion (or directly from the
 *      Ato D converter) the following scaling is assumed for
 *      input to the RPE-LTP algorithm:
 *
 *      in:  0.1.....................12
 *           S.v.v.v.v.v.v.v.v.v.v.v.v.*.*.*
 *
 *      Where S is the sign bit, v a valid bit, and * a "don't care" bit.
 *      The original signal is called sop[..]
 *
 *      out:   0.1................... 12 
 *           S.S.v.v.v.v.v.v.v.v.v.v.v.v.0.0
 */


static void Gsm_Preprocess(
        struct gsm_state * S,
        word             * s,
        word             * so )         /* [0..159]     IN/OUT  */
{

        word       z1 = S->z1;
        longword L_z2 = S->L_z2;
        word       mp = S->mp;

        word            s1;
        longword      L_s2;

        longword      L_temp;

        word            msp, lsp;
        word            SO;

/*      longword        ltmp;           /* for   ADD */
/*      ulongword       utmp;           /* for L_ADD */

        register int            k = 160;

        while (k--) {

        /*  4.2.1   Downscaling of the input signal
         */
                SO = SASR( *s, 3 ) << 2;
                s++;

                pgpAssert (SO >= -0x4000); /* downscaled by     */
                pgpAssert (SO <=  0x3FFC); /* previous routine. */


        /*  4.2.2   Offset compensation
         * 
         *  This part implements a high-pass filter and requires extended
         *  arithmetic precision for the recursive part of this filter.
         *  The input of this procedure is the array so[0...159] and the
         *  output the array sof[ 0...159 ].
         */
                /*   Compute the non-recursive part
                 */

                s1 = SO - z1;                   /* s1 = gsm_sub( *so, z1 ); */
                z1 = SO;

                pgpAssert(s1 != MIN_WORD);

                /*   Compute the recursive part
                 */
                L_s2 = s1;
                L_s2 <<= 15;

                /*   Execution of a 31 bv 16 bits multiplication
                 */

                msp = SASR( L_z2, 15 );
                lsp = GSM_L_SUB(L_z2,(msp<<15));

                L_s2  += GSM_MULT_R( lsp, 32735 );
                L_temp = (longword)msp * 32735; /* GSM_L_MULT(msp,32735) >> 1;*/
                L_z2   = GSM_L_ADD( L_temp, L_s2 );

                /*    Compute sof[k] with rounding
                 */
                L_temp = GSM_L_ADD( L_z2, 16384 );

        /*   4.2.3  Preemphasis
         */

                msp   = GSM_MULT_R( mp, -28180 );
                mp    = SASR( L_temp, 15 );
                *so++ = GSM_ADD( mp, msp );
        }

        S->z1   = z1;
        S->L_z2 = L_z2;
        S->mp   = mp;
}

/*
 *  4.2 FIXED POINT IMPLEMENTATION OF THE RPE-LTP CODER
 */

static void Gsm_Coder(
        struct gsm_state        * S,
        word    * s,    /* [0..159] samples                     IN      */
/*
 * The RPE-LTD coder works on a frame by frame basis.  The length of
 * the frame is equal to 160 samples.  Some computations are done
 * once per frame to produce at the output of the coder the
 * LARc[1..8] parameters which are the coded LAR coefficients and
 * also to realize the inverse filtering operation for the entire
 * frame (160 samples of signal d[0..159]).  These parts produce at
 * the output of the coder:
 */
        word    * LARc, /* [0..7] LAR coefficients              OUT     */
/*
 * Procedure 4.2.11 to 4.2.18 are to be executed four times per
 * frame.  That means once for each sub-segment RPE-LTP analysis of
 * 40 samples.  These parts produce at the output of the coder:
 */
        word    * Nc,   /* [0..3] LTP lag                       OUT     */
        word    * bc,   /* [0..3] coded LTP gain                OUT     */
        word    * Mc,   /* [0..3] RPE grid selection            OUT     */
        word    * xmaxc,/* [0..3] Coded maximum amplitude       OUT     */
        word    * xMc   /* [13*4] normalized RPE samples        OUT     */
)
{
        int     k;
        word    * dp  = S->dp0 + 120;   /* [ -120...-1 ] */
        word    * dpp = dp;             /* [ 0...39 ]    */

        static word     e [50] = {0};

        word    so[160];

        Gsm_Preprocess                  (S, s, so);
        Gsm_LPC_Analysis                (so, LARc);
        Gsm_Short_Term_Analysis_Filter  (S, LARc, so);

        for (k = 0; k <= 3; k++, xMc += 13) {

                Gsm_Long_Term_Predictor ( S,
                                         so+k*40, /* d      [0..39] IN  */
                                         dp,      /* dp  [-120..-1] IN  */
                                        e + 5,    /* e      [0..39] OUT */
                                        dpp,      /* dpp    [0..39] OUT */
                                         Nc++,
                                         bc++);

                Gsm_RPE_Encoding        (
                                        e + 5,  /* e      ][0..39][ IN/OUT */
                                          xmaxc++, Mc++, xMc );
                /*
                 * Gsm_Update_of_reconstructed_short_time_residual_signal
                 *                      ( dpp, e + 5, dp );
                 */

                { register int i;
                  for (i = 0; i <= 39; i++)
                        dp[ i ] = GSM_ADD( e[5 + i], dpp[i] );
                }
                dp  += 40;
                dpp += 40;

        }
        (void)memcpy( (char *)S->dp0, (char *)(S->dp0 + 160),
                120 * sizeof(*S->dp0) );
}

/*
 *  4.3 FIXED POINT IMPLEMENTATION OF THE RPE-LTP DECODER
 */

static void Postprocessing(
        struct gsm_state        * S,
        register word           * s)
{
        register int            k;
        register word           msr = S->msr;
        register word           tmp;

        for (k = 160; k--; s++) {
                tmp = GSM_MULT_R( msr, 28180 );
                msr = GSM_ADD(*s, tmp);            /* Deemphasis             */
                *s  = GSM_ADD(msr, msr) & 0xFFF8;  /* Truncation & Upscaling */
        }
        S->msr = msr;
}

static void Gsm_Decoder(
        struct gsm_state        * S,

        word            * LARcr,        /* [0..7]               IN      */

        word            * Ncr,          /* [0..3]               IN      */
        word            * bcr,          /* [0..3]               IN      */
        word            * Mcr,          /* [0..3]               IN      */
        word            * xmaxcr,       /* [0..3]               IN      */
        word            * xMcr,         /* [0..13*4]            IN      */

        word            * s)            /* [0..159]             OUT     */
{
        int             j, k;
        word            erp[40], wt[160];
        word            * drp = S->dp0 + 120;

        for (j=0; j <= 3; j++, xmaxcr++, bcr++, Ncr++, Mcr++, xMcr += 13) {

                Gsm_RPE_Decoding( *xmaxcr, *Mcr, xMcr, erp );
                Gsm_Long_Term_Synthesis_Filtering( S, *Ncr, *bcr, erp, drp );

                for (k = 0; k <= 39; k++) wt[ j * 40 + k ] =  drp[ k ];
        }

        Gsm_Short_Term_Synthesis_Filter( S, LARcr, wt, s );
        Postprocessing(S, s);
}

/*
 *  4.2.4 .. 4.2.7 LPC ANALYSIS SECTION
 */

/* 4.2.4 */


static void Autocorrelation(
        word     * s,           /* [0..159]     IN/OUT  */
        longword * L_ACF)       /* [0..8]       OUT     */
/*
 *  The goal is to compute the array L_ACF[k].  The signal s[i] must
 *  be scaled in order to avoid an overflow situation.
 */
{
        register int    k, i;

        word            temp, smax, scalauto;

        /*  Dynamic scaling of the array  s[0..159]
         */

        /*  Search for the maximum.
         */
        smax = 0;
        for (k = 0; k <= 159; k++) {
                temp = GSM_ABS( s[k] );
                if (temp > smax) smax = temp;
        }

        /*  Computation of the scaling factor.
         */
        if (smax == 0) scalauto = 0;
        else {
                pgpAssert(smax > 0);
                scalauto = 4 - gsm_norm( (longword)smax << 16 );/* sub(4,..) */
        }

        /*  Scaling of the array s[0...159]
         */

        if (scalauto > 0) {

#   define SCALE(n)     \
        case n: for (k = 0; k <= 159; k++) \
                        s[k] = GSM_MULT_R( s[k], 16384 >> (n-1) );\
                break;

                switch (scalauto) {
                SCALE(1)
                SCALE(2)
                SCALE(3)
                SCALE(4)
                }
# undef SCALE
        }

        /*  Compute the L_ACF[..].
         */
        {
                word  * sp = s;
                word    sl = *sp;

#               define STEP(k)   L_ACF[k] += ((longword)sl * sp[ -(k) ]);

#       define NEXTI     sl = *++sp


        for (k = 9; k--; L_ACF[k] = 0) ;

        STEP (0);
        NEXTI;
        STEP(0); STEP(1);
        NEXTI;
        STEP(0); STEP(1); STEP(2);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5); STEP(6);
        NEXTI;
        STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5); STEP(6); STEP(7);

        for (i = 8; i <= 159; i++) {

                NEXTI;

                STEP(0);
                STEP(1); STEP(2); STEP(3); STEP(4);
                STEP(5); STEP(6); STEP(7); STEP(8);
        }

        for (k = 9; k--; L_ACF[k] <<= 1) ;

        }
        /*   Rescaling of the array s[0..159]
         */
        if (scalauto > 0) {
                pgpAssert(scalauto <= 4);
                for (k = 160; k--; *s++ <<= scalauto) ;
        }
}

/* 4.2.5 */

static void Reflection_coefficients(
        longword        * L_ACF,                /* 0...8        IN      */
        register word   * r                     /* 0...7        OUT     */
)
{
        register int    i, m, n;
        register word   temp;
/*      register longword ltmp; */
        word            ACF[9]; /* 0..8 */
        word            P[  9]; /* 0..8 */
        word            K[  9]; /* 2..8 */

        /*  Schur recursion with 16 bits arithmetic.
         */

        if (L_ACF[0] == 0) {
                for (i = 8; i--; *r++ = 0) ;
                return;
        }

        pgpAssert( L_ACF[0] != 0 );
        temp = gsm_norm( L_ACF[0] );

        pgpAssert(temp >= 0 && temp < 32);

        /* ? overflow ? */
        for (i = 0; i <= 8; i++) ACF[i] = SASR( L_ACF[i] << temp, 16 );

        /*   Initialize array P[..] and K[..] for the recursion.
         */

        for (i = 1; i <= 7; i++) K[ i ] = ACF[ i ];
        for (i = 0; i <= 8; i++) P[ i ] = ACF[ i ];

        /*   Compute reflection coefficients
         */
        for (n = 1; n <= 8; n++, r++) {

                temp = P[1];
                temp = GSM_ABS(temp);
                if (P[0] < temp) {
                        for (i = n; i <= 8; i++) *r++ = 0;
                        return;
                }

                *r = gsm_div( temp, P[0] );

                pgpAssert(*r >= 0);
                if (P[1] > 0) *r = -*r;         /* r[n] = sub(0, r[n]) */
                pgpAssert (*r != MIN_WORD);
                if (n == 8) return;

                /*  Schur recursion
                 */
                temp = GSM_MULT_R( P[1], *r );
                P[0] = GSM_ADD( P[0], temp );

                for (m = 1; m <= 8 - n; m++) {
                        temp     = GSM_MULT_R( K[ m   ],    *r );
                        P[m]     = GSM_ADD(    P[ m+1 ],  temp );

                        temp     = GSM_MULT_R( P[ m+1 ],    *r );
                        K[m]     = GSM_ADD(    K[ m   ],  temp );
                }
        }
}

/* 4.2.6 */

static void Transformation_to_Log_Area_Ratios(
        register word   * r                     /* 0..7    IN/OUT */
)
/*
 *  The following scaling for r[..] and LAR[..] has been used:
 *
 *  r[..]   = integer( real_r[..]*32768. ); -1 <= real_r < 1.
 *  LAR[..] = integer( real_LAR[..] * 16384 );
 *  with -1.625 <= real_LAR <= 1.625
 */
{
        register word   temp;
        register int    i;


        /* Computation of the LAR[0..7] from the r[0..7]
         */
        for (i = 1; i <= 8; i++, r++) {

                temp = *r;
                temp = GSM_ABS(temp);
                pgpAssert(temp >= 0);

                if (temp < 22118) {
                        temp >>= 1;
                } else if (temp < 31130) {
                        pgpAssert( temp >= 11059 );
                        temp -= 11059;
                } else {
                        pgpAssert( temp >= 26112 );
                        temp -= 26112;
                        temp <<= 2;
                }

                *r = *r < 0 ? -temp : temp;
                pgpAssert( *r != MIN_WORD );
        }
}

static void Gsm_LPC_Analysis(
        word             * s,           /* 0..159 signals       IN/OUT  */
        word             * LARc)        /* 0..7   LARc's        OUT     */
{
        longword        L_ACF[9];

        Autocorrelation                   (s,     L_ACF );
        Reflection_coefficients           (L_ACF, LARc  );
        Transformation_to_Log_Area_Ratios (LARc);
        Quantization_and_coding           (LARc);
}



/* 4.2.7 */

static void Quantization_and_coding(
        register word * LAR     /* [0..7]       IN/OUT  */
)
{
        register word   temp;
/* longword     ltmp; */


        /*  This procedure needs four tables; the following equations
         *  give the optimum scaling for the constants:
         *
         *  A[0..7] = integer( real_A[0..7] * 1024 )
         *  B[0..7] = integer( real_B[0..7] *  512 )
         *  MAC[0..7] = maximum of the LARc[0..7]
         *  MIC[0..7] = minimum of the LARc[0..7]
         */

#       undef STEP
#       define  STEP( A, B, MAC, MIC )          \
                temp = GSM_MULT( A,   *LAR );   \
                temp = GSM_ADD(  temp,   B );   \
                temp = GSM_ADD(  temp, 256 );   \
                temp = SASR(     temp,   9 );   \
                *LAR  =  temp>MAC ? MAC - MIC : (temp<MIC ? 0 : temp - MIC); \
                LAR++;

        STEP(  20480,     0,  31, -32 );
        STEP(  20480,     0,  31, -32 );
        STEP(  20480,  2048,  15, -16 );
        STEP(  20480, -2560,  15, -16 );

        STEP(  13964,    94,   7,  -8 );
        STEP(  15360, -1792,   7,  -8 );
        STEP(   8534,  -341,   3,  -4 );
        STEP(   9036, -1144,   3,  -4 );

#       undef   STEP
}


/*
 *  SHORT TERM ANALYSIS FILTERING SECTION
 */

/* 4.2.8 */

static void Decoding_of_the_coded_Log_Area_Ratios(
        word    * LARc,         /* coded log area ratio [0..7]  IN      */
        word    * LARpp)        /* out: decoded ..                      */
{
        register word   temp1 /* , temp2 */;
/*      register long   ltmp;   /* for GSM_ADD */

        /*  This procedure requires for efficient implementation
         *  two tables.
         *
         *  INVA[1..8] = integer( (32768 * 8) / real_A[1..8])
         *  MIC[1..8]  = minimum value of the LARc[1..8]
         */

        /*  Compute the LARpp[1..8]
         */

        /*      for (i = 1; i <= 8; i++, B++, MIC++, INVA++, LARc++, LARpp++) {
         *
         *              temp1  = GSM_ADD( *LARc, *MIC ) << 10;
         *              temp2  = *B << 1;
         *              temp1  = GSM_SUB( temp1, temp2 );
         *
         *              pgpAssert(*INVA != MIN_WORD);
         *
         *              temp1  = GSM_MULT_R( *INVA, temp1 );
         *              *LARpp = GSM_ADD( temp1, temp1 );
         *      }
         */

#undef  STEP
#define STEP( B, MIC, INVA )    \
                temp1    = GSM_ADD( *LARc++, MIC ) << 10;       \
                temp1    = GSM_SUB( temp1, B << 1 );            \
                temp1    = GSM_MULT_R( INVA, temp1 );           \
                *LARpp++ = GSM_ADD( temp1, temp1 );

        STEP(      0,  -32,  13107 );
        STEP(      0,  -32,  13107 );
        STEP(   2048,  -16,  13107 );
        STEP(  -2560,  -16,  13107 );

        STEP(     94,   -8,  19223 );
        STEP(  -1792,   -8,  17476 );
        STEP(   -341,   -4,  31454 );
        STEP(  -1144,   -4,  29708 );

        /* NOTE: the addition of *MIC is used to restore
         *       the sign of *LARc.
         */
}

/* 4.2.9 */
/* Computation of the quantized reflection coefficients 
 */

/* 4.2.9.1  Interpolation of the LARpp[1..8] to get the LARp[1..8]
 */

/*
 *  Within each frame of 160 analyzed speech samples the short term
 *  analysis and synthesis filters operate with four different sets of
 *  coefficients, derived from the previous set of decoded LARs(LARpp(j-1))
 *  and the actual set of decoded LARs (LARpp(j))
 *
 * (Initial value: LARpp(j-1)[1..8] = 0.)
 */

static void Coefficients_0_12(
        register word * LARpp_j_1,
        register word * LARpp_j,
        register word * LARp)
{
        register int    i;
/*      register longword ltmp; */

        for (i = 1; i <= 8; i++, LARp++, LARpp_j_1++, LARpp_j++) {
                *LARp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j, 2 ));
                *LARp = GSM_ADD( *LARp,  SASR( *LARpp_j_1, 1));
        }
}

static void Coefficients_13_26(
        register word * LARpp_j_1,
        register word * LARpp_j,
        register word * LARp)
{
        register int i;
/*      register longword ltmp; */
        for (i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++) {
                *LARp = GSM_ADD( SASR( *LARpp_j_1, 1), SASR( *LARpp_j, 1 ));
        }
}

static void Coefficients_27_39(
        register word * LARpp_j_1,
        register word * LARpp_j,
        register word * LARp)
{
        register int i;
/*      register longword ltmp; */

        for (i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++) {
                *LARp = GSM_ADD( SASR( *LARpp_j_1, 2 ), SASR( *LARpp_j, 2 ));
                *LARp = GSM_ADD( *LARp, SASR( *LARpp_j, 1 ));
        }
}


static void Coefficients_40_159(
        register word * LARpp_j,
        register word * LARp)
{
        register int i;

        for (i = 1; i <= 8; i++, LARp++, LARpp_j++)
                *LARp = *LARpp_j;
}

/* 4.2.9.2 */

static void LARp_to_rp(
        register word * LARp)   /* [0..7] IN/OUT  */
/*
 *  The input of this procedure is the interpolated LARp[0..7] array.
 *  The reflection coefficients, rp[i], are used in the analysis
 *  filter and in the synthesis filter.
 */
{
        register int            i;
        register word           temp;
/*      register longword       ltmp; */

        for (i = 1; i <= 8; i++, LARp++) {

                /* temp = GSM_ABS( *LARp );
                                *
                 * if (temp < 11059) temp <<= 1;
                 * else if (temp < 20070) temp += 11059;
                 * else temp = GSM_ADD( temp >> 2, 26112 );
                 *
                 * *LARp = *LARp < 0 ? -temp : temp;
                 */

                if (*LARp < 0) {
                        temp = *LARp == MIN_WORD ? MAX_WORD : -(*LARp);
                        *LARp = - ((temp < 11059) ? temp << 1
                                : ((temp < 20070) ? temp + 11059
                                :  GSM_ADD( temp >> 2, 26112 )));
                } else {
                        temp  = *LARp;
                        *LARp =    (temp < 11059) ? temp << 1
                                : ((temp < 20070) ? temp + 11059
                                :  GSM_ADD( temp >> 2, 26112 ));
                }
        }
}


/* 4.2.10 */
#ifdef __MC68K__
/*
 *  This procedure computes the short term residual signal d[..] to be fed
 *  to the RPE-LTP loop from the s[..] signal and from the local rp[..]
 *  array (quantized reflection coefficients).  As the call of this
 *  procedure can be done in many ways (see the interpolation of the LAR
 *  coefficient), it is assumed that the computation begins with index
 *  k_start (for arrays d[..] and s[..]) and stops with index k_end
 *  (k_start and k_end are defined in 4.2.9.1).  This procedure also
 *  needs to keep the array u[0..7] in memory for each call.
 */
static asm void Short_term_analysis_filtering(
        word   *u,
        word   * rp,   /* [0..7]       IN      */
        int    k_n,    /*   k_end - k_start    */
        word   * s     /* [0..n-1]     IN/OUT  */
)
{
    fralloc+
    /*
     * 68K assembler version, with the internal loop unrolled and all the adds
     * and multiplies inlined. All variables are placed in registers. Note that the
     * multiplies in this routine do _not_ need to check for MIN_WORD * MIN_WORD.
     *
     * Registers used, in terms of the variables in the C version:
     *
     * a0: u
     * a1: rp
     * a2: s
     * a3: rpi
     * a4: uii
     *
     * d0: k_n
     * d1: di
     * d2: sav
     * d3: ui
     * d4: scratch register
     * d5: scratch register
     *
     */
    movem.l d3-d5/a2-a4,-(a7)
    movea.l u,a0
    movea.l rp,a1
    movea.l s,a2
    move.w k_n,d0
    moveq #15,d5            // d5 contains the immediate for the GSM_MULT_R right shifts
    
@loop:
    move.w (a2),d1          // di = sav = *s
    move.w d1,d2
    movea.l a1,a3           // rpi = rp
    movea.l a0,a4           // uii = u
    
// ST_ANAL_INSIDE: Iteration 1
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *uii++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd1
    bmi @pos1
    move.w #-32768,d2
    bra @endadd1
@pos1:
    move.w #32767, d2
@endadd1:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd2
    bmi @pos2
    move.w #-32768,d1
    bra @endadd2
@pos2:
    move.w #32767, d1
@endadd2:

// Iteration 2
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *uii++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd3
    bmi @pos3
    move.w #-32768,d2
    bra @endadd3
@pos3:
    move.w #32767, d2
@endadd3:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd4
    bmi @pos4
    move.w #-32768,d1
    bra @endadd4
@pos4:
    move.w #32767, d1
@endadd4:

// Iteration 3
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *ui++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd5
    bmi @pos5
    move.w #-32768,d2
    bra @endadd5
@pos5:
    move.w #32767, d2
@endadd5:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd6
    bmi @pos6
    move.w #-32768,d1
    bra @endadd6
@pos6:
    move.w #32767, d1
@endadd6:

// Iteration 4
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *ui++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd7
    bmi @pos7
    move.w #-32768,d2
    bra @endadd7
@pos7:
    move.w #32767, d2
@endadd7:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd8
    bmi @pos8
    move.w #-32768,d1
    bra @endadd8
@pos8:
    move.w #32767, d1
@endadd8:

// Iteration 5
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *ui++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd9
    bmi @pos9
    move.w #-32768,d2
    bra @endadd9
@pos9:
    move.w #32767, d2
@endadd9:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd10
    bmi @pos10
    move.w #-32768,d1
    bra @endadd10
@pos10:
    move.w #32767, d1
@endadd10:

// Iteration 6
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *ui++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd11
    bmi @pos1
    move.w #-32768,d2
    bra @endadd11
@pos11:
    move.w #32767, d2
@endadd11:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd12
    bmi @pos12
    move.w #-32768,d1
    bra @endadd12
@pos12:
    move.w #32767, d1
@endadd12:

// Iteration 7
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *ui++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd13
    bmi @pos13
    move.w #-32768,d2
    bra @endadd13
@pos13:
    move.w #32767, d2
@endadd13:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd14
    bmi @pos14
    move.w #-32768,d1
    bra @endadd14
@pos14:
    move.w #32767, d1
@endadd14:


// Iteration 8
    move.w (a4),d3          // ui = *uii
    move.w d2,(a4)+         // *ui++ = sav
    
    //GSM_MULT_R(*rpi, di)
    move.w (a3),d2
    muls.w d1,d2
    addi.l #16384,d2
    asr.l d5,d2             
    
    //sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di))
    add.w d3,d2
    bvc @endadd15
    bmi @pos3
    move.w #-32768,d2
    bra @endadd15
@pos15:
    move.w #32767, d2
@endadd15:

    //GSM_MULT_R(*rpi++, ui)
    move.w (a3)+,d4
    muls.w d3,d4
    addi.l #16384,d4
    asr.l d5,d4
    
    //di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui))
    add.w d4,d1
    bvc @endadd16
    bmi @pos16
    move.w #-32768,d1
    bra @endadd16
@pos16:
    move.w #32767, d1
@endadd16:

    // *s = di;
    move.w d1,(a2)+

    subq.w #1,d0
    bne @loop

    movem.l (a7)+,d3-d5/a2-a4
    frfree
    rts
}

#else	/*	__MC68K__	*/
static void Short_term_analysis_filtering(
        register word   *u,
        register word   * rp,   /* [0..7]       IN      */
        register int    k_n,    /*   k_end - k_start    */
        register word   * s     /* [0..n-1]     IN/OUT  */
)
/*
 *  This procedure computes the short term residual signal d[..] to be fed
 *  to the RPE-LTP loop from the s[..] signal and from the local rp[..]
 *  array (quantized reflection coefficients).  As the call of this
 *  procedure can be done in many ways (see the interpolation of the LAR
 *  coefficient), it is assumed that the computation begins with index
 *  k_start (for arrays d[..] and s[..]) and stops with index k_end
 *  (k_start and k_end are defined in 4.2.9.1).  This procedure also
 *  needs to keep the array u[0..7] in memory for each call.
 */
{
        word           di, sav, *rpi, *uii, ui;
        for (; k_n--; s++) {
          di = sav = *s;
          rpi = rp;
          uii = u;
#define ST_ANAL_INSIDE \
  ui = *uii; *uii++ = sav; \
  sav = GSM_ADD( ui, GSM_MULT_R(*rpi, di)); \
  di = GSM_ADD( di, GSM_MULT_R(*rpi++, ui));
          ST_ANAL_INSIDE       /* 0 */
          ST_ANAL_INSIDE       /* 1 */
          ST_ANAL_INSIDE       /* 2 */
          ST_ANAL_INSIDE       /* 3 */
          ST_ANAL_INSIDE       /* 4 */
          ST_ANAL_INSIDE       /* 5 */
          ST_ANAL_INSIDE       /* 6 */
          ST_ANAL_INSIDE       /* 7 */
          *s = di;
        }
}
#endif	/*	__MC68K__	*/


#ifdef __MC68K__
static asm void Short_term_synthesis_filtering(
        word   *v,
        word   * rrp,  /* [0..7]       IN      */
        int    k,      /* k_end - k_start      */
        word   * wt,   /* [0..k-1]     IN      */
        word   * srr    /* [0..k-1]     OUT     */
)
{
        fralloc+
        /*
         * 68K assembler version, with the internal loop unrolled and all the adds
         * and multiplies inlined. All variables are placed in registers. The 
         * multiplies in these routines need to check for MIN_WORD * MIN_WORD.
         *
         * Registers used, in terms of variables in the C version:
         *
         * a0: &v[i] -- decremented in unrolled loop
         * a1: &rrp[i] -- decremented in unrolled loop
         * a2: &v[i+1] -- decremented in unrolled loop
         * a3: wt
         * a4: srr (note: this is renamed from the C variable "sr")
         *
         * d0: k
         * d1: sri
         * d2: scratch register
         * d3: scratch register
         * d4: 15 -- number of bits to right shift in the GSM_MULT_R routine
         *
         * d5-d7 are used as storage for the values used to initialize a0-a3 at the
         * head of the unrolled loop:
         *
         * d5: v + 14: pointer to v[i] at the start of the unrolled loop
         * d6: v + 16: pointer to v[i+1] at the start of the unrolled loop
         * d7: rrp + 14: pointer to rrp[i] at the start of the unrolled loop
         *
         */
        movem.l d3-d7/a2-a4,-(a7)
        move.w k,d0
        movea.l wt,a3
        movea.l srr,a4
        moveq #15,d4            // d4 contains the size of the shift for GSM_MULT_R
        
        move.l v,d5                     // Put the starting loop addresses into spare data registers
        move.l d5,d6
        addi.l #14,d5
        addi.l #16,d6
        move.l rrp,d7
        addi.l #14,d7

@loop:
        move.w (a3)+,d1         // sri = *wt++
        
// Set up the address registers for the unrolled loop
        movea.l d5,a0           // a0 = &v[i]
        movea.l d7,a1           // a1 = &rrp[i]
        movea.l d6,a2           // a2 = &v[i+1]
        
// Now do the loop for i = 7 to 0
// i = 7
        //GSM_MULT_R( rrp[i], v[i] )
        move.w (a1),d2                          // d2,d3 are scratch registers
        move.w (a0),d3
        muls.w d2,d3                            // multiply
        addi.l #16384,d3                        // round
        asr.l d4,d3                                     // right shift 15
        cmpi.w #0x8000,d3                       // check for MIN_WORD * MIN_WORD
        bne @endmult71
        move.w #0x7FFF,d3                       // MIN_WORD * MIN_WORD = MAX_WORD
@endmult71:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1                                     // sri is in d1
        bvc @endsub7
        bmi @possub7
        move.w #-32768,d1                       // negative overflow
        bra @endsub7
@possub7:
        move.w #32767, d1                       // positive overflow
@endsub7:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                            // multiply (d2 still has rrp[i] in it)
        addi.l #16384,d2                        // round
        asr.l d4,d2                                     // right shift 15
        cmpi.w #0x8000,d2                       // check for MIN_WORD * MIN_WORD
        bne @endmult72
        move.w #0x7FFF,d2                       // MIN_WORD * MIN_WORD = MAX_WORD
@endmult72:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3                          // annoying to have to do this twice...
        add.w d2,d3                                     // add
        bvc @endadd7
        bmi @posadd7
        move.w #-32768,d3                       // negative overflow
        bra @endadd7
@posadd7:
        move.w #32767, d3                       // positive overflow
@endadd7:
        move.w d3,(a2)                          // move result to v[i+1]
        
// i = 6
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult61
        move.w #0x7FFF,d3
@endmult61:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub6
        bmi @possub6
        move.w #-32768,d1
        bra @endsub6
@possub6:
        move.w #32767, d1
@endsub6:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult62
        move.w #0x7FFF,d2
@endmult62:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd6
        bmi @posadd6
        move.w #-32768,d3
        bra @endadd6
@posadd6:
        move.w #32767, d3
@endadd6:
        move.w d3,-(a2)
        
// i = 5
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult51
        move.w #0x7FFF,d3
@endmult51:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub5
        bmi @possub5
        move.w #-32768,d1
        bra @endsub5
@possub5:
        move.w #32767, d1
@endsub5:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult52
        move.w #0x7FFF,d2
@endmult52:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd5
        bmi @posadd5
        move.w #-32768,d3
        bra @endadd5
@posadd5:
        move.w #32767, d3
@endadd5:
        move.w d3,-(a2)

// i = 4
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult41
        move.w #0x7FFF,d3
@endmult41:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub4
        bmi @possub4
        move.w #-32768,d1
        bra @endsub4
@possub4:
        move.w #32767, d1
@endsub4:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult42
        move.w #0x7FFF,d2
@endmult42:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd4
        bmi @posadd4
        move.w #-32768,d3
        bra @endadd4
@posadd4:
        move.w #32767, d3
@endadd4:
        move.w d3,-(a2)

// i = 3
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult31
        move.w #0x7FFF,d3
@endmult31:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub3
        bmi @possub3
        move.w #-32768,d1
        bra @endsub3
@possub3:
        move.w #32767, d1
@endsub3:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult32
        move.w #0x7FFF,d2
@endmult32:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd3
        bmi @posadd3
        move.w #-32768,d3
        bra @endadd3
@posadd3:
        move.w #32767, d3
@endadd3:
        move.w d3,-(a2)

// i = 2
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult21
        move.w #0x7FFF,d3
@endmult21:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub2
        bmi @possub2
        move.w #-32768,d1
        bra @endsub2
@possub2:
        move.w #32767, d1
@endsub2:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult22
        move.w #0x7FFF,d2
@endmult22:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd2
        bmi @posadd2
        move.w #-32768,d3
        bra @endadd2
@posadd2:
        move.w #32767, d3
@endadd2:
        move.w d3,-(a2)

// i = 1
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult11
        move.w #0x7FFF,d3
@endmult11:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub1
        bmi @possub1
        move.w #-32768,d1
        bra @endsub1
@possub1:
        move.w #32767, d1
@endsub1:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult12
        move.w #0x7FFF,d2
@endmult12:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd1
        bmi @posadd1
        move.w #-32768,d3
        bra @endadd1
@posadd1:
        move.w #32767, d3
@endadd1:
        move.w d3,-(a2)

// i = 0
        //GSM_MULT_R( rrp[i], v[i] )
        move.w -(a1),d2                         // d2,d3 are scratch registers
        move.w -(a0),d3
        muls.w d2,d3
        addi.l #16384,d3
        asr.l d4,d3
        cmpi.w #0x8000,d3
        bne @endmult01
        move.w #0x7FFF,d3
@endmult01:
        
        //sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) )
        sub.w d3,d1
        bvc @endsub0
        bmi @possub0
        move.w #-32768,d1
        bra @endsub0
@possub0:
        move.w #32767, d1
@endsub0:
        
        //GSM_MULT_R( rrp[i], sri )
        muls.w d1,d2                                    // d2 still has rrp[i] in it
        addi.l #16384,d2
        asr.l d4,d2
        cmpi.w #0x8000,d2
        bne @endmult02
        move.w #0x7FFF,d2
@endmult02:
        
        //v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) )
        move.w (a0),d3
        add.w d2,d3
        bvc @endadd0
        bmi @posadd0
        move.w #-32768,d3
        bra @endadd0
@posadd0:
        move.w #32767, d3
@endadd0:
        move.w d3,-(a2)

        
        //*srr++ = v[0] = sri
        move.w d1,(a0)
        move.w d1,(a4)+
        
        subq.w #1,d0
        bne @loop
        
        movem.l (a7)+,d3-d7/a2-a4
        frfree
        rts
}
#else	/*	__MC68K__	*/
static void Short_term_synthesis_filtering(
        word   *v,
        word   * rrp,  /* [0..7]       IN      */
        int    k,      /* k_end - k_start      */
        word   * wt,   /* [0..k-1]     IN      */
        word   * sr    /* [0..k-1]     OUT     */
)
{
  register int            i;
  register word           sri;
  while (k--) {
    sri = *wt++;
    for (i = 8; i--;) {
      if (rrp[i] == MIN_WORD) {
         sri  = GSM_SUB( sri, (v[i]==MIN_WORD) ? MAX_WORD :
                                                 GSM_MULT_R( rrp[i], v[i] ));
         v[i+1]=GSM_ADD( v[i], (sri == MIN_WORD) ? MAX_WORD :
                                                 GSM_MULT_R( rrp[i], sri ));
      }
#ifdef BUG_FIX
          else
          {
#endif /* BUG_FIX */
      sri = GSM_SUB( sri, GSM_MULT_R( rrp[i], v[i] ) );
      v[i+1] = GSM_ADD( v[i], GSM_MULT_R( rrp[i], sri ) );
#ifdef BUG_FIX
          }
#endif /* BUG_FIX */
    }
    *sr++ = v[0] = sri;
  }
}
#endif /* __MC68K__ */

static void Gsm_Short_Term_Analysis_Filter(

        struct gsm_state * S,

        word    * LARc,         /* coded log area ratio [0..7]  IN      */
        word    * s             /* signal [0..159]              IN/OUT  */
)
{
        word            * LARpp_j       = S->LARpp[ S->j      ];
        word            * LARpp_j_1     = S->LARpp[ S->j ^= 1 ];

        word            LARp[8];

#       define  FILTER  GSM_ST_ANAL_FILT

        Decoding_of_the_coded_Log_Area_Ratios( LARc, LARpp_j );

        Coefficients_0_12(  LARpp_j_1, LARpp_j, LARp );
        LARp_to_rp( LARp );
        FILTER( S->u, LARp, 13, s);

        Coefficients_13_26( LARpp_j_1, LARpp_j, LARp);
        LARp_to_rp( LARp );
        FILTER( S->u, LARp, 14, s + 13);

        Coefficients_27_39( LARpp_j_1, LARpp_j, LARp);
        LARp_to_rp( LARp );
        FILTER( S->u, LARp, 13, s + 27);

        Coefficients_40_159( LARpp_j, LARp);
        LARp_to_rp( LARp );
        FILTER( S->u, LARp, 120, s + 40);
}

static void Gsm_Short_Term_Synthesis_Filter(
        struct gsm_state * S,

        word    * LARcr,        /* received log area ratios [0..7] IN  */
        word    * wt,           /* received d [0..159]             IN  */

        word    * s             /* signal   s [0..159]            OUT  */
)
{
        word            * LARpp_j       = S->LARpp[ S->j     ];
        word            * LARpp_j_1     = S->LARpp[ S->j ^=1 ];

        word            LARp[8];

        Decoding_of_the_coded_Log_Area_Ratios( LARcr, LARpp_j );

        Coefficients_0_12( LARpp_j_1, LARpp_j, LARp );
        LARp_to_rp( LARp );
        GSM_ST_SYNTH_FILT( S->v, LARp, 13, wt, s );

        Coefficients_13_26( LARpp_j_1, LARpp_j, LARp);
        LARp_to_rp( LARp );
        GSM_ST_SYNTH_FILT( S->v, LARp, 14, wt + 13, s + 13 );

        Coefficients_27_39( LARpp_j_1, LARpp_j, LARp);
        LARp_to_rp( LARp );
        GSM_ST_SYNTH_FILT( S->v, LARp, 13, wt + 27, s + 27 );

        Coefficients_40_159( LARpp_j, LARp );
        LARp_to_rp( LARp );
        GSM_ST_SYNTH_FILT( S->v, LARp, 120, wt + 40, s + 40);
}

/*
 *  4.2.11 .. 4.2.12 LONG TERM PREDICTOR (LTP) SECTION
 */

/*
 * This module computes the LTP gain (bc) and the LTP lag (Nc)
 * for the long term analysis filter.   This is done by calculating a
 * maximum of the cross-correlation function between the current
 * sub-segment short term residual signal d[0..39] (output of
 * the short term analysis filter; for simplification the index
 * of this array begins at 0 and ends at 39 for each sub-segment of the
 * RPE-LTP analysis) and the previous reconstructed short term
 * residual signal dp[ -120 .. -1 ].  A dynamic scaling must be
 * performed to avoid overflow.
 */

 /* The next procedure exists in six versions.  First two integer
  * version (if USE_FLOAT_MUL is not defined); then four floating
  * point versions, twice with proper scaling (USE_FLOAT_MUL defined),
  * once without (USE_FLOAT_MUL and FAST defined, and fast run-time
  * option used).  Every pair has first a Cut version (see the -C
  * option to toast or the LTP_CUT option to gsm_option()), then the
  * uncut one.  (For a detailed explanation of why this is altogether
  * a bad idea, see Henry Spencer and Geoff Collyer, ``#ifdef Considered
  * Harmful''.)
  */

#ifdef  LTP_CUT

static void Cut_Calculation_of_the_LTP_parameters(
        register word   * d,            /* [0..39]      IN      */
        register word   * dp,           /* [-120..-1]   IN      */
        word            * bc_out,       /*              OUT     */
        word            * Nc_out        /*              OUT     */
)
{
        register int    k, lambda;
        word            Nc, bc;

        longword        L_result;
        longword        L_max, L_power;
        word            R, S, dmax, scal, best_k;

        register word   temp, wt_k;

        /*  Search of the optimum scaling of d[0..39].
         */
        dmax = 0;
        for (k = 0; k <= 39; k++) {
                temp = d[k];
                temp = GSM_ABS( temp );
                if (temp > dmax) {
                        dmax = temp;
                        best_k = k;
                }
        }
        temp = 0;
        if (dmax == 0) scal = 0;
        else {
                pgpAssert(dmax > 0);
                temp = gsm_norm( (longword)dmax << 16 );
        }
        if (temp > 6) scal = 0;
        else scal = 6 - temp;
        pgpAssert(scal >= 0);

        /* Search for the maximum cross-correlation and coding of the LTP lag
         */
        L_max = 0;
        Nc    = 40;     /* index for the maximum cross-correlation */
        wt_k  = SASR(d[best_k], scal);

        for (lambda = 40; lambda <= 120; lambda++) {
                L_result = (longword)wt_k * dp[best_k - lambda];
                if (L_result > L_max) {
                        Nc    = lambda;
                        L_max = L_result;
                }
        }
        *Nc_out = Nc;
        L_max <<= 1;

        /*  Rescaling of L_max
         */
        pgpAssert(scal <= 100 && scal >= -100);
        L_max = L_max >> (6 - scal);    /* sub(6, scal) */

        pgpAssert( Nc <= 120 && Nc >= 40);
        /*   Compute the power of the reconstructed short term residual
         *   signal dp[..]
         */
        L_power = 0;
        for (k = 0; k <= 39; k++) {

                register longword L_temp;

                L_temp   = SASR( dp[k - Nc], 3 );
                L_power += L_temp * L_temp;
        }
        L_power <<= 1;  /* from L_MULT */

        /*  Normalization of L_max and L_power
         */

        if (L_max <= 0)  {
                *bc_out = 0;
                return;
        }
        if (L_max >= L_power) {
                *bc_out = 3;
                return;
        }

        temp = gsm_norm( L_power );

        R = SASR( L_max   << temp, 16 );
        S = SASR( L_power << temp, 16 );

        /*  Coding of the LTP gain
         */

        /*  Table 4.3a must be used to obtain the level DLB[i] for the
         *  quantization of the LTP gain b to get the coded version bc.
         */
        for (bc = 0; bc <= 2; bc++) if (R <= GSM_MULT(S, gsm_DLB[bc])) break;
        *bc_out = bc;
}

#endif  /* LTP_CUT */

#ifdef __MC68K__
static asm longword LTP_loop_internal(word *wt,word *dp)
{
    fralloc+
    /*
     * Registers used:
     *
     * a0: wt
     * a1: dp
     * d0: L_result
     * d1: scratch register
     *
     */
    movea.l wt,a0
    movea.l dp,a1
    moveq.l #0,d0
    
    
    // Unrolled loop: do it forty times!
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    
    move.w (a0)+,d1
    muls.w (a1)+,d1
    add.l d1,d0
    frfree
    rts
}
#else /* __MC68K__ */
static longword LTP_loop_internal(word *wt,word *dp) {
  word i;
  longword L_result = 0;
  for (i=0; i<40; i++) {
    L_result += (longword) *wt++ * *dp++;
  }
  return L_result;
}
#endif /* __MC68K__ */

static void Calculation_of_the_LTP_parameters(
        register word   * d,            /* [0..39]      IN      */
        register word   * dp,           /* [-120..-1]   IN      */
        word            * bc_out,       /*              OUT     */
        word            * Nc_out        /*              OUT     */
)
{
        register int    k, lambda;
        word            Nc, bc;
        word            wt[40];

        longword        L_max, L_power;
        word            R, S, dmax, scal;
        register word   temp;

        /*  Search of the optimum scaling of d[0..39].
         */
        dmax = 0;

        for (k = 0; k <= 39; k++) {
                temp = d[k];
                temp = GSM_ABS( temp );
                if (temp > dmax) dmax = temp;
        }

        temp = 0;
        if (dmax == 0) scal = 0;
        else {
                pgpAssert(dmax > 0);
                temp = gsm_norm( (longword)dmax << 16 );
        }

        if (temp > 6) scal = 0;
        else scal = 6 - temp;

        pgpAssert(scal >= 0);

        /*  Initialization of a working array wt
         */

        for (k = 0; k <= 39; k++) wt[k] = SASR( d[k], scal );

        /* Search for the maximum cross-correlation and coding of the LTP lag
         */
        L_max = 0;
        Nc    = 40;     /* index for the maximum cross-correlation */

        for (lambda = 40; lambda <= 120; lambda++) {
          longword L_result;
          L_result = GSM_LTP_LOOP( wt, dp - lambda );
          if (L_result > L_max) {
            Nc    = lambda;
            L_max = L_result;
          }
        }

        *Nc_out = Nc;
        L_max <<= 1;

        /*  Rescaling of L_max
         */
        pgpAssert(scal <= 100 && scal >=  -100);
        L_max = L_max >> (6 - scal);    /* sub(6, scal) */

        pgpAssert( Nc <= 120 && Nc >= 40);

        /*   Compute the power of the reconstructed short term residual
         *   signal dp[..]
         */
        L_power = 0;
        for (k = 0; k <= 39; k++) {

                register longword L_temp;

                L_temp   = SASR( dp[k - Nc], 3 );
                L_power += L_temp * L_temp;
        }
        L_power <<= 1;  /* from L_MULT */

        /*  Normalization of L_max and L_power
         */

        if (L_max <= 0)  {
                *bc_out = 0;
                return;
        }
        if (L_max >= L_power) {
                *bc_out = 3;
                return;
        }

        temp = gsm_norm( L_power );

        R = SASR( L_max   << temp, 16 );
        S = SASR( L_power << temp, 16 );

        /*  Coding of the LTP gain
         */

        /*  Table 4.3a must be used to obtain the level DLB[i] for the
         *  quantization of the LTP gain b to get the coded version bc.
         */
        for (bc = 0; bc <= 2; bc++) if (R <= GSM_MULT(S, gsm_DLB[bc])) break;
        *bc_out = bc;
}

/* 4.2.12 */

static void Long_term_analysis_filtering(
        word            bc,     /*                                      IN  */
        word            Nc,     /*                                      IN  */
        register word   * dp,   /* previous d   [-120..-1]              IN  */
        register word   * d,    /* d            [0..39]                 IN  */
        register word   * dpp,  /* estimate     [0..39]                 OUT */
        register word   * e     /* long term res. signal [0..39]        OUT */
)
/*
 *  In this part, we have to decode the bc parameter to compute
 *  the samples of the estimate dpp[0..39].  The decoding of bc needs the
 *  use of table 4.3b.  The long term residual signal e[0..39]
 *  is then calculated to be fed to the RPE encoding section.
 */
{
        register int      k;

#       undef STEP
#       define STEP(BP)                                 \
        for (k = 0; k <= 39; k++) {                     \
                dpp[k]  = GSM_MULT_R( BP, dp[k - Nc]);  \
                e[k]    = GSM_SUB( d[k], dpp[k] );      \
        }

        switch (bc) {
        case 0: STEP(  3277 ); break;
        case 1: STEP( 11469 ); break;
        case 2: STEP( 21299 ); break;
        case 3: STEP( 32767 ); break;
        }
}

static void Gsm_Long_Term_Predictor(

        struct gsm_state        * S,

        word    * d,    /* [0..39]   residual signal    IN      */
        word    * dp,   /* [-120..-1] d'                IN      */

        word    * e,    /* [0..39]                      OUT     */
        word    * dpp,  /* [0..39]                      OUT     */
        word    * Nc,   /* correlation lag              OUT     */
        word    * bc    /* gain factor                  OUT     */
)
{
        pgpAssert( d  ); pgpAssert( dp ); pgpAssert( e  );
        pgpAssert( dpp); pgpAssert( Nc ); pgpAssert( bc );

/* Original version, calling Cut_Calculation...
#ifdef LTP_CUT
                if (S->ltp_cut)
                        Cut_Calculation_of_the_LTP_parameters(d, dp, bc, Nc);
                else
#endif
                        Calculation_of_the_LTP_parameters(d, dp, bc, Nc);
*/

#ifdef LTP_CUT
                if (S->ltp_cut)
                        *bc = *Nc = 0;
                else
#endif
                        Calculation_of_the_LTP_parameters(d, dp, bc, Nc);

        Long_term_analysis_filtering( *bc, *Nc, dp, d, dpp, e );
}

/* 4.3.2 */
static void Gsm_Long_Term_Synthesis_Filtering(
        struct gsm_state        * S,

        word                    Ncr,
        word                    bcr,
        register word           * erp,          /* [0..39]                IN */
        register word           * drp           /* [-120..-1] IN, [0..40] OUT */
)
/*
 *  This procedure uses the bcr and Ncr parameter to realize the
 *  long term synthesis filtering.  The decoding of bcr needs
 *  table 4.3b.
 */
{
        register int            k;
        word                    brp, drpp, Nr;

        /*  Check the limits of Nr.
         */
        Nr = Ncr < 40 || Ncr > 120 ? S->nrp : Ncr;
        S->nrp = Nr;
        pgpAssert(Nr >= 40 && Nr <= 120);

        /*  Decoding of the LTP gain bcr
         */
        brp = gsm_QLB[ bcr ];

        /*  Computation of the reconstructed short term residual
         *  signal drp[0..39]
         */
        pgpAssert(brp != MIN_WORD);

        for (k = 0; k <= 39; k++) {
                drpp   = GSM_MULT_R( brp, drp[ k - Nr ] );
                drp[k] = GSM_ADD( erp[k], drpp );
        }

        /*
         *  Update of the reconstructed short term residual signal
         *  drp[ -1..-120 ]
         */

        for (k = 0; k <= 119; k++) drp[ -120 + k ] = drp[ -80 + k ];
}

/*  4.2.13 .. 4.2.17  RPE ENCODING SECTION
 */

/* 4.2.13 */

static void Weighting_filter(
        register word   * e,            /* signal [-5..0.39.44] IN  */
        word            * x             /* signal [0..39]       OUT */
)
/*
 *  The coefficients of the weighting filter are stored in a table
 *  (see table 4.4).  The following scaling is used:
 *
 *      H[0..10] = integer( real_H[ 0..10] * 8192 );
 */
{
        /* word                 wt[ 50 ]; */

        register longword       L_result;
        register int            k /* , i */ ;

        /*  Initialization of a temporary working array wt[0...49]
         */

        /* for (k =  0; k <=  4; k++) wt[k] = 0;
         * for (k =  5; k <= 44; k++) wt[k] = *e++;
         * for (k = 45; k <= 49; k++) wt[k] = 0;
         *
         *  (e[-5..-1] and e[40..44] are allocated by the caller,
         *  are initially zero and are not written anywhere.)
         */
        e -= 5;

        /*  Compute the signal x[0..39]
         */
        for (k = 0; k <= 39; k++) {

                L_result = 8192 >> 1;

                /* for (i = 0; i <= 10; i++) {
                 *      L_temp   = GSM_L_MULT( wt[k+i], gsm_H[i] );
                 *      L_result = GSM_L_ADD( L_result, L_temp );
                 * }
                 */

#undef  STEP
#define STEP( i, H )    (e[ k + i ] * (longword)H)

                /*  Every one of these multiplications is done twice --
                 *  but I don't see an elegant way to optimize this. 
                 *  Do you?
                 */

#ifdef  STUPID_COMPILER
                L_result += STEP(       0,      -134 ) ;
                L_result += STEP(       1,      -374 )  ;
                       /* + STEP(       2,      0    )  */
                L_result += STEP(       3,      2054 ) ;
                L_result += STEP(       4,      5741 ) ;
                L_result += STEP(       5,      8192 ) ;
                L_result += STEP(       6,      5741 ) ;
                L_result += STEP(       7,      2054 ) ;
                       /* + STEP(       8,      0    )  */
                L_result += STEP(       9,      -374 ) ;
                L_result += STEP(       10,     -134 ) ;
#else
                L_result +=
                  STEP( 0,      -134 ) 
                + STEP( 1,      -374 ) 
             /* + STEP( 2,      0    )  */
                + STEP( 3,      2054 ) 
                + STEP( 4,      5741 ) 
                + STEP( 5,      8192 )
                + STEP( 6,      5741 )
                + STEP( 7,      2054 )
             /* + STEP( 8,      0    )  */
                + STEP( 9,      -374 ) 
                + STEP(10,      -134 )
                ;
#endif

                /* L_result = GSM_L_ADD( L_result, L_result ); (* scaling(x2) *)
                 * L_result = GSM_L_ADD( L_result, L_result ); (* scaling(x4) *)
                 *
                 * x[k] = SASR( L_result, 16 );
                 */

                /* 2 adds vs. >>16 => 14, minus one shift to compensate for
                 * those we lost when replacing L_MULT by '*'.
                 */

                L_result = SASR( L_result, 13 );
                x[k] =  (  L_result < MIN_WORD ? MIN_WORD
                        : (L_result > MAX_WORD ? MAX_WORD : L_result ));
        }
}

/* 4.2.14 */

static void RPE_grid_selection(
        word            * x,            /* [0..39]              IN  */ 
        word            * xM,           /* [0..12]              OUT */
        word            * Mc_out        /*                      OUT */
)
/*
 *  The signal x[0..39] is used to select the RPE grid which is
 *  represented by Mc.
 */
{
        /* register word        temp1;  */
        register int            /* m, */  i;
        register longword       L_result, L_temp;
        longword                EM;     /* xxx should be L_EM? */
        word                    Mc;

        longword                L_common_0_3;

        EM = 0;
        Mc = 0;

        /* for (m = 0; m <= 3; m++) {
         *      L_result = 0;
         *
         *
         *      for (i = 0; i <= 12; i++) {
         *
         *              temp1    = SASR( x[m + 3*i], 2 );
         *
         *              pgpAssert(temp1 != MIN_WORD);
         *
         *              L_temp   = GSM_L_MULT( temp1, temp1 );
         *              L_result = GSM_L_ADD( L_temp, L_result );
         *      }
         *
         *      if (L_result > EM) {
         *              Mc = m;
         *              EM = L_result;
         *      }
         * }
         */

#undef  STEP
#define STEP( m, i )            L_temp = SASR( x[m + 3 * i], 2 );       \
                                L_result += L_temp * L_temp;

        /* common part of 0 and 3 */

        L_result = 0;
        STEP( 0, 1 ); STEP( 0, 2 ); STEP( 0, 3 ); STEP( 0, 4 );
        STEP( 0, 5 ); STEP( 0, 6 ); STEP( 0, 7 ); STEP( 0, 8 );
        STEP( 0, 9 ); STEP( 0, 10); STEP( 0, 11); STEP( 0, 12);
        L_common_0_3 = L_result;

        /* i = 0 */

        STEP( 0, 0 );
        L_result <<= 1; /* implicit in L_MULT */
        EM = L_result;

        /* i = 1 */

        L_result = 0;
        STEP( 1, 0 );
        STEP( 1, 1 ); STEP( 1, 2 ); STEP( 1, 3 ); STEP( 1, 4 );
        STEP( 1, 5 ); STEP( 1, 6 ); STEP( 1, 7 ); STEP( 1, 8 );
        STEP( 1, 9 ); STEP( 1, 10); STEP( 1, 11); STEP( 1, 12);
        L_result <<= 1;
        if (L_result > EM) {
                Mc = 1;
                EM = L_result;
        }

        /* i = 2 */

        L_result = 0;
        STEP( 2, 0 );
        STEP( 2, 1 ); STEP( 2, 2 ); STEP( 2, 3 ); STEP( 2, 4 );
        STEP( 2, 5 ); STEP( 2, 6 ); STEP( 2, 7 ); STEP( 2, 8 );
        STEP( 2, 9 ); STEP( 2, 10); STEP( 2, 11); STEP( 2, 12);
        L_result <<= 1;
        if (L_result > EM) {
                Mc = 2;
                EM = L_result;
        }

        /* i = 3 */

        L_result = L_common_0_3;
        STEP( 3, 12 );
        L_result <<= 1;
        if (L_result > EM) {
                Mc = 3;
                EM = L_result;
        }

        /**/

        /*  Down-sampling by a factor 3 to get the selected xM[0..12]
         *  RPE sequence.
         */
        for (i = 0; i <= 12; i ++) xM[i] = x[Mc + 3*i];
        *Mc_out = Mc;
}

/* 4.12.15 */

static void APCM_quantization_xmaxc_to_exp_mant(
        word            xmaxc,          /* IN   */
        word            * exp_out,      /* OUT  */
        word            * mant_out )    /* OUT  */
{
        word    exp, mant;

        /* Compute exponent and mantissa of the decoded version of xmaxc
         */

        exp = 0;
        if (xmaxc > 15) exp = SASR(xmaxc, 3) - 1;
        mant = xmaxc - (exp << 3);

        if (mant == 0) {
                exp  = -4;
                mant = 7;
        }
        else {
                while (mant <= 7) {
                        mant = (mant << 1) | 1;
                        exp--;
                }
                mant -= 8;
        }

        pgpAssert( exp  >= -4 && exp <= 6 );
        pgpAssert( mant >= 0 && mant <= 7 );

        *exp_out  = exp;
        *mant_out = mant;
}

static void APCM_quantization(
        word            * xM,           /* [0..12]              IN      */

        word            * xMc,          /* [0..12]              OUT     */
        word            * mant_out,     /*                      OUT     */
        word            * exp_out,      /*                      OUT     */
        word            * xmaxc_out     /*                      OUT     */
)
{
        int     i, itest;

        word    xmax, xmaxc, temp, temp1, temp2;
        word    exp, mant;


        /*  Find the maximum absolute value xmax of xM[0..12].
         */

        xmax = 0;
        for (i = 0; i <= 12; i++) {
                temp = xM[i];
                temp = GSM_ABS(temp);
                if (temp > xmax) xmax = temp;
        }

        /*  Qantizing and coding of xmax to get xmaxc.
         */

        exp   = 0;
        temp  = SASR( xmax, 9 );
        itest = 0;

        for (i = 0; i <= 5; i++) {

                itest |= (temp <= 0);
                temp = SASR( temp, 1 );

                pgpAssert(exp <= 5);
                if (itest == 0) exp++;          /* exp = add (exp, 1) */
        }

        pgpAssert(exp <= 6 && exp >= 0);
        temp = exp + 5;

        pgpAssert(temp <= 11 && temp >= 0);
        xmaxc = GSM_ADD( SASR(xmax, temp), exp << 3 );

        /*   Quantizing and coding of the xM[0..12] RPE sequence
         *   to get the xMc[0..12]
         */

        APCM_quantization_xmaxc_to_exp_mant( xmaxc, &exp, &mant );

        /*  This computation uses the fact that the decoded version of xmaxc
         *  can be calculated by using the exponent and the mantissa part of
         *  xmaxc (logarithmic table).
         *  So, this method avoids any division and uses only a scaling
         *  of the RPE samples by a function of the exponent.  A direct 
         *  multiplication by the inverse of the mantissa (NRFAC[0..7]
         *  found in table 4.5) gives the 3 bit coded version xMc[0..12]
         *  of the RPE samples.
         */


        /* Direct computation of xMc[0..12] using table 4.5
         */

        pgpAssert( exp <= 4096 && exp >= -4096);
        pgpAssert( mant >= 0 && mant <= 7 ); 

        temp1 = 6 - exp;                /* normalization by the exponent */
        temp2 = gsm_NRFAC[ mant ];      /* inverse mantissa              */

        for (i = 0; i <= 12; i++) {

                pgpAssert(temp1 >= 0 && temp1 < 16);

                temp = xM[i] << temp1;
                temp = GSM_MULT( temp, temp2 );
                temp = SASR(temp, 12);
                xMc[i] = temp + 4;              /* see note below */
        }

        /*  NOTE: This equation is used to make all the xMc[i] positive.
         */

        *mant_out  = mant;
        *exp_out   = exp;
        *xmaxc_out = xmaxc;
}

/* 4.2.16 */

static void APCM_inverse_quantization(
        register word   * xMc,  /* [0..12]                      IN      */
        word            mant,
        word            exp,
        register word   * xMp)  /* [0..12]                      OUT     */
/* 
 *  This part is for decoding the RPE sequence of coded xMc[0..12]
 *  samples to obtain the xMp[0..12] array.  Table 4.6 is used to get
 *  the mantissa of xmaxc (FAC[0..7]).
 */
{
        int     i;
        word    temp, temp1, temp2, temp3;
/*      longword        ltmp; */

        pgpAssert( mant >= 0 && mant <= 7 );

        temp1 = gsm_FAC[ mant ];        /* see 4.2-15 for mant */
        temp2 = GSM_SUB( 6, exp );      /* see 4.2-15 for exp  */
        temp3 = gsm_asl( 1, GSM_SUB( temp2, 1 ));

        for (i = 13; i--;) {

//printf("%x ",*xMc);
//fflush(stdout);
                pgpAssert( *xMc <= 7 && *xMc >= 0 );       /* 3 bit unsigned */

                /* temp = GSM_SUB( *xMc++ << 1, 7 ); */
                temp = (*xMc++ << 1) - 7;               /* restore sign   */
                pgpAssert( temp <= 7 && temp >= -7 );      /* 4 bit signed   */

                temp <<= 12;                            /* 16 bit signed  */
                temp = GSM_MULT_R( temp1, temp );
                temp = GSM_ADD( temp, temp3 );
                *xMp++ = gsm_asr( temp, temp2 );
        }
}

/* 4.2.17 */

static void RPE_grid_positioning(
        word            Mc,             /* grid position        IN      */
        register word   * xMp,          /* [0..12]              IN      */
        register word   * ep            /* [0..39]              OUT     */
)
/*
 *  This procedure computes the reconstructed long term residual signal
 *  ep[0..39] for the LTP analysis filter.  The inputs are the Mc
 *  which is the grid position selection and the xMp[0..12] decoded
 *  RPE samples which are upsampled by a factor of 3 by inserting zero
 *  values.
 */
{
        int     i = 13;

        pgpAssert(0 <= Mc && Mc <= 3);

        switch (Mc) {
                case 3: *ep++ = 0;
                case 2:  do {
                                *ep++ = 0;
                case 1:         *ep++ = 0;
                case 0:         *ep++ = *xMp++;
                         } while (--i);
        }
        while (++Mc < 4) *ep++ = 0;

        /*

        int i, k;
        for (k = 0; k <= 39; k++) ep[k] = 0;
        for (i = 0; i <= 12; i++) {
                ep[ Mc + (3*i) ] = xMp[i];
        }
        */
}

/* 4.2.18 */

/*  This procedure adds the reconstructed long term residual signal
 *  ep[0..39] to the estimated signal dpp[0..39] from the long term
 *  analysis filter to compute the reconstructed short term residual
 *  signal dp[-40..-1]; also the reconstructed short term residual
 *  array dp[-120..-41] is updated.
 */

#if 0   /* Has been inlined in code.c */
static void Gsm_Update_of_reconstructed_short_time_residual_signal(
        word    * dpp,          /* [0...39]     IN      */
        word    * ep,           /* [0...39]     IN      */
        word    * dp)           /* [-120...-1]  IN/OUT  */
{
        int             k;

        for (k = 0; k <= 79; k++)
                dp[ -120 + k ] = dp[ -80 + k ];

        for (k = 0; k <= 39; k++)
                dp[ -40 + k ] = GSM_ADD( ep[k], dpp[k] );
}
#endif  /* Has been inlined in code.c */

static void Gsm_RPE_Encoding(
        word    * e,            /* -5..-1][0..39][40..44        IN/OUT  */
        word    * xmaxc,        /*                              OUT */
        word    * Mc,           /*                              OUT */
        word    * xMc)          /* [0..12]                      OUT */
{
        word    x[40];
        word    xM[13], xMp[13];
        word    mant, exp;

        Weighting_filter(e, x);
        RPE_grid_selection(x, xM, Mc);

        APCM_quantization(      xM, xMc, &mant, &exp, xmaxc);
        APCM_inverse_quantization(  xMc,  mant,  exp, xMp);

        RPE_grid_positioning( *Mc, xMp, e );

}

static void Gsm_RPE_Decoding(
        word            xmaxcr,
        word            Mcr,
        word            * xMcr,  /* [0..12], 3 bits             IN      */
        word            * erp    /* [0..39]                     OUT     */
)
{
        word    exp, mant;
        word    xMp[ 13 ];

        APCM_quantization_xmaxc_to_exp_mant( xmaxcr, &exp, &mant );
        APCM_inverse_quantization( xMcr, mant, exp, xMp );
        RPE_grid_positioning( Mcr, xMp, erp );

}


/* public functions */

gsm gsm_create(void) {
  gsm  r;
  r = (gsm)malloc(sizeof(struct gsm_state));
  if (!r) return r;

  memset((char *)r, 0, sizeof(*r));
  r->nrp = 40;

  return r;
}

void gsm_destroy(gsm S)
{
        if (S) free((char *)S);
}

int gsm_decode(gsm s, gsm_byte * c, gsm_signal * target)
{
        word    LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13*4];

        /* GSM_MAGIC  = (*c >> 4) & 0xF; */

        if (((*c >> 4) & 0x0F) != GSM_MAGIC) return -1;

        LARc[0]  = (*c++ & 0xF) << 2;           /* 1 */
        LARc[0] |= (*c >> 6) & 0x3;
        LARc[1]  = *c++ & 0x3F;
        LARc[2]  = (*c >> 3) & 0x1F;
        LARc[3]  = (*c++ & 0x7) << 2;
        LARc[3] |= (*c >> 6) & 0x3;
        LARc[4]  = (*c >> 2) & 0xF;
        LARc[5]  = (*c++ & 0x3) << 2;
        LARc[5] |= (*c >> 6) & 0x3;
        LARc[6]  = (*c >> 3) & 0x7;
        LARc[7]  = *c++ & 0x7;
        
#ifdef LTP_CUT
        if (s->ltp_cut)
        {
                Nc[0]= Nc[1] = Nc[2] = Nc[3] = 0;
                bc[0] = bc[1] = bc[2] = bc[3] = 0;
                
                Mc[0] = (*c >> 6) & 0x3;
                xmaxc[0] = *c++ & 0x3F;
                xmc[0] = (*c >> 5) & 0x7;
                xmc[1] = (*c >> 2) & 0x7;
                xmc[2] = (*c++ & 0x3) << 1;
                xmc[2] |= (*c >> 7) & 0x1;
                xmc[3] = (*c >> 4) & 0x7;
                xmc[4] = (*c >> 1) & 0x7;
                xmc[5] = (*c++ & 0x1) << 2;
                xmc[5] |= (*c >> 6) & 0x3;
                xmc[6] = (*c >> 3) & 0x7;
                xmc[7] = *c++ & 0x7;
                xmc[8] = (*c >> 5) & 0x7;
                xmc[9] = (*c >> 2) & 0x7;
                xmc[10] = (*c++ & 0x3) << 1;
                xmc[10] |= (*c >> 7) & 0x1;
                xmc[11] = (*c >> 4) & 0x7;
				xmc[12]  = *c & 0x7;			/* ##### bugfix WFP3 */
                Mc[1] = (*c++ & 0x1) << 1;
                Mc[1] |= (*c >> 7) & 0x1;
                xmaxc[1] = (*c >> 1) & 0x3F;
                xmc[13] = (*c++ & 0x1) << 2;
                xmc[13] |= (*c >> 6) & 0x3;
                xmc[14] = (*c >> 3) & 0x7;
                xmc[15] = (*c++ & 0x7);
                xmc[16] = (*c >> 5) & 0x7;
                xmc[17] = (*c >> 2) & 0x7;
                xmc[18] = (*c++ & 0x3) << 1;
                xmc[18] |= (*c >> 7) & 0x1;
                xmc[19] = (*c >> 4) & 0x7;
                xmc[20] = (*c >> 1) & 0x7;
                xmc[21] = (*c++ & 0x1) << 2;
                xmc[21] |= (*c >> 6) & 0x3;
                xmc[22] = (*c >> 3) & 0x7;
                xmc[23] = (*c++ & 0x7);
                xmc[24] = (*c >> 5) & 0x7;
                xmc[25] = (*c >> 2) & 0x7;
                Mc[2] = (*c++ & 0x3);
                xmaxc[2] = (*c >> 2) & 0x3F;
                xmc[26] = (*c++ & 0x3) << 1;
                xmc[26] |= (*c >> 7) & 0x1;
                xmc[27] = (*c >> 4) & 0x7;
                xmc[28] = (*c >> 1) & 0x7;
                xmc[29] = (*c++ & 0x1) << 2;
                xmc[29] |= (*c >> 6) & 0x3;
                xmc[30] = (*c >> 3) & 0x7;
                xmc[31] = (*c++ & 0x7);
                xmc[32] = (*c >> 5) & 0x7;
                xmc[33] = (*c >> 2) & 0x7;
                xmc[34] = (*c++ & 0x3) << 1;
                xmc[34] |= (*c >> 7) & 0x1;
                xmc[35] = (*c >> 4) & 0x7;
                xmc[36] = (*c >> 1) & 0x7;
                xmc[37] = (*c++ & 0x1) << 2;
                xmc[37] |= (*c >> 6) & 0x3;
                xmc[38] = (*c >> 3) & 0x7;
                Mc[3] = (*c >> 1) & 0x3;
                xmaxc[3] = (*c++ & 0x1) << 5;
                xmaxc[3] |= (*c >> 3) & 0x1F;
                xmc[39] = (*c++ & 0x7);
                xmc[40] = (*c >> 5) & 0x7;
                xmc[41] = (*c >> 2) & 0x7;
                xmc[42] = (*c++ & 0x3) << 1;
                xmc[42] |= (*c >> 7) & 0x1;
                xmc[43] = (*c >> 4) & 0x7;
                xmc[44] = (*c >> 1) & 0x7;
                xmc[45] = (*c++ & 0x1) << 2;
                xmc[45] |= (*c >> 6) & 0x3;
                xmc[46] = (*c >> 3) & 0x7;
                xmc[47] = (*c++ & 0x7);
                xmc[48] = (*c >> 5) & 0x7;
                xmc[49] = (*c >> 2) & 0x7;
                xmc[50] = (*c++ & 0x3) << 1;
                xmc[50] |= (*c >> 7) & 0x1;
                xmc[51] = (*c >> 4) & 0x7;
        }
        else
        {
#endif /* LTP_CUT */

                Nc[0]  = (*c >> 1) & 0x7F;
                bc[0]  = (*c++ & 0x1) << 1;
                bc[0] |= (*c >> 7) & 0x1;
                Mc[0]  = (*c >> 5) & 0x3;
                xmaxc[0]  = (*c++ & 0x1F) << 1;
                xmaxc[0] |= (*c >> 7) & 0x1;
                xmc[0]  = (*c >> 4) & 0x7;
                xmc[1]  = (*c >> 1) & 0x7;
                xmc[2]  = (*c++ & 0x1) << 2;
                xmc[2] |= (*c >> 6) & 0x3;
                xmc[3]  = (*c >> 3) & 0x7;
                xmc[4]  = *c++ & 0x7;
                xmc[5]  = (*c >> 5) & 0x7;
                xmc[6]  = (*c >> 2) & 0x7;
                xmc[7]  = (*c++ & 0x3) << 1;            /* 10 */
                xmc[7] |= (*c >> 7) & 0x1;
                xmc[8]  = (*c >> 4) & 0x7;
                xmc[9]  = (*c >> 1) & 0x7;
                xmc[10]  = (*c++ & 0x1) << 2;
                xmc[10] |= (*c >> 6) & 0x3;
                xmc[11]  = (*c >> 3) & 0x7;
                xmc[12]  = *c++ & 0x7;
                Nc[1]  = (*c >> 1) & 0x7F;
                bc[1]  = (*c++ & 0x1) << 1;
                bc[1] |= (*c >> 7) & 0x1;
                Mc[1]  = (*c >> 5) & 0x3;
                xmaxc[1]  = (*c++ & 0x1F) << 1;
                xmaxc[1] |= (*c >> 7) & 0x1;
                xmc[13]  = (*c >> 4) & 0x7;
                xmc[14]  = (*c >> 1) & 0x7;
                xmc[15]  = (*c++ & 0x1) << 2;
                xmc[15] |= (*c >> 6) & 0x3;
                xmc[16]  = (*c >> 3) & 0x7;
                xmc[17]  = *c++ & 0x7;
                xmc[18]  = (*c >> 5) & 0x7;
                xmc[19]  = (*c >> 2) & 0x7;
                xmc[20]  = (*c++ & 0x3) << 1;
                xmc[20] |= (*c >> 7) & 0x1;
                xmc[21]  = (*c >> 4) & 0x7;
                xmc[22]  = (*c >> 1) & 0x7;
                xmc[23]  = (*c++ & 0x1) << 2;
                xmc[23] |= (*c >> 6) & 0x3;
                xmc[24]  = (*c >> 3) & 0x7;
                xmc[25]  = *c++ & 0x7;
                Nc[2]  = (*c >> 1) & 0x7F;
                bc[2]  = (*c++ & 0x1) << 1;             /* 20 */
                bc[2] |= (*c >> 7) & 0x1;
                Mc[2]  = (*c >> 5) & 0x3;
                xmaxc[2]  = (*c++ & 0x1F) << 1;
                xmaxc[2] |= (*c >> 7) & 0x1;
                xmc[26]  = (*c >> 4) & 0x7;
                xmc[27]  = (*c >> 1) & 0x7;
                xmc[28]  = (*c++ & 0x1) << 2;
                xmc[28] |= (*c >> 6) & 0x3;
                xmc[29]  = (*c >> 3) & 0x7;
                xmc[30]  = *c++ & 0x7;
                xmc[31]  = (*c >> 5) & 0x7;
                xmc[32]  = (*c >> 2) & 0x7;
                xmc[33]  = (*c++ & 0x3) << 1;
                xmc[33] |= (*c >> 7) & 0x1;
                xmc[34]  = (*c >> 4) & 0x7;
                xmc[35]  = (*c >> 1) & 0x7;
                xmc[36]  = (*c++ & 0x1) << 2;
                xmc[36] |= (*c >> 6) & 0x3;
                xmc[37]  = (*c >> 3) & 0x7;
                xmc[38]  = *c++ & 0x7;
                Nc[3]  = (*c >> 1) & 0x7F;
                bc[3]  = (*c++ & 0x1) << 1;
                bc[3] |= (*c >> 7) & 0x1;
                Mc[3]  = (*c >> 5) & 0x3;
                xmaxc[3]  = (*c++ & 0x1F) << 1;
                xmaxc[3] |= (*c >> 7) & 0x1;
                xmc[39]  = (*c >> 4) & 0x7;
                xmc[40]  = (*c >> 1) & 0x7;
                xmc[41]  = (*c++ & 0x1) << 2;
                xmc[41] |= (*c >> 6) & 0x3;
                xmc[42]  = (*c >> 3) & 0x7;
                xmc[43]  = *c++ & 0x7;                  /* 30  */
                xmc[44]  = (*c >> 5) & 0x7;
                xmc[45]  = (*c >> 2) & 0x7;
                xmc[46]  = (*c++ & 0x3) << 1;
                xmc[46] |= (*c >> 7) & 0x1;
                xmc[47]  = (*c >> 4) & 0x7;
                xmc[48]  = (*c >> 1) & 0x7;
                xmc[49]  = (*c++ & 0x1) << 2;
                xmc[49] |= (*c >> 6) & 0x3;
                xmc[50]  = (*c >> 3) & 0x7;
                xmc[51]  = *c & 0x7;                    /* 33 */

#ifdef LTP_CUT
        }
#endif /* LTP_CUT */

        Gsm_Decoder(s, LARc, Nc, bc, Mc, xmaxc, xmc, target);

        return 0;
}

void gsm_encode(gsm s, gsm_signal * source, gsm_byte * c)
{
        word            LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13*4];
        
        Gsm_Coder(s, source, LARc, Nc, bc, Mc, xmaxc, xmc);
        *c++ =   ((GSM_MAGIC & 0xF) << 4)               /* 1 */
               | ((LARc[0] >> 2) & 0xF);
        *c++ =   ((LARc[0] & 0x3) << 6)
               | (LARc[1] & 0x3F);
        *c++ =   ((LARc[2] & 0x1F) << 3)
               | ((LARc[3] >> 2) & 0x7);
        *c++ =   ((LARc[3] & 0x3) << 6)
               | ((LARc[4] & 0xF) << 2)
               | ((LARc[5] >> 2) & 0x3);
        *c++ =   ((LARc[5] & 0x3) << 6)
               | ((LARc[6] & 0x7) << 3)
               | (LARc[7] & 0x7);
        
#ifdef LTP_CUT       
        if (s->ltp_cut)
        {
                /*
                 * Pack the frame without the LTP fields -- this saves us 36 bits when the
                 * LTP is not in use.
                 */
                *c++ =   ((Mc[0] & 0x3) << 6)
                       | (xmaxc[0]  & 0x3F);
                *c++ =   ((xmc[0] & 0x7) << 5)
                       | ((xmc[1] & 0x7) << 2)
                       | ((xmc[2] >> 1) & 0x3);
                *c++ =   ((xmc[2] & 0x1) << 7)
                       | ((xmc[3] & 0x7) << 4)
                       | ((xmc[4] & 0x7) << 1)
                       | ((xmc[5] >> 2) & 0x1);
                *c++ =   ((xmc[5] & 0x3) << 6)
                       | ((xmc[6] & 0x7) << 3)
                       | (xmc[7] & 0x7);
                *c++ =   ((xmc[8] & 0x7) << 5)
                       | ((xmc[9] & 0x7) << 2)
                       | ((xmc[10] >> 1) & 0x3);        /* 10 */
                *c++ =   ((xmc[10] & 0x1) << 7)
                       | ((xmc[11] & 0x7) << 4)
                       | ((xmc[12] & 0x7) << 1)
                       | ((Mc[1] >> 1) & 0x1);
                *c++ =   ((Mc[1] & 0x1) << 7)
                       | ((xmaxc[1] & 0x3F) << 1)
                       | ((xmc[13] >> 2) & 0x1);
                *c++ =   ((xmc[13] & 0x3) << 6)
                       | ((xmc[14] & 0x7) << 3)
                       | (xmc[15] & 0x7);
                *c++ =   ((xmc[16] & 0x7) << 5)
                       | ((xmc[17] & 0x7) << 2)
                       | ((xmc[18] >> 1) & 0x3);
                *c++ =   ((xmc[18] & 0x1) << 7)
                       | ((xmc[19] & 0x7) << 4)
                       | ((xmc[20] & 0x7) << 1)
                       | ((xmc[21] >> 2) & 0x1);
                *c++ =   ((xmc[21] & 0x3) << 6)
                       | ((xmc[22] & 0x7) << 3)
                       | (xmc[23] & 0x7);
                *c++ =   ((xmc[24] & 0x7) << 5)
                       | ((xmc[25] & 0x7) << 2)
                       | Mc[2] & 0x3;
                *c++ =   ((xmaxc[2] & 0x3F) << 2)
                       | ((xmc[26] >> 1) & 0x3);
                *c++ =   ((xmc[26] & 0x1) << 7)
                       | ((xmc[27] & 0x7) << 4)
                       | ((xmc[28] & 0x7) << 1)
                       | ((xmc[29] >> 2) & 0x1);
                *c++ =   ((xmc[29] & 0x3) << 6)
                       | ((xmc[30] & 0x7) << 3)
                       | (xmc[31] & 0x7);                               /* 20 */
                *c++ =   ((xmc[32] & 0x7) << 5)
                       | ((xmc[33] & 0x7) << 2)
                       | ((xmc[34] >> 1) & 0x3);
                *c++ =   ((xmc[34] & 0x1) << 7)
                       | ((xmc[35] & 0x7) << 4)
                       | ((xmc[36] & 0x7) << 1)
                       | ((xmc[37] >> 2) & 0x1);
                *c++ =   ((xmc[37] & 0x3) << 6)
                       | ((xmc[38] & 0x7) << 3)
                       | ((Mc[3] & 0x3) << 1)
                       | ((xmaxc[3] >> 5) & 0x1);
                *c++ =   ((xmaxc[3] & 0x1F) << 3)
                       | (xmc[39] & 0x7);
                *c++ =   ((xmc[40] & 0x7) << 5)
                       | ((xmc[41] & 0x7) << 2)
                       | ((xmc[42] >> 1) & 0x3);
                *c++ =   ((xmc[42] & 0x1) << 7)
                       | ((xmc[43] & 0x7) << 4)
                       | ((xmc[44] & 0x7) << 1)
                       | ((xmc[45] >> 2) & 0x1);
                *c++ =   ((xmc[45] & 0x3) << 6)
                       | ((xmc[46] & 0x7) << 3)
                       | (xmc[47] & 0x7);
                *c++ =   ((xmc[48] & 0x7) << 5)
                       | ((xmc[49] & 0x7) << 2)
                       | ((xmc[50] >> 1) & 0x3);
                *c++ =   ((xmc[50] & 0x1) << 7)
                       | ((xmc[51] & 0x7) << 4)
                       & 0xF0;                                          /* Last four bits are ignored */
        }
        else
        {
#endif /* LTP_CUT */

                *c++ =   ((Nc[0] & 0x7F) << 1)
                       | ((bc[0] >> 1) & 0x1);
                *c++ =   ((bc[0] & 0x1) << 7)
                       | ((Mc[0] & 0x3) << 5)
                       | ((xmaxc[0] >> 1) & 0x1F);
                *c++ =   ((xmaxc[0] & 0x1) << 7)
                       | ((xmc[0] & 0x7) << 4)
                       | ((xmc[1] & 0x7) << 1)
                       | ((xmc[2] >> 2) & 0x1);
                *c++ =   ((xmc[2] & 0x3) << 6)
                       | ((xmc[3] & 0x7) << 3)
                       | (xmc[4] & 0x7);
                *c++ =   ((xmc[5] & 0x7) << 5)                  /* 10 */
                       | ((xmc[6] & 0x7) << 2)
                       | ((xmc[7] >> 1) & 0x3);
                *c++ =   ((xmc[7] & 0x1) << 7)
                       | ((xmc[8] & 0x7) << 4)
                       | ((xmc[9] & 0x7) << 1)
                       | ((xmc[10] >> 2) & 0x1);
                *c++ =   ((xmc[10] & 0x3) << 6)
                       | ((xmc[11] & 0x7) << 3)
                       | (xmc[12] & 0x7);
                *c++ =   ((Nc[1] & 0x7F) << 1)
                       | ((bc[1] >> 1) & 0x1);
                *c++ =   ((bc[1] & 0x1) << 7)
                       | ((Mc[1] & 0x3) << 5)
                       | ((xmaxc[1] >> 1) & 0x1F);
                *c++ =   ((xmaxc[1] & 0x1) << 7)
                       | ((xmc[13] & 0x7) << 4)
                       | ((xmc[14] & 0x7) << 1)
                       | ((xmc[15] >> 2) & 0x1);
                *c++ =   ((xmc[15] & 0x3) << 6)
                       | ((xmc[16] & 0x7) << 3)
                       | (xmc[17] & 0x7);
                *c++ =   ((xmc[18] & 0x7) << 5)
                       | ((xmc[19] & 0x7) << 2)
                       | ((xmc[20] >> 1) & 0x3);
                *c++ =   ((xmc[20] & 0x1) << 7)
                       | ((xmc[21] & 0x7) << 4)
                       | ((xmc[22] & 0x7) << 1)
                       | ((xmc[23] >> 2) & 0x1);
                *c++ =   ((xmc[23] & 0x3) << 6)
                       | ((xmc[24] & 0x7) << 3)
                       | (xmc[25] & 0x7);
                *c++ =   ((Nc[2] & 0x7F) << 1)                  /* 20 */
                       | ((bc[2] >> 1) & 0x1);
                *c++ =   ((bc[2] & 0x1) << 7)
                       | ((Mc[2] & 0x3) << 5)
                       | ((xmaxc[2] >> 1) & 0x1F);
                *c++ =   ((xmaxc[2] & 0x1) << 7)
                       | ((xmc[26] & 0x7) << 4)
                       | ((xmc[27] & 0x7) << 1)
                       | ((xmc[28] >> 2) & 0x1);
                *c++ =   ((xmc[28] & 0x3) << 6)
                       | ((xmc[29] & 0x7) << 3)
                       | (xmc[30] & 0x7);
                *c++ =   ((xmc[31] & 0x7) << 5)
                       | ((xmc[32] & 0x7) << 2)
                       | ((xmc[33] >> 1) & 0x3);
                *c++ =   ((xmc[33] & 0x1) << 7)
                       | ((xmc[34] & 0x7) << 4)
                       | ((xmc[35] & 0x7) << 1)
                       | ((xmc[36] >> 2) & 0x1);
                *c++ =   ((xmc[36] & 0x3) << 6)
                       | ((xmc[37] & 0x7) << 3)
                       | (xmc[38] & 0x7);
                *c++ =   ((Nc[3] & 0x7F) << 1)
                       | ((bc[3] >> 1) & 0x1);
                *c++ =   ((bc[3] & 0x1) << 7)
                       | ((Mc[3] & 0x3) << 5)
                       | ((xmaxc[3] >> 1) & 0x1F);
                *c++ =   ((xmaxc[3] & 0x1) << 7)
                       | ((xmc[39] & 0x7) << 4)
                       | ((xmc[40] & 0x7) << 1)
                       | ((xmc[41] >> 2) & 0x1);
                *c++ =   ((xmc[41] & 0x3) << 6)                 /* 30 */
                       | ((xmc[42] & 0x7) << 3)
                       | (xmc[43] & 0x7);
                *c++ =   ((xmc[44] & 0x7) << 5)
                       | ((xmc[45] & 0x7) << 2)
                       | ((xmc[46] >> 1) & 0x3);
                *c++ =   ((xmc[46] & 0x1) << 7)
                       | ((xmc[47] & 0x7) << 4)
                       | ((xmc[48] & 0x7) << 1)
                       | ((xmc[49] >> 2) & 0x1);
                *c++ =   ((xmc[49] & 0x3) << 6)
                       | ((xmc[50] & 0x7) << 3)
                       | (xmc[51] & 0x7);
                       
#ifdef LTP_CUT
        }
#endif /* LTP_CUT */
}

