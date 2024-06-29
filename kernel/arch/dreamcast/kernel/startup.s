! KallistiOS ##version##
!
! startup.s
! Copyright (C) 2000-2001 Megan Potter
! Copyright (C) 2024 Andy Barajas
!
! This file is added to GCC during the patching stage of toolchain
! compilation. Any changes to this file will not take effect until the
! toolchain is recompiled.
!
! This file must appear FIRST in your linking order, or your program won't
! work correctly as a raw binary.
!
! This is very loosely based on Marcus' crt0.s/startup.s
!

.globl start
.globl _start
.globl _arch_real_exit
.globl __arch_old_sr
.globl __arch_old_vbr
.globl __arch_old_stack
.globl __arch_old_fpscr
.globl __arch_mem_top

#ifdef PROFILE
.globl _gprof_init
#endif

.weak   _arch_stack_16m
.weak   _arch_stack_32m

_start:
start:
	! Disable interrupts (if they're enabled)
	mov.l	old_sr_addr,r0
	stc	sr,r1
	mov.l	r1,@r0
	mov.l	init_sr,r0
	ldc	r0,sr

	! Run in the P2 area
	mov.l	setup_cache_addr,r0
	mov.l	p2_mask,r1
	or	r1,r0
	jmp	@r0
	nop

setup_cache:
	! Now that we are in P2, it's safe to enable the cache
	! Check to see if we should enable OCRAM.
	mov.l	kos_init_flags_addr, r0
	mov.b 	@(3,r0),r0
	tst 	#0x10,r0
	bf	.L_setup_cache_L0
	mov.w	ccr_data,r1
	bra	.L_setup_cache_L1
	nop
.L_setup_cache_L0:
	mov.w	ccr_data_ocram,r1
.L_setup_cache_L1:
	mov.l	ccr_addr,r0
	mov.l	r1,@r0

	! After changing CCR, eight instructions must be executed before
	! it's safe to enter a cached area such as P1
	nop			! 1
	nop			! 2
	nop			! 3
	nop			! 4
	nop			! 5 (d-cache now safe)
	nop			! 6
	mov.l	init_addr,r0	! 7
	mov	#0,r1		! 8
	jmp	@r0		! go
	mov	r1,r0
	nop

init:
	! Save old PR on old stack so we can get to it later
	sts.l	pr,@-r15

	! Save the current stack, and set a new stack (higher up in RAM)
	mov.l	old_stack_addr,r0
	mov.l	r15,@r0
	mov.l   new_stack_16m,r15
	mov.l	@r15,r15

	! Check if 0xadffffff is a mirror of 0xacffffff, or if unique
	! If unique, then memory is 32MB instead of 16MB, and we must
	! set up new stack even higher
	mov.l	p2_mask,r0
	mov	r0,r2
	or	r15,r2
	mov	#0xba,r1
	mov.b	r1,@-r2			! Store 0xba to 0xacffffff
	mov.l	new_stack_32m,r1
	mov.l	@r1,r1
	or	r0,r1
	mov	#0xab,r0
	mov.b	r0,@-r1			! Store 0xab in 0xadffffff
	mov.b	@r1,r0
	mov.b	@r2,r1			! Reloaded values
	cmp/eq	r0,r1			! Check if values match
	bt	memchk_done		! If so, mirror - we're done, move on
	mov.l	new_stack_32m,r15	! If not, unique - set higher stack
	mov.l	@r15,r15
memchk_done:
	mov.l	mem_top_addr,r0
	mov.l	r15,@r0			! Save address of top of memory

	! Save VBR
	mov.l	old_vbr_addr,r0
	stc	vbr,r1
	mov.l	r1,@r0

	! Save FPSCR
	mov.l	old_fpscr_addr,r0
	sts	fpscr,r1
	mov.l	r1,@r0

	! Reset FPSCR
	mov	#4,r4		! Use 00040000 (DN=1)
	mov.l	fpscr_addr,r0
	jsr	@r0
	shll16	r4

	! Setup a sentinel value for frame pointer in case we're using
	! FRAME_POINTERS for stack tracing.
	mov	#-1,r14

	! Jump to the kernel main
	mov.l	main_addr,r0
	jsr	@r0
	nop

	! Program can return here (not likely) or jump here directly
	! from anywhere in it to go straight back to the monitor
_arch_real_exit:
	! Save exit code parameter to r8
	mov r4, r8

	! Reset SR
	mov.l	old_sr,r0
	ldc	r0,sr

	! Disable MMU, invalidate TLB
	mov	#4,r0
	mov.l	mmu_addr,r1
	mov.l	r0,@r1

	! Wait (just in case)
	nop				! 1
	nop				! 2
	nop				! 3
	nop				! 4
	nop				! 5
	nop				! 6
	nop				! 7
	nop				! 8

	! Restore VBR
	mov.l	old_vbr,r0
	ldc	r0,vbr

	! If we're working under dcload, call its EXIT syscall
	mov.l	dcload_magic_addr,r0
	mov.l	@r0,r0
	mov.l	dcload_magic_value,r1
	cmp/eq	r0,r1
	bf	normal_exit

	mov.l	dcload_syscall,r0
	mov.l	@r0,r0
	! Move saved exit code to be used as exit syscall parameter
	mov r8, r5
	jsr	@r0
	mov	#15,r4

	! Set back the stack and return (presumably to a serial debug)
normal_exit:
	mov.l	old_stack,r15
	lds.l	@r15+,pr
	rts
	nop

#ifdef PROFILE
! Call profiler startup code
	.align 2
_gprof_init:
	mov.l lowpc, r4
	mov.l highpc, r5
	mov.l monstartup_addr, r2
	jmp @r2
	nop

! GPROF variables
	.align 2

monstartup_addr:
	.long __monstartup
lowpc:
	.long __executable_start
highpc:
	.long __etext
#endif /* PROFILE */

! Misc variables
	.align	2

dcload_magic_addr:
	.long	0x8c004004
dcload_magic_value:
	.long	0xdeadbeef
dcload_syscall:
	.long	0x8c004008
__arch_old_sr:
old_sr:
	.long	0
__arch_old_vbr:
old_vbr:
	.long	0
__arch_old_fpscr:
old_fpscr:
	.long	0
init_sr:
	.long	0x500000f0
old_sr_addr:
	.long	old_sr
old_vbr_addr:
	.long	old_vbr
old_fpscr_addr:
	.long	old_fpscr
old_stack_addr:
	.long	old_stack
__arch_old_stack:
old_stack:
	.long	0
__arch_mem_top:
	.long	0
mem_top_addr:
	.long	__arch_mem_top
new_stack_16m:
	.long	_arch_stack_16m
new_stack_32m:
	.long	_arch_stack_32m
p2_mask:
	.long	0xa0000000
setup_cache_addr:
	.long	setup_cache
init_addr:
	.long	init
main_addr:
	.long	_arch_main
mmu_addr:
	.long	0xff000010
fpscr_addr:
	.long	___set_fpscr	! in libgcc
kos_init_flags_addr:
	.long	___kos_init_flags
ccr_addr:
	.long	0xff00001c
ccr_data:
	.word	0x090d
ccr_data_ocram:
	.word	0x092d
