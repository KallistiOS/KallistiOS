# AICAOS
#
# crt0.s
# Copyright (C) 2000-2002 Megan Potter
# Copyright (C) 2025 Paul Cercueil
#
# Startup for ARM program

.text
.section .text.init

.extern	arm_main
.extern	fiq_handler
.extern	current_task

# Meaningless but makes the linker shut up
.globl	reset
reset:

	# Exception vectors
	b	start
	mov	pc,r14		// Undef
	mov	pc,r14		// Softint
	sub	pc,r14,#4	// Prefetch abort
	sub	pc,r14,#4	// Data abort
	sub	pc,r14,#4	// Reserved instruction
	sub	pc,r14,#4	// IRQ

	# Beginning of FIQ exception routine

	# Disable IRQs/FIQs
	mrs	r8, CPSR
	orr	r8, r8, #0xc0
	msr	CPSR_c, r8

	# Save r0-r7 and r14 (old PC) inside the thread structure
	ldr	r8, =current_task
	ldr	r8, [r8]
	stmia	r8, {r0-r7,r14}

	# Save the banked CPSR for later
	mrs	r1, SPSR

	# Switch to Supervisor mode
	# and disable interrupts
	mrs	r9, CPSR
	orr	r9, r9, #0xd3
	msr	CPSR_c, r9
	nop
	nop
	nop
	nop

	# Save r8-r14 and the CPSR
	ldr	r0, =current_task
	ldr	r0, [r0]
	add	r0, r0, #36
	stmia	r0!, {r8-r14}
	str	r1, [r0]

	# Use the FIQ stack for the moment
	ldr	sp,=__fiq_stack

	# Jump to the C handler
	b	fiq_handler

start:
	# Setup a basic stack
	ldr	sp,=__stack

	# Call the main for the SPU
	b	arm_main
