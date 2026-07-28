/* KallistiOS ##version##

   arch/xbox/kernel/thdswitch.S
   Copyright (C) 2026 The KallistiOS Contributors

   Cooperative i386 context switch. Keep all offsets synchronized with
   struct irq_context and the static assertions in irq.c.
*/

#define CTX_EDI       0x00
#define CTX_ESI       0x04
#define CTX_EBP       0x08
#define CTX_ESP       0x0c
#define CTX_EBX       0x10
#define CTX_EDX       0x14
#define CTX_ECX       0x18
#define CTX_EAX       0x1c
#define CTX_EIP       0x20
#define CTX_EFLAGS    0x24
#define CTX_CS        0x28
#define CTX_DS        0x2c
#define CTX_ES        0x30
#define CTX_FS        0x34
#define CTX_GS        0x38
#define CTX_SS        0x3c
#define CTX_FXSTATE   0x40

	.code32
	.text
	.p2align 4
	.globl _thd_block_now

/*
 * int thd_block_now(irq_context_t *current)
 *
 * Save the caller as though this function returned, ask the portable
 * scheduler for the next context, and restore it with IRET. IRET makes the
 * target EIP, CS, and EFLAGS (especially IF) active atomically.
 */
_thd_block_now:
	pushfl
	cli
	pushal

	/* pushal frame at ESP:
	   +00 EDI, +04 ESI, +08 EBP, +0c pre-pushal ESP,
	   +10 EBX, +14 EDX, +18 ECX, +1c EAX,
	   +20 EFLAGS, +24 return EIP, +28 context argument. */
	movl	0x28(%esp), %eax

	movl	0x00(%esp), %edx
	movl	%edx, CTX_EDI(%eax)
	movl	0x04(%esp), %edx
	movl	%edx, CTX_ESI(%eax)
	movl	0x08(%esp), %edx
	movl	%edx, CTX_EBP(%eax)
	/* Resume as if thd_block_now returned: skip its return address, leaving
	   ESP at the caller's argument slot (S + 4, or frame + 0x28). */
	leal	0x28(%esp), %edx
	movl	%edx, CTX_ESP(%eax)
	movl	0x10(%esp), %edx
	movl	%edx, CTX_EBX(%eax)
	movl	0x14(%esp), %edx
	movl	%edx, CTX_EDX(%eax)
	movl	0x18(%esp), %edx
	movl	%edx, CTX_ECX(%eax)
	movl	0x1c(%esp), %edx
	movl	%edx, CTX_EAX(%eax)
	movl	0x24(%esp), %edx
	movl	%edx, CTX_EIP(%eax)
	movl	0x20(%esp), %edx
	movl	%edx, CTX_EFLAGS(%eax)

	xorl	%edx, %edx
	movw	%cs, %dx
	movl	%edx, CTX_CS(%eax)
	movw	%ds, %dx
	movl	%edx, CTX_DS(%eax)
	movw	%es, %dx
	movl	%edx, CTX_ES(%eax)
	movw	%fs, %dx
	movl	%edx, CTX_FS(%eax)
	movw	%gs, %dx
	movl	%edx, CTX_GS(%eax)
	movw	%ss, %dx
	movl	%edx, CTX_SS(%eax)

	fxsave	CTX_FXSTATE(%eax)

	/* GCC's i386 ABI expects ESP to be 16-byte aligned at a call site.
	   The saved entry frame leaves it 8 modulo 16, so reserve padding.
	   This abandoned frame need not be unwound after the scheduler chooses
	   another context. */
	subl	$8, %esp

	/* Scheduler returns the context selected for execution in EAX. */
	call	_thd_choose_new

	jmp	_arch_irq_restore_context

/*
 * Restore the context in EAX. Exception entry uses the same exit path, keeping
 * cooperative and interrupt-driven context restoration byte-for-byte equal.
 * This routine never returns.
 */
	.globl _arch_irq_restore_context
_arch_irq_restore_context:
	/* Restore floating-point and data-segment state first. */
	fxrstor	CTX_FXSTATE(%eax)
	movw	CTX_DS(%eax), %dx
	movw	%dx, %ds
	movw	CTX_ES(%eax), %dx
	movw	%dx, %es
	movw	CTX_FS(%eax), %dx
	movw	%dx, %fs
	movw	CTX_GS(%eax), %dx
	movw	%dx, %gs

	/* Build a same-privilege IRET frame immediately below target ESP. */
	movl	CTX_ESP(%eax), %ecx
	subl	$12, %ecx
	movl	CTX_EIP(%eax), %edx
	movl	%edx, 0(%ecx)
	movl	CTX_CS(%eax), %edx
	movl	%edx, 4(%ecx)
	movl	CTX_EFLAGS(%eax), %edx
	movl	%edx, 8(%ecx)
	movl	%ecx, %esp

	movl	CTX_EDI(%eax), %edi
	movl	CTX_ESI(%eax), %esi
	movl	CTX_EBP(%eax), %ebp
	movl	CTX_EBX(%eax), %ebx
	movl	CTX_EDX(%eax), %edx
	movl	CTX_ECX(%eax), %ecx
	movl	CTX_EAX(%eax), %eax
	iret
