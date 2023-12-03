! KallistiOS ##version##
!
!   arch/dreamcast/kernel/entry.s
!   (c)2000-2001 Megan Potter
!
! Assembler code for entry and exit to/from the kernel via exceptions
!

! Routine that all exception handlers jump to after setting
! an error code. Out of neccessity, all of this function is in
! assembler instead of C. Once the registers are saved, we will
! jump into a shared routine. This register save and restore code
! is mostly from my sh-stub code. For now this is pretty high overhead
! for a context switcher (or especially a timer! =) but it can
! be optimized later.

	.text
	.align		2
	.globl		_irq_srt_addr
	.globl		_irq_handle_exception
	.globl		_irq_save_regs
	.globl		_irq_force_return

! Static kernel-mode stack; we can get away with this because in our
! tiny microkernel, only one thread will ever actually be sitting inside
! the kernel code. All other threads will be halted at the point at which
! they made their trap call into the kernel. This lets us do all sorts of
! useful things, like freedom to remap memory at any time, safety from
! mis-mapped user-mode stack pointers, etc. It also opens the door for
! hard real-time interrupts and exceptions that interrupt the kernel
! itself.
	.space		4096		! One page
krn_stack:

! All exception vectors lead to Rome (i.e., this label).
_irq_save_regs:
! On the SH4, an exception triggers a toggle of RB in SR. So all
! the R0-R7 registers were convienently saved for us.
	mov.l		_irq_srt_addr,r0	! Grab the location of the reg store
	add		#0x20,r0	! Start at the top of the BANK regs
	stc.l		r7_bank,@-r0	! Save R7
	stc.l		r6_bank,@-r0	! Save R6
	stc.l		r5_bank,@-r0	! Save R5
	stc.l		r4_bank,@-r0	! Save R4
	stc.l		r3_bank,@-r0	! Save R3
	stc.l		r2_bank,@-r0	! Save R2
	stc.l		r1_bank,@-r0	! Save R1
	stc.l		r0_bank,@-r0	! Save R0
	
	mov.l		r8,@(0x20,r0)	! save R8
	mov.l		r9,@(0x24,r0)	! save R9
	mov.l		r10,@(0x28,r0)	! save R10
	mov.l		r11,@(0x2c,r0)	! save R11
	mov.l		r12,@(0x30,r0)	! save R12
	mov.l		r13,@(0x34,r0)	! save R13
	mov.l		r14,@(0x38,r0)	! save R14
	mov.l		r15,@(0x3c,r0)	! save R15 (SP)
	add		#0x5c,r0	! readjust register pointer
	stc.l		ssr,@-r0	! save SSR  0x58
	sts.l		macl,@-r0	! save MACL 0x54
	sts.l		mach,@-r0	! save MACH 0x50
	stc.l		vbr,@-r0	! save VBR  0x4c
	stc.l		gbr,@-r0	! save GBR  0x48
	sts.l		pr,@-r0		! save PR   0x44
	stc.l		spc,@-r0	! save PC   0x40
	
	add		#0x60,r0	! readjust register pointer
	add		#0x44,r0
	sts.l		fpul,@-r0	! save FPUL  0xe0
	sts.l		fpscr,@-r0	! save FPSCR 0xdc
	mov		#0,r2		! Set known FP flags
	lds		r2,fpscr
	fmov.s		fr15,@-r0	! save FR15  0xd8
	fmov.s		fr14,@-r0	! save FR14
	fmov.s		fr13,@-r0	! save FR13
	fmov.s		fr12,@-r0	! save FR12
	fmov.s		fr11,@-r0	! save FR11
	fmov.s		fr10,@-r0	! save FR10
	fmov.s		fr9,@-r0	! save FR9
	fmov.s		fr8,@-r0	! save FR8
	fmov.s		fr7,@-r0	! save FR7
	fmov.s		fr6,@-r0	! save FR6
	fmov.s		fr5,@-r0	! save FR5
	fmov.s		fr4,@-r0	! save FR4
	fmov.s		fr3,@-r0	! save FR3
	fmov.s		fr2,@-r0	! save FR2
	fmov.s		fr1,@-r0	! save FR1
	fmov.s		fr0,@-r0	! save FR0   0x9c
	frchg				! Second FP bank
	fmov.s		fr15,@-r0	! save FR15  0x98
	fmov.s		fr14,@-r0	! save FR14
	fmov.s		fr13,@-r0	! save FR13
	fmov.s		fr12,@-r0	! save FR12
	fmov.s		fr11,@-r0	! save FR11
	fmov.s		fr10,@-r0	! save FR10
	fmov.s		fr9,@-r0	! save FR9
	fmov.s		fr8,@-r0	! save FR8
	fmov.s		fr7,@-r0	! save FR7
	fmov.s		fr6,@-r0	! save FR6
	fmov.s		fr5,@-r0	! save FR5
	fmov.s		fr4,@-r0	! save FR4
	fmov.s		fr3,@-r0	! save FR3
	fmov.s		fr2,@-r0	! save FR2
	fmov.s		fr1,@-r0	! save FR1
	fmov.s		fr0,@-r0	! save FR0   0x5c
	frchg				! First FP bank again

	! Setup our kernel-mode stack
	mov.l		stkaddr,r15

	! Before we enter the main C code again, re-enable exceptions
	! (but not interrupts) so we can still debug inside handlers.
	bsr		_irq_disable
	nop

	! R4 still contains the exception code
	mov.l		hdl_except,r2	! Call handle_exception
	jsr		@r2
	nop
	bra		_save_regs_finish
	nop

