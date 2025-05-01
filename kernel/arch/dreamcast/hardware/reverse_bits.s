! KallistiOS ##version##
!
! arch/dreamcast/hardware/reverse_bits.s
! Copyright (C) 2025 Ruslan Rostovtsev
!
! Optimized SH4 assembler function for reversing bits of a byte.
!

.globl _reverse_bits

!
! uint8_t reverse_bits(uint8_t b);
!
! r4: b (byte to reverse)
!
    .align 2
_reverse_bits:
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
