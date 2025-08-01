! KallistiOS ##version##
!
!   arch/dreamcast/kernel/thdswitch.s
!   Copyright (C) 2003 Megan Potter
!   Copyright (C) 2023 Paul Cercueil <paul@crapouillou.net>
!   Copyright (C) 2025 Falco Girgis
!
! Assembler code for swapping out running threads
!

	.text
	.balign		4
	.globl		_thd_block_now

! Call this function to save the current task state (with one small
! exception -- PC will be moved forwards to the "restore" code) and
! call the thread scheduler to find a new task to revive in its place.
!
! Important note: this function and all of the _irq_*_regs in entry.s
! must be kept strictly in sync as far as the structure of the saved
! registers; all of this must be kept in sync with the C code.
!
! This takes one arg: a pointer to the current task's register save
! table (same thing that normally goes in irq_srt_addr).
!
! R4 = address of register save table
!
! No explicit return value, though R0 might be changed while the
! called is blocked. Returns when we are woken again later.
!
_thd_block_now:
	! There's no need to save R0-R7 since these are not guaranteed
	! to persist across calls. So we'll put our temps down there
	! and start at R8.

	! Save SR and disable interrupts
	mov.l	irqd_and,r1
	mov.l	irqd_or,r2
	stc	sr,r0
	and	r0,r1
	or	r2,r1
	ldc	r1,sr

	add		#0x72,r4

	mov		r0,r1
	add		#0x72,r4

	sts.l		fpscr,@-r4	! save FPSCR 0xe0

	mov		r4,r3
	add		#-4,r3
	mov		#0x7,r2

1:
	! Write a bogus value (r0) at each (i*0x20) offset of the irq context
	! structure, using the movca.l opcode. This will pre-allocate cache
	! blocks that covers the whole memory area, without fetching data
	! from RAM, which means that the stores will then be as fast as they
	! can be.
	movca.l		r0,@r3
	dt		r2
	bf/s		1b
	add		#-0x20,r3

	! Ok save the "permanent" GPRs
	mov.l		r15,@-r4	! save R15   0xdc
	mov		#0x30,r2	! Set bits 20/21 to r2
	mov.l		r14,@-r4	! save R14
	shll16		r2
	mov.l		r13,@-r4	! save R13
	mov.l		r12,@-r4	! save R12
	mov.l		r11,@-r4	! save R11
	mov.l		r10,@-r4	! save R10
	mov.l		r9,@-r4		! save R9
	mov.l		r8,@-r4		! save R8    0xc0
	add		#-0x20,r4	! Skip R7-R0

	lds		r2,fpscr	! Reset FPSCR, switch to bank 2, 64-bit I/O

	fmov		dr14,@-r4	! Save FR15/FR14  0x98
	fmov		dr12,@-r4	! Save FR13/FR12
	fmov		dr10,@-r4	! Save FR11/FR10
	fmov		dr8,@-r4	! Save FR9/FR8
	fmov		dr6,@-r4	! Save FR7/FR6
	fmov		dr4,@-r4	! Save FR5/FR4
	fmov		dr2,@-r4	! Save FR3/FR2
	fmov		dr0,@-r4	! Save FR1/FR0    0x60
	frchg				! Switch back to first bank

	fmov		dr14,@-r4	! Save FR15/FR14  0x58
	fmov		dr12,@-r4	! Save FR13/FR12
	fmov		dr10,@-r4	! Save FR11/FR10
	fmov		dr8,@-r4	! Save FR9/FR8
	fmov		dr6,@-r4	! Save FR7/FR6
	fmov		dr4,@-r4	! Save FR5/FR4
	fmov		dr2,@-r4	! Save FR3/FR2
	fmov		dr0,@-r4	! Save FR1/FR0    0x20
	fschg				! Restore 32-bit I/O

	! Save any machine words. We want the swapping-in of this task
	! to simulate returning from this function, so what we'll do is
	! put PR as PC. Everything else can stay the same.
	sts.l		fpul,@-r4	! save FPUL 0x1c
	mov		r1,r0
	mov.l		r0,@-r4		! save SR
	sts.l		macl,@-r4	! save MACL
	sts.l		mach,@-r4	! save MACH
	stc.l		vbr,@-r4	! save VBR
	stc.l		gbr,@-r4	! save GBR
	sts.l		pr,@-r4		! save PR
	sts.l		pr,@-r4		! save "PC" 0x00

	! Ok, everything is saved now. There's no need to switch stacks or
	! anything, because any stack usage by the thread scheduler will not
	! disturb the saved task (assuming it doesn't overrun its stack anyway).
	mov.l		tcnaddr,r0
	jsr		@r0
	nop

	! To swap in the new task, we'll just force an interrupt return. That will
	! cover all the contingencies.
	mov.l		ifraddr,r1
	jmp		@r1
	mov		r0,r4

	.align 2
irqd_and:
	.long	0xefffff0f
irqd_or:
	.long	0x000000f0

	.balign	4
ifraddr:
	.long	_irq_force_return
tcnaddr:
	.long	_thd_choose_new
