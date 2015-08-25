/*
 * bni80386c.c - Low-level bignum routines, 32-bit version.
 *
 * Assembly primitives for bignum library, 80386 family, 32-bit code.
 * Inline assembler for MSVC++ 4.X
 *
 * $Id: bni80386c.c,v 1.2 1998/09/20 12:03:39 wprice Exp $
 *
 * Several primitives are included here.  Only bniMulAdd1 is *really*
 * critical, but once that's written, bniMulN1 and bniMulSub1 are quite
 * easy to write as well, so they are included here as well.
 * bniDiv21 and bniModQ are so easy to write that they're included, too.
 *
 * The usual 80x86 calling conventions have AX, BX, CX and DX
 * volatile, and SI, DI, SP and BP preserved across calls.
 * This includes the "E"xtended forms of all of those registers
 * 
 */

/*
 * Some compilers complain about #if FOO if FOO isn't defined,
 * so do the ANSI-mandated thing explicitly...
 */
#ifndef NO_ASSERT_H
#define NO_ASSERT_H 0
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

#if NEED_MEMORY_H
#include <memory.h>
#endif

#include "bni.h"
#include "bni32.h"
#include "bnimem.h"
#include "bnlegal.h"

#include "bnkludge.h"

#ifndef BNWORD32
#error 32-bit bignum library requires a 32-bit data type
#endif


/* Disable warning for no return value, typical of asm functions */
#pragma warning( disable : 4035 )

BNWORD32
bniMulAdd1_32(
BNWORD32 *outParam, BNWORD32 const *inParam, unsigned len, BNWORD32 k)
{
__asm
 {
	mov esi, inParam	; load inParam
	mov edi, outParam	; load outParam
	mov ecx, k			; load k
	push ebp			; preserve ebp for return block
	mov ebp, len		; load len (must be last)

;; First multiply step has no carry in.
	mov	eax,[esi]	; U
	mov	ebx,[edi]	;  V
	mul	ecx		; NP	first multiply
	add	ebx,eax		; U
	lea	eax,[ebp*4-4]	;  V	loop unrolling
	adc	edx,0		; U
	and	eax,12		;  V	loop unrolling
	mov	[edi],ebx	; U

	add	esi,eax		;  V	loop unrolling
	add	edi,eax		; U	loop unrolling

; inline assembler won't do tables, so use simple compare code
;	jmp	DWORD PTR ma32_jumptable[eax]	; NP	loop unrolling
;
;	align	4
;ma32_jumptable:
;	dd	ma32_case0
;	dd	ma32_case1
;	dd	ma32_case2
;	dd	ma32_case3
;
;	nop

	cmp	eax, 0
	je	ma32_case0
	cmp	eax, 4
	je	ma32_case1
	cmp	eax, 8
	je	ma32_case2
	jmp	ma32_case3

	align	8
	nop
	nop
	nop			; To align loop properly


ma32_case0:
	sub	ebp,4		; U
	jbe	SHORT ma32_done	;  V

ma32_loop:
	mov	eax,[esi+4]	; U
	mov	ebx,edx		;  V	Remember carry for later
	add	esi,16		; U
	add	edi,16		;  V
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi-12]	;  V
	adc	edx,0		; U
	add	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi-12],ebx	;  V
ma32_case3:
	mov	eax,[esi-8]	; U
	mov	ebx,edx		;  V	Remember carry for later
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi-8]	;  V
	adc	edx,0		; U
	add	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi-8],ebx	;  V
ma32_case2:
	mov	eax,[esi-4]	; U
	mov	ebx,edx		;  V	Remember carry for later
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi-4]	;  V
	adc	edx,0		; U
	add	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi-4],ebx	;  V
ma32_case1:
	mov	eax,[esi]	; U
	mov	ebx,edx		;  V	Remember carry for later
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi]	;  V
	adc	edx,0		; U
	add	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi],ebx	;  V

	sub	ebp,4		; U
	ja	SHORT ma32_loop	;  V

ma32_done:
	mov	eax,edx		; U
	pop	ebp
 }
}


BNWORD32
bniMulSub1_32(
BNWORD32 *outParam, BNWORD32 const *inParam, unsigned len, BNWORD32 k)
{
__asm
 {
	mov esi, inParam	; load inParam
	mov edi, outParam	; load outParam
	mov ecx, k			; load k
	push ebp			; preserve ebp for return block
	mov ebp, len		; load len (must be last)

;; First multiply step has no carry in.
	mov	eax,[esi]	;  V
	mov	ebx,[edi]	; U
	mul	ecx		; NP	first multiply
	sub	ebx,eax		; U
	lea	eax,[ebp*4-4]	;  V	loop unrolling
	adc	edx,0		; U
	and	eax,12		;  V	loop unrolling
	mov	[edi],ebx	; U

	add	esi,eax		;  V	loop unrolling
	add	edi,eax		; U	loop unrolling

; inline assembler won't do tables, so use simple compare code
;	jmp	DWORD PTR ms32_jumptable[eax]	; NP	loop unrolling
;
;	align	4
;ms32_jumptable:
;	dd	ms32_case0
;	dd	ms32_case1
;	dd	ms32_case2
;	dd	ms32_case3
;
;	nop

	cmp	eax, 0
	je	ms32_case0
	cmp	eax, 4
	je	ms32_case1
	cmp	eax, 8
	je	ms32_case2
	jmp	ms32_case3

	align	8
	nop
	nop
	nop

ms32_case0:
	sub	ebp,4		; U
	jbe	SHORT ms32_done	;  V

ms32_loop:
	mov	eax,[esi+4]	; U
	mov	ebx,edx		;  V	Remember carry for later
	add	esi,16		; U
	add	edi,16		;  V
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi-12]	;  V
	adc	edx,0		; U
	sub	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi-12],ebx	;  V
ms32_case3:
	mov	eax,[esi-8]	; U
	mov	ebx,edx		;  V	Remember carry for later
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi-8]	;  V
	adc	edx,0		; U
	sub	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi-8],ebx	;  V
ms32_case2:
	mov	eax,[esi-4]	; U
	mov	ebx,edx		;  V	Remember carry for later
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi-4]	;  V
	adc	edx,0		; U
	sub	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi-4],ebx	;  V
ms32_case1:
	mov	eax,[esi]	; U
	mov	ebx,edx		;  V	Remember carry for later
	mul	ecx		; NP
	add	eax,ebx		; U	Add carry in from previous word
	mov	ebx,[edi]	;  V
	adc	edx,0		; U
	sub	ebx,eax		;  V
	adc	edx,0		; U
	mov	[edi],ebx	;  V

	sub	ebp,4		; U
	ja	SHORT ms32_loop	;  V

ms32_done:
	mov	eax,edx		; U
	pop	ebp
 }
}

/* Reenable missing return value warning */
#pragma warning( default : 4035 )
