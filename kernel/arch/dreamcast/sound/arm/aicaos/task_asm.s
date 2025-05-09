# AICAOS
#
# task_asm.s
# Copyright (C) 2025 Paul Cercueil
#
# Low-level task handling

.text
.align

.extern current_task
.extern __task_reschedule
.extern __task_exit
.globl task_exit
.globl task_select
.globl task_reschedule
.globl task_yield

task_select:
	# Restore r8-r14
	add	r1, r0, #36
	ldmia	r1, {r8-r14}

	# Switch to FIQ mode, to mask r8-r14
	mrs	r1, CPSR
	bic	r1, r1, #0x1f
	orr	r1, r1, #0x11
	msr	CPSR_c, r1
	nop
	nop
	nop
	nop

	# Restore the CPSR
	ldr	r8, [r0, #64]
	msr	SPSR, r8

	# Restore r0-r7 and PC
	mov	r8, r0
	ldmia	r8, {r0-r7, lr}

	# Jump to the new task
	subs	pc, lr, #4

task_exit:
	# Switch from the task's stack back to the main stack
	ldr	sp,=__stack
	b	__task_exit

task_yield:
	mov	r0, #1
	b	1f

task_reschedule:
	mov	r0, #0
1:
	mrs	r2, CPSR
	orr	r1, r2, #0xc0
	msr	CPSR_c, r1

	# Save r0-r7 on the task structure
	ldr	r1, =current_task
	ldr	r1, [r1]
	stmia	r1,{r0-r7}

	# Save r8-r14 and the CPSR
	add	r1, r1, #36
	stmia	r1!, {r8-r14}
	str	r2, [r1]

	# Use the FIQ stack from now on
	ldr	sp,=__fiq_stack

	# Save r14 as PC
	add	r2, pc, #8
	str	r2, [r1, #-32]
	b	__task_reschedule
	mov	pc, lr

