! KallistiOS ##version##
!
!   arch/dreamcast/kernel/entry.s
!   Copyright (C) 2000, 2001 Megan Potter
!   Copyright (C) 2023 Paul Cercueil <paul@crapouillou.net>
!   Copyright (C) 2025 Falco Girgis
!
! Assembler code for entry and exit to/from the kernel via exceptions
!

! Routine that all exception handlers jump to after setting
! an error code. Out of necessity, all of this function is in
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
	add		#0x72,r0	! Start at the top of the BANK regs
	add		#0x72,r0
	sts.l		fpscr,@-r0	! save FPSCR 0xe0

	mov		r0,r1
	add		#-4,r1
	mov		#0x7,r2

1:
	! Write a bogus value (r0) at each (i*0x20) offset of the irq context
	! structure, using the movca.l opcode. This will pre-allocate cache
	! blocks that covers the whole memory area, without fetching data
	! from RAM, which means that the stores will then be as fast as they
	! can be.
	movca.l		r0,@r1
	dt		r2
	bf/s		1b
	add		#-0x20,r1

	mov.l		r15,@-r0	! save R15   0xdc
	mov		#0x30,r2	! Set bits 20/21 to r2
	mov.l		r14,@-r0	! save R14   0xd8
	shll16		r2		!
	mov.l		r13,@-r0	! save R13   0xd4
	mov.l		r12,@-r0	! save R12
	mov.l		r11,@-r0	! save R11
	mov.l		r10,@-r0	! save R10
	mov.l		r9,@-r0		! save R9
	mov.l		r8,@-r0		! save R8
	stc.l		r7_bank,@-r0	! Save R7
	stc.l		r6_bank,@-r0	! Save R6
	stc.l		r5_bank,@-r0	! Save R5
	stc.l		r4_bank,@-r0	! Save R4
	stc.l		r3_bank,@-r0	! Save R3
	stc.l		r2_bank,@-r0	! Save R2
	stc.l		r1_bank,@-r0	! Save R1
	stc.l		r0_bank,@-r0	! Save R0    0xa0
	lds		r2,fpscr	! Reset FPSCR, switch to bank 2, 64-bit I/O

	fmov		dr14,@-r0	! Save FR15/FR14  0x98
	fmov		dr12,@-r0	! Save FR13/FR12
	fmov		dr10,@-r0	! Save FR11/FR10
	fmov		dr8,@-r0	! Save FR9/FR8
	fmov		dr6,@-r0	! Save FR7/FR6
	fmov		dr4,@-r0	! Save FR5/FR4
	fmov		dr2,@-r0	! Save FR3/FR2
	fmov		dr0,@-r0	! Save FR1/FR0    0x60
	frchg				! Switch back to first bank

	fmov		dr14,@-r0	! Save FR15/FR14  0x58
	fmov		dr12,@-r0	! Save FR13/FR12
	fmov		dr10,@-r0	! Save FR11/FR10
	fmov		dr8,@-r0	! Save FR9/FR8
	fmov		dr6,@-r0	! Save FR7/FR6
	fmov		dr4,@-r0	! Save FR5/FR4
	fmov		dr2,@-r0	! Save FR3/FR2
	fmov		dr0,@-r0	! Save FR1/FR0    0x20
	fschg				! Restore 32-bit I/O

	! Setup our kernel-mode stack
	mov.l		stkaddr,r15

	sts.l		fpul,@-r0	! save FPUL  0x1c
	stc.l		ssr,@-r0	! save SSR
	sts.l		macl,@-r0	! save MACL
	sts.l		mach,@-r0	! save MACH
	stc.l		vbr,@-r0	! save VBR
	stc.l		gbr,@-r0	! save GBR
	sts.l		pr,@-r0		! save PR
	stc.l		spc,@-r0	! save PC    0x00

	! Before we enter the main C code again, re-enable exceptions
	! (but not interrupts) so we can still debug inside handlers.
	mov.l	irqd_and,r1
	mov.l	irqd_or,r2
	stc	sr,r0
	and	r0,r1
	or	r2,r1
	ldc	r1,sr

	! R4 still contains the exception code
	mov.l		hdl_except,r2	! Call handle_exception
	jsr		@r2
	nop
	bra		_save_regs_finish
	nop

	.align 2
irqd_and:
	.long	0xefffff0f
irqd_or:
	.long	0x000000f0

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
	mov	#0x10,r2		! Set bit 20 to r2
	ldc.l	@r1+,spc		! restore SPC 0x00
	lds.l	@r1+,pr			! restore PR
	ldc.l	@r1+,gbr		! restore GBR
!	ldc.l	@r1+,vbr		! restore VBR (don't play with VBR)
	add	#4,r1			!
	lds.l	@r1+,mach		! restore MACH
	lds.l	@r1+,macl		! restore MACL
	ldc.l	@r1+,ssr		! restore SSR  0x18
	lds.l	@r1+,fpul		! restore FPUL 0x1c
	shll16	r2
	lds	r2,fpscr		! Reset FPSCR, 64-bit I/O

	fmov	@r1+,dr0		! restore FR0/FR1    0x20
	fmov	@r1+,dr2		! restore FR2/FR3
	fmov	@r1+,dr4		! restore FR4/FR5
	fmov	@r1+,dr6		! restore FR6/FR7
	fmov	@r1+,dr8		! restore FR8/FR9
	fmov	@r1+,dr10		! restore FR10/FR11
	fmov	@r1+,dr12		! restore FR12/FR13
	fmov	@r1+,dr14		! restore FR14/FR15  0x58
	frchg				! Second FP bank

	fmov	@r1+,dr0		! restore FR0/FR1    0x60
	fmov	@r1+,dr2		! restore FR2/FR3
	fmov	@r1+,dr4		! restore FR4/FR5
	fmov	@r1+,dr6		! restore FR6/FR7
	fmov	@r1+,dr8		! restore FR8/FR9
	fmov	@r1+,dr10		! restore FR10/FR11
	fmov	@r1+,dr12		! restore FR12/FR13
	fmov	@r1+,dr14		! restore FR14/FR15  0x98

	ldc.l	@r1+,r0_bank		! restore R0    0xa0
	ldc.l	@r1+,r1_bank		! restore R1
	ldc.l	@r1+,r2_bank		! restore R2
	ldc.l	@r1+,r3_bank		! restore R3
	ldc.l	@r1+,r4_bank		! restore R4
	ldc.l	@r1+,r5_bank		! restore R5
	ldc.l	@r1+,r6_bank		! restore R6
	ldc.l	@r1+,r7_bank		! restore R7
	mov.l	@r1+,r8			! restore R8
	mov.l	@r1+,r9			! restore R9
	mov.l	@r1+,r10		! restore R10
	mov.l	@r1+,r11		! restore R11
	mov.l	@r1+,r12		! restore R12
	mov.l	@r1+,r13		! restore R13
	mov.l	@r1+,r14		! restore R14
	mov.l	@r1+,r15		! restore R15   0xdc

	lds.l	@r1+,fpscr		! restore FPSCR 0xe0

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
	! for this small C call, is the stack. To facilitate the stack, we'll
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