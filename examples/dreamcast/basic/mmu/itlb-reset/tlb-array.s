! KallistiOS ##version##
! Copyright (C) 2026 Joseph Black
! Test-only direct array access. Caller excludes interrupts. No translated
! memory or stack access; all array operations execute in P2.
	.text
	.globl _tlb_probe_write
	.globl _tlb_probe_read
_tlb_probe_write:
	mov.l write_addr,r0
	mov.l p2_mask,r1
	or r1,r0
	jmp @r0
	nop
write_real:
	mov.l r5,@r4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rts
	nop
_tlb_probe_read:
	mov.l read_addr,r0
	mov.l p2_mask,r1
	or r1,r0
	jmp @r0
	nop
read_real:
	mov.l @r4,r0
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rts
	nop
	.align 2
write_addr: .long write_real
read_addr: .long read_real
p2_mask: .long 0x20000000
