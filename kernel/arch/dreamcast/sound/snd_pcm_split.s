! KallistiOS ##version##
!
! arch/dreamcast/sound/snd_pcm_split.s
! Copyright (C) 2023, 2024 Ruslan Rostovtsev
!
! Optimized SH4 assembler code for separating stereo 8/16-bit PCM and
! stereo 4-bit ADPCM into independent single channels.
!

.section .text
.globl _snd_pcm16_split
.globl _snd_pcm8_split
.globl _snd_adpcm_split

!
! void snd_pcm16_split(uint32_t *data, uint32_t *left, uint32_t *right, size_t size);
!
	.align 2
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
! void snd_pcm8_split(uint32_t *data, uint32_t *left, uint32_t *right, size_t size);
!
	.align 2
_snd_pcm8_split:
	mov #-5, r1
	shld r1, r7
	mov #0, r0
	mov #16, r1
.pcm8_pref:
	add #32, r4
	pref @r4
	add #-32, r4
.pcm8_copy:
	mov.b @r4+, r3
	mov.b r3, @(r0,r5)
	dt r1
	mov.b @r4+, r3
	mov.b r3, @(r0,r6)
	bf/s .pcm8_copy
	add #1, r0
	dt r7
	bf/s .pcm8_pref
	mov #16, r1
	rts
	nop

!
! void snd_adpcm_split(uint32_t *data, uint32_t *left, uint32_t *right, size_t size);
!
	.align 2
_snd_adpcm_split:
	mov #-5, r1
	shld r1, r7
	mov.l r8, @-r15
	mov.l r9, @-r15
	mov.l r10, @-r15
	mov.l r11, @-r15
	mov #4, r1
	mov #15, r11
.adpcm_pref:
	add #32, r4
	pref @r4
	add #-32, r4
.adpcm_copy:
	dt r1
	!!! src = *data++;
	mov.l @r4+, r0

	!!! dst_r = src & 0xf
	mov r0, r3
	and r11, r3

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! dst_l = src & 0xf
	mov r0, r2
	and r11, r2

	!!! src >>= 4
	shld r10, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! src >>= 4
	shld r10, r0

	!!! tmp <<= 4,
	mov #4, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! src >>= 4
	shld r10, r0

	!!! tmp <<= 8,
	shll8 r9
	shll8 r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! src >>= 4
	shld r10, r0

	!!! tmp <<= 12,
	mov #12, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! src = *data++;
	mov.l @r4+, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 16,
	shll16 r9
	shll16 r8

	!!! src >>= 4
	shld r10, r0

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 20,
	mov #20, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 24,
	mov #24, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! src >>= 4
	mov #-4, r10
	shld r10, r0

	!!! tmp_r = src & 0xf
	mov r0, r9
	and r11, r9

	!!! src >>= 4
	shld r10, r0

	!!! tmp_l = src & 0xf
	mov r0, r8
	and r11, r8

	!!! tmp <<= 28,
	mov #28, r10
	shld r10, r9
	shld r10, r8

	!!! dst |= tmp
	or r9, r3
	or r8, r2

	!!! *chan++ = dst;
	mov.l r2, @r5
	mov.l r3, @r6
.adpcm_loops:
	add #4, r5
	bf/s .adpcm_copy
	add #4, r6
	dt r7
	bf/s .adpcm_pref
	mov #4, r1
.adpcm_exit:
	mov.l @r15+, r11
	mov.l @r15+, r10
	mov.l @r15+, r9
	rts
	mov.l @r15+, r8