! irq_force_return() jumps here; make sure we're in register
! bank 1 (as opposed to 0)
_irq_force_return:
	mov.l	_irqfr_or,r1
	stc	sr,r0
	or	r1,r0
	ldc	r0,sr
	bra	_save_regs_finish
	nop
	
	.align 2
_irqfr_or:
	.long	0x20000000
stkaddr:
	.long	krn_stack


! Now restore all the registers and jump back to the thread
_save_regs_finish:
	mov.l	_irq_srt_addr, r1	! Get register store address
	ldc.l	@r1+,r0_bank		! restore R0	(r1 is now _irq_srt_addr+0)
	ldc.l	@r1+,r1_bank		! restore R1
	ldc.l	@r1+,r2_bank		! restore R2
	ldc.l	@r1+,r3_bank		! restore R3
	ldc.l	@r1+,r4_bank		! restore R4
	ldc.l	@r1+,r5_bank		! restore R5
	ldc.l	@r1+,r6_bank		! restore R6
	ldc.l	@r1+,r7_bank		! restore R7
	add	#-32,r1			! Go back to the front
	mov.l	@(0x20,r1), r8		! restore R8	(r1 is now ...+0)
	mov.l	@(0x24,r1), r9		! restore R9
	mov.l	@(0x28,r1), r10		! restore R10
	mov.l	@(0x2c,r1), r11		! restore R11
	mov.l	@(0x30,r1), r12		! restore R12
	mov.l	@(0x34,r1), r13		! restore R13
	mov.l	@(0x38,r1), r14		! restore R14
	mov.l	@(0x3c,r1), r15		! restore program's stack
	
	add	#0x40,r1		! jump up to status words
	ldc.l	@r1+,spc		! restore SPC 0x40	(r1 is now +0x40)
	lds.l	@r1+,pr			! restore PR  0x44	(+0x44)
	ldc.l	@r1+,gbr		! restore GBR 0x48	(+0x48)
