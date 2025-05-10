! KallistiOS ##version##
!
! dc/math.s
! Copyright (C) 2023 Paul Cercueil
! Copyright (C) 2025 Ruslan Rostovtsev
!
! Optimized SH4 assembler functions for bit reversal.
!
.globl _bit_reverse
.globl _bit_reverse8

!
! unsigned int bit_reverse(unsigned int value);
!
! r4: value (integer to reverse)
!
.align 2
_bit_reverse:
	mov r4,r0
	sett
_1:
	rotcr r0
	rotcl r1
	cmp/eq #1,r0
	bf _1

	rts
	mov r1,r0

!
! unsigned char bit_reverse8(unsigned char value);
!
! r4: value (byte to reverse)
!
.align 2
_bit_reverse8:
    mov	#0, r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rotcl	r0
    rotr	r4
    rts
    rotcl	r0
