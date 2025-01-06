! KallistiOS ##version##
!
!   arch/dreamcast/hardware/pvr/pvr_send_to_ta.s
!   Copyright (C) 2024 Paul Cercueil
!
! Fast function to upload geometry to the Tile Accelerator without SQs
!

.globl _pvr_send_to_ta

_pvr_send_to_ta:
	fschg
	mov.l	_irq_and,r1
	mov	#0x78,r2
	mov.l	_ta_addr,r3
	stc	sr,r0
	and	r0,r1
	fmov	@r4+,dr0
	shll	r2
	fmov	@r4+,dr2
	or	r2,r1
	fmov	@r4+,dr4
	fmov	@r4+,dr6
	ldc	r1,sr

	movca.l	r0,@r3
	add	#32,r3
	fmov	dr6,@-r3
	fmov	dr4,@-r3
	fmov	dr2,@-r3
	fmov	dr0,@-r3
	fschg

	ldc	r0,sr

	rts
	ocbp	@r3

.align 2
_irq_and:
	.long	0xefffff0f
_ta_addr:
	.long	0x90000000