!	ldc.l	@r1+,vbr		! restore VBR (don't play with VBR)
	add	#4,r1			!			(+0x4c)
	lds.l	@r1+,mach		! restore MACH 0x50	(+0x50)
	lds.l	@r1+,macl		! restore MACL 0x54	(+0x54)
	ldc.l	@r1+,ssr		! restore SSR  0x58	(+0x58)

	mov	#0,r2			! Set known FP flags	(+0x5c)
	lds	r2,fpscr
	frchg				! Second FP bank
	fmov.s	@r1+,fr0		! restore FR0  0x5c
	fmov.s	@r1+,fr1		! restore FR1
	fmov.s	@r1+,fr2		! restore FR2
	fmov.s	@r1+,fr3		! restore FR3
	fmov.s	@r1+,fr4		! restore FR4
	fmov.s	@r1+,fr5		! restore FR5
	fmov.s	@r1+,fr6		! restore FR6
	fmov.s	@r1+,fr7		! restore FR7
	fmov.s	@r1+,fr8		! restore FR8
	fmov.s	@r1+,fr9		! restore FR9
	fmov.s	@r1+,fr10		! restore FR10
	fmov.s	@r1+,fr11		! restore FR11
	fmov.s	@r1+,fr12		! restore FR12
	fmov.s	@r1+,fr13		! restore FR13
	fmov.s	@r1+,fr14		! restore FR14
	fmov.s	@r1+,fr15		! restore FR15 0x98
	frchg				! First FP bank
	fmov.s	@r1+,fr0		! restore FR0  0x9c
	fmov.s	@r1+,fr1		! restore FR1
	fmov.s	@r1+,fr2		! restore FR2
	fmov.s	@r1+,fr3		! restore FR3
	fmov.s	@r1+,fr4		! restore FR4
	fmov.s	@r1+,fr5		! restore FR5
	fmov.s	@r1+,fr6		! restore FR6
	fmov.s	@r1+,fr7		! restore FR7
	fmov.s	@r1+,fr8		! restore FR8
	fmov.s	@r1+,fr9		! restore FR9
	fmov.s	@r1+,fr10		! restore FR10
	fmov.s	@r1+,fr11		! restore FR11
	fmov.s	@r1+,fr12		! restore FR12
	fmov.s	@r1+,fr13		! restore FR13
	fmov.s	@r1+,fr14		! restore FR14
	fmov.s	@r1+,fr15		! restore FR15  0xd8
	lds.l	@r1+,fpscr		! restore FPSCR 0xdc
	lds.l	@r1+,fpul		! restore FPUL  0xe0

!	add	#-0x70,r1		! jump back to registers
!	add	#-0x34,r1
!	mov.l	@(0,r1),r0		! restore R0
!	mov.l	@(4,r1),r1		! restore R1
	mov	#2,r0

	rte				! return
	nop
	
	.align 2
_irq_srt_addr:
	.long	0	! Save Regs Table -- this is an indirection
			! so we can easily swap out pointers during a
			! context switch.
hdl_except:
	.long	_irq_handle_exception


! Special case handler for TLB miss exceptions. There are two reasons
! why we'd want to do this and complicate things. The first is speed --
! if TLB misses happen often (which is likely if we're using the MMU
! allocator) then saving the full processor context and switching
! back is going to be a major drain on the dcache and also just
! general processor time. Second reason is that it allows us to process
! these inside an IRQ/exception handler without having to have nestable
! exceptions just yet. That's a whole 'nother egg I don't want to
! break just yet.
!
! !!NOTE!! This is highly dependent on the structure of the MMU tables
! in mmu.h and the MMU code in mmu.c. If either of those change, this will
! likely need to change as well.
	.text
	.align 2
tlb_miss_hnd:
	! Get the exception event code; we want to handle only
	! 0x0040 (ITLB_MISS/DTLB_MISS_READ) or 0x0060 (DTLB_MISS_WRITE)
	mov	#-1,r3		! 0xff000024 (EXPEVT) -> r3
	shll16	r3
	shll8	r3
	add	#0x24,r3
	mov.l	@r3,r0		! Get EXPEVT

	mov	#0x40,r1	! 0x0040 -> r1

	cmp/eq	r0,r1
	bt.s	tmh_doit
	mov	#0x60,r1

	cmp/eq	r0,r1
	bt	tmh_doit

	! It's not one of the MISS codes, just send it on to the normal
	! irq processing.
	bra	_irq_save_regs
	mov	#2,r4

tmh_doit:
	! So it's an ITLB or DTLB_MISS code. Look at the MMU module's
	! shortcut flag. If that's set, it's safe to pass on processing
	! directly to the mapping function.

	! Check the shortcut flag
	mov.l	tmh_shortcut_addr,r0
	mov.l	@r0,r0
	cmp/pz	r0
	bt	tmh_clear
	bra	_irq_save_regs
	mov	#2,r4

tmh_clear:
	! Coast is clear -- setup the args and call the C function. Regs R0-R7
	! are volatile on SH-4 anyway, and R8-R14 will be saved if needed
	! onto our temp stack. So all we need to worry about here, at least
	! for this small C call, is the stack. To faciliate the stack, we'll
	! save R15 and setup a small temp stack.
	mov.l	tmh_stack_save_addr,r0		! Setup stack
	mov.l	r15,@r0
	mov.l	tmh_temp_stack_addr,r15

	mov	#0,r4				! Call gen_miss
	mov	#0,r5
	mov.l	tmh_gen_miss_addr,r0
	jsr	@r0
	mov	#0,r6

	mov.l	tmh_stack_save,r15		! Fix stack back

	! Return back from the exception
	rte
	nop

	.align	2
tmh_shortcut_addr:
	.long	_mmu_shortcut_ok
tmh_stack_save_addr:
	.long	tmh_stack_save
tmh_stack_save:
	.long	0
tmh_temp_stack_addr:
	.long	tmh_temp_stack
tmh_gen_miss_addr:
	.long	_mmu_gen_tlb_miss

	.data
	.space	256
tmh_temp_stack:


! The SH4 has very odd exception handling. Instead of having a vector
! table like a sensible processor, it has a vector code block. *sigh*
! Thus this table of assembly code. Note that we can't catch reset
! exceptions at all, but that really shouldn't matter.
	.text
	.align 2
	.globl _irq_vma_table
_irq_vma_table:
	.rep	0x100
	.byte	0
	.endr
	
_vma_table_100:		! General exceptions
	nop				! Can't have a branch as the first instr
	bra	_irq_save_regs
	mov	#1,r4			! Set exception code
	
	.rep	0x300 - 6
	.byte	0
	.endr

_vma_table_400:		! TLB miss exceptions (MMU)
	nop
!	bra	tlb_miss_hnd
!	nop
	bra	_irq_save_regs
	mov	#2,r4			! Set exception code

	.rep	0x200 - 6
	.byte	0
	.endr

_vma_table_600:		! IRQs
	nop
	bra	_irq_save_regs
	mov	#3,r4			! Set exception code


! Disable interrupts, but leave exceptions enabled. Returns the old
! interrupt status.
!
! Calling this inside an exception/interrupt handler will generally not have
! any effect.
!
	.globl	_irq_disable
_irq_disable:
	mov.l	_irqd_and,r1
	mov.l	_irqd_or,r2
	stc	sr,r0
	and	r0,r1
	or	r2,r1
	ldc	r1,sr
	rts
	nop
	
	.align 2
_irqd_and:
	.long	0xefffff0f
_irqd_or:
	.long	0x000000f0


! Enable interrupts and exceptions. Returns the old interrupt status.
!
! Call this inside an exception/interrupt handler only with GREAT CARE.
!
	.globl	_irq_enable
_irq_enable:
	mov.l	_irqe_and,r1
	stc	sr,r0
	and	r0,r1
	ldc	r1,sr
	rts
	nop
	
	.align 2
_irqe_and:
	.long	0xefffff0f


! Restore interrupts to the state returned by irq_disable()
! or irq_enable().
	.globl	_irq_restore
_irq_restore:
	ldc	r4,sr
	rts
	nop


! Retrieve SR
	.globl	_irq_get_sr
_irq_get_sr:
	rts
	stc	sr,r0
