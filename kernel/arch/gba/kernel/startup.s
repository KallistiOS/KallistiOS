@ KallistiOS 2.0.0
@
@ startup.s
@ (c)2001 Dan Potter
@
@ This file must appear FIRST in your linking order, or your program won't
@ work correctly as a raw binary.
@
@ This is very loosely based on Chuck Mason's gbacrt0.s, which was
@ apparently based very loosely on Elo's =)
@

.globl _start
.globl arch_real_exit

.extern __bss_start__, __bss_end__, _etext, _sdata
.extern __bss_start, __end__, _edata, _end, _stack, _etext
.extern _ssarm, _esarm
.extern arch_main
.extern arm_irq_handler

RAM_BASE = 0x02000000
RAM_END = 0x02040000

WRAM_BASE = 0x03000000

cpu_mode_usr = 0x10
cpu_mode_irq = 0x12
cpu_mode_svc = 0x13

cpu_stack_usr = 0x03007f00
cpu_stack_irq = 0x03007fa0
cpu_stack_svc = 0x03007fe0

interrupt_vector = 0x03007ffc


	.text

_start:
	b	_real_start
	@ This bit here is the GBA ROM header (or space for it)
	.long	0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0

	.long	0,0,0,0,0,0,0,0

_real_start:	
	@ Switch to IRQ mode, and set the IRQ mode stack
	mov	r0, #(cpu_mode_irq | 0x80 | 0x40)
	msr	cpsr_c, r0
	ldr	r13,=cpu_stack_irq

	@ Switch to SVC mode, and set the SVC mode stack
	mov	r0, #(cpu_mode_svc | 0x80 | 0x40)
	msr	cpsr_c, r0
	ldr	r13,=cpu_stack_svc

	@ Copy .data into RAM
	ldr	r0,=_sdata	@ Start of data segment
	mov	r1,#RAM_BASE	@ Start of GBA RAM
	ldr	r3,=_edata	@ End of data segment
_copy_loop:
	cmp	r1,r3
	ldrcc	r2, [r0], #4
	strcc	r2, [r1], #4
	bcc	_copy_loop

	@ Zero the rest of RAM
	mov	r3,#RAM_END
	mov	r2,#0
_zero_loop:
	cmp	r1,r3
	strcc	r2,[r1],#4
	bcc	_zero_loop

@	@ Copy .sarm into internal WRAM
@	ldr	r0,=_ssarm	@ Start of sarm segment
@	mov	r1,#WRAM_BASE	@ Start of GBA internal WRAM
@	ldr	r3,=_esarm	@ End of sarm segment
@_copy_loop_2:
@	cmp	r1,r3
@	ldrcc	r2,[r0],#4
@	strcc	r2,[r1],#4
@	bcc	_copy_loop_2

	@ Enable interrupts during SVC mode
	mov	r0, #(cpu_mode_svc | 0x40)
	msr	cpsr_c, r0

	@ Disable interrupts during USR mode
	mov	r0, #(cpu_mode_usr | 0x80 | 0x40)
	msr	cpsr_c, r0

	@ Default IRQ handler
	ldr	r0,=interrupt_vector
	adr	r1,_irq_hnd
	str	r1,[r0]

	@ Re-enable interrupts in USR mode
	mov	r0,#(cpu_mode_usr | 0x40)
	msr	cpsr_c, r0
	ldr	sp,=cpu_stack_usr

	@ Call main program
	ldr	r0,=arch_main
	mov	lr,pc
	bx	r0		@ Call Thumb code

	@ Crash.
_done:
	swi	#3		@ GBA syscall: halt
	bal	_done

_arch_main_sym:
	.long	arch_main

_irq_handler_sym:
	.long	arm_irq_handler

_irq_hnd:
	@ We start out in ARM mode, but must switch to Thumb to
	@ go any further (since the rest of the OS is written in
	@ Thumb mode).
	ldr	r0,=_irq_handler_sym
	ldr	r0,[r0]
	bx	r0
