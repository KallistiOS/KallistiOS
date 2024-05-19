! KallistiOS ##version##
!
!   arch/dreamcast/kernel/entry.s
!   Copyright (C) 2024 Paul Cercueil <paul@crapouillou.net>
!
! Assembler code to implement atomic compare-and-swap
!

.text
.align 2

.globl __sh_cas_atomic_begin
.globl __sh_cas_atomic_end

__sh_cas_atomic_begin:
	mov.l @r4,r0
	cmp/eq r4,r5
	bf __sh_cas_atomic_end
	mov.l r6,@r4

__sh_cas_atomic_end:
	rts
	nop
