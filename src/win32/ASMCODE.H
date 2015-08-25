#ifndef ASMCODE_H
#define ASMCODE_H

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

extern word		Fgsm_mult	(word a, word b);
extern longword	Fgsm_L_mult	(word a, word b);
extern word		Fgsm_mult_r	(word a, word b);
extern word		Fgsm_div	(word num, word denum);
extern word		Fgsm_add	(word a, word b );
extern longword	Fgsm_L_add	(longword a, longword b );
extern word		Fgsm_sub	(word a, word b);
extern longword	Fgsm_L_sub	(longword a, longword b);
extern word		Fgsm_abs	(word a);
extern word		Fgsm_norm	(longword a );
extern longword	Fgsm_L_asl	(longword a, int n);
extern word		Fgsm_asl	(word a, int n);
extern longword	Fgsm_L_asr	(longword a, int n);
extern word		Fgsm_asr	(word a, int n);

void FShort_term_analysis_filtering(word   *u, word   * rp, int    k_n, word   * s);
void FShort_term_synthesis_filtering(word   *v, word   * rrp, int k, word   * wt, word   * sr);
longword FLTP_loop_internal(word *wt,word *dp) ;

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* ASMCODE_H */

