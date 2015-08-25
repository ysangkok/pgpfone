/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/*$Header: /pgpsrc/clients/fone/common/gsm.h,v 1.2 1998/09/20 12:09:28 wprice Exp $*/

#ifndef	GSM_H
#define	GSM_H

/*
 *	Interface
 */

typedef struct gsm_state * 	gsm;
typedef short		   	gsm_signal;		/* signed 16 bit */
typedef unsigned char	gsm_byte;
typedef gsm_byte 		   gsm_frame[33];		/* 33 * 8 bits	 */

#define	GSM_MAGIC	0xD			  	/* 13 kbit/s RPE-LTP */

#define	GSM_PATCHLEVEL	7
#define	GSM_MINOR	0
#define	GSM_MAJOR	1

#define	GSM_OPT_VERBOSE	1
#define	GSM_OPT_FAST	2
#define	GSM_OPT_LTP_CUT	3

#define LTP_CUT

#ifdef __cplusplus	/* [ */
extern "C" {
#endif
extern gsm  gsm_create(void);
extern void gsm_destroy(gsm);

extern short  gsm_option(gsm, short, short *);

extern void gsm_encode(gsm, gsm_signal *, gsm_byte  *);
extern int  gsm_decode(gsm, gsm_byte   *, gsm_signal *);

extern int  gsm_explode(gsm, gsm_byte   *, gsm_signal *);
extern void gsm_implode(gsm, gsm_signal *, gsm_byte   *);
#ifdef __cplusplus	/* [ */
}
#endif
#endif	/* GSM_H */
