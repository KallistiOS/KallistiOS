! KallistiOS ##version##
!
! arch/dreamcast/sound/snd_pcm_split.s
! Copyright (C) 2023 Ruslan Rostovtsev
!
! Optimized assembler code for separating stereo PCM 16-bit to single channels
!

.section .text
.globl _snd_pcm16_split
.globl _snd_pcm16_split_sq_start

.align 2

!
! void snd_pcm16_split(uint32_t *data, uint32_t *left, uint32_t *right, uint32_t size);
!
_snd_pcm16_split:
	mov #-5, r3
	shld r3, r7
	mov.l r8, @-r15
	mov.l r11, @-r15
	mov.l r12, @-r15
	mov r4, r8
	add #32, r8
	mov #31, r3
	mov #0, r0
.pcm16_pref:
	pref @r8
.pcm16_load:
	tst r3, r0
	mov.l @r4+, r1
	mov.l @r4+, r2
	swap.w r1, r11
	mov r2, r12
	xtrct r11, r12
	swap.w r2, r11
	bt/s .pcm16_store_alloc
	xtrct r1, r11
.pcm16_store:
	mov.l r11, @(r0,r5)
	mov.l r12, @(r0,r6)
.pcm16_loops:
	tst r3, r4
	bf/s .pcm16_load
	add #4, r0
	dt r7
	bf/s .pcm16_pref
	add #32, r8
.pcm16_exit:
	mov.l @r15+, r12
	mov.l @r15+, r11
	mov.l @r15+, r8
	rts
	nop
.pcm16_store_alloc:
	add r0, r5
	add r0, r6
	mov r11, r0
	movca.l r0, @r5
	mov r12, r0
	movca.l r0, @r6
	bra .pcm16_loops
	mov #0, r0

!
! void snd_pcm16_split_sq_start(uint32_t *data, uint32_t left, uint32_t right, uint32_t size);
!
_snd_pcm16_split_sq_start:
	mov #-5, r3
	shld r3, r7
	mov.l r8, @-r15
	mov.l r11, @-r15
	mov.l r12, @-r15
	mov r4, r8
	add #32, r8
	mov #31, r3
	mov #0, r0
.pcm16_sq_pref:
	pref @r8
.pcm16_sq_load:
	mov.l @r4+, r1
	mov.l @r4+, r2
	swap.w r1, r11
	mov r2, r12
	xtrct r11, r12
	swap.w r2, r11
	xtrct r1, r11
	mov.l r11, @(r0,r5)
	mov.l r12, @(r0,r6)
	tst r3, r4
	bf/s .pcm16_sq_load
	add #4, r0
	tst r3, r0
	bf .pcm16_sq_count
.pcm16_sq_flush:
	mov r5, r1
	add r0, r1
	add #-32, r1
	pref @r1
	mov r6, r2
	add r0, r2
	add #-32, r2
	pref @r2
.pcm16_sq_count:
	dt r7
	bf/s .pcm16_sq_pref
	add #32, r8
.pcm16_sq_exit:
	mov.l @r15+, r12
	mov.l @r15+, r11
	mov.l @r15+, r8
	rts
	nop
