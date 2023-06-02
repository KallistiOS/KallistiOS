! KallistiOS ##version##
!
! dc/math.s
! Copyright (C) 2023 Paul Cercueil

! Return the bit-reverse value of the first argument.
.globl _bit_reverse
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
