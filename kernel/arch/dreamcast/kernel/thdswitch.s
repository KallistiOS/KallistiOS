! KallistiOS ##version##
!
!   arch/dreamcast/kernel/thdswitch.s
!   Copyright (c)2003 Megan Potter
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
	sts.l		pr,@-r15
	mov.l		idaddr,r0
	jsr		@r0
	nop
	lds.l		@r15+,pr

	! Ok save the "permanent" GPRs
	mov.l		r8,@(0x20,r4)	! Save R8
	mov.l		r9,@(0x24,r4)	! Save R9
	mov.l		r10,@(0x28,r4)	! Save R10
	mov.l		r11,@(0x2c,r4)	! Save R11
	mov.l		r12,@(0x30,r4)	! Save R12
	mov.l		r13,@(0x34,r4)	! Save R13
	mov.l		r14,@(0x38,r4)	! Save R14 (FP maybe)
	mov.l		r15,@(0x3c,r4)	! Save R15 (SP)

	! Save any machine words. We want the swapping-in of this task
	! to simulate returning from this function, so what we'll do is
	! put PR as PC. Everything else can stay the same.
	add		#0x5c,r4	! readjust register ptr
	mov.l		r0,@-r4		! save SR   0x58
	sts.l		macl,@-r4	! save MACL 0x54
	sts.l		mach,@-r4	! save MACH 0x50
	stc.l		vbr,@-r4	! save VBR  0x4c
	stc.l		gbr,@-r4	! save GBR  0x48
	sts.l		pr,@-r4		! save PR   0x44
	sts.l		pr,@-r4		! save "PC" 0x40

	! Save FPRs. We could probably skimp on FR0-FR7 but it'd probably
	! be more trouble than it's worth to figure out which bank we're
	! currently on, etc.
	add		#0x60,r4	! readjust register pointer
	add		#0x44,r4
	sts.l		fpul,@-r4	! save FPUL  0xe0
	sts.l		fpscr,@-r4	! save FPSCR 0xdc
	mov		#0,r2		! Set known FP flags
	lds		r2,fpscr
	fmov.s		fr15,@-r4	! save FR15  0xd8
	fmov.s		fr14,@-r4	! save FR14
	fmov.s		fr13,@-r4	! save FR13
	fmov.s		fr12,@-r4	! save FR12
	fmov.s		fr11,@-r4	! save FR11
	fmov.s		fr10,@-r4	! save FR10
	fmov.s		fr9,@-r4	! save FR9
	fmov.s		fr8,@-r4	! save FR8
	fmov.s		fr7,@-r4	! save FR7
	fmov.s		fr6,@-r4	! save FR6
	fmov.s		fr5,@-r4	! save FR5
	fmov.s		fr4,@-r4	! save FR4
	fmov.s		fr3,@-r4	! save FR3
	fmov.s		fr2,@-r4	! save FR2
	fmov.s		fr1,@-r4	! save FR1
	fmov.s		fr0,@-r4	! save FR0   0x9c
	frchg				! Second FP bank
	fmov.s		fr15,@-r4	! save FR15  0x98
	fmov.s		fr14,@-r4	! save FR14
	fmov.s		fr13,@-r4	! save FR13
	fmov.s		fr12,@-r4	! save FR12
	fmov.s		fr11,@-r4	! save FR11
	fmov.s		fr10,@-r4	! save FR10
	fmov.s		fr9,@-r4	! save FR9
	fmov.s		fr8,@-r4	! save FR8
	fmov.s		fr7,@-r4	! save FR7
	fmov.s		fr6,@-r4	! save FR6
	fmov.s		fr5,@-r4	! save FR5
	fmov.s		fr4,@-r4	! save FR4
	fmov.s		fr3,@-r4	! save FR3
	fmov.s		fr2,@-r4	! save FR2
	fmov.s		fr1,@-r4	! save FR1
	fmov.s		fr0,@-r4	! save FR0   0x5c
	frchg				! First FP bank again

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

	.balign	4
idaddr:
	.long	_irq_disable
ifraddr:
	.long	_irq_force_return
tcnaddr:
	.long	_thd_choose_new
