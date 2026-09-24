! Reset all four ITLB entries from P2; caller excludes concurrent refills.

	.text
	.globl _mmu_reset_itlb

_mmu_reset_itlb:
	mov.l	mraddr,r0
	mov.l	p2mask,r1
	or	r1,r0
	jmp	@r0
	nop

	.align	2
mraddr:	.long	mmu_reset_real
p2mask:	.long	0x20000000
	

mmu_reset_real:
	! Clear the ITLB Address array
	mov.l	itlb1,r4
	mov	#0,r0
	mov	#1,r1
	! Entry selection is address bits 9:8 in all three ITLB arrays.
	shll8	r1
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4

	! Clear ITLB Data Array 1
	mov.l	itlb2,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4

	! Clear ITLB Data Array 2
	mov.l	itlb3,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4
	add	r1,r4
	mov.l	r0,@r4

	! make sure we have enough instrs before returning to P1
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rts
	nop

	.align	2
itlb1:	.long	0xf2000000 
itlb2:	.long	0xf3000000
itlb3:	.long	0xf3800000
