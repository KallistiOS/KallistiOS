/* KallistiOS ##version##

   arch/xbox/kernel/irqentry.S
   Copyright (C) 2026 The KallistiOS Contributors

   IA-32 exception entry. The per-vector stubs normalize hardware and
   synthetic error-code frames before saving into struct irq_context.
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

	.extern _arch_irq_dispatch
	.extern _arch_irq_get_context
	.extern _arch_irq_restore_context
	.extern _inside_int

/*
 * At common entry the stack is uniformly:
 *   +00 vector, +04 error, +08 EIP, +0c CS, +10 EFLAGS
 * After PUSHAL, those fields begin at +0x20. The interrupted ESP is therefore
 * frame + 0x34 for every same-privilege exception, with or without a
 * CPU-pushed error code.
 */
_irq_common_entry:
	cld
	pushal
	movl	%esp, %ebp

	/* Obtain the context selected for exception storage. */
	andl	$-16, %esp
	call	_arch_irq_get_context
	movl	%eax, %edi
	movl	%ebp, %esp

	movl	0x00(%ebp), %edx
	movl	%edx, CTX_EDI(%edi)
	movl	0x04(%ebp), %edx
	movl	%edx, CTX_ESI(%edi)
	movl	0x08(%ebp), %edx
	movl	%edx, CTX_EBP(%edi)
	leal	0x34(%ebp), %edx
	movl	%edx, CTX_ESP(%edi)
	movl	0x10(%ebp), %edx
	movl	%edx, CTX_EBX(%edi)
	movl	0x14(%ebp), %edx
	movl	%edx, CTX_EDX(%edi)
	movl	0x18(%ebp), %edx
	movl	%edx, CTX_ECX(%edi)
	movl	0x1c(%ebp), %edx
	movl	%edx, CTX_EAX(%edi)
	movl	0x28(%ebp), %edx
	movl	%edx, CTX_EIP(%edi)
	movl	0x30(%ebp), %edx
	movl	%edx, CTX_EFLAGS(%edi)
	movl	0x2c(%ebp), %edx
	movl	%edx, CTX_CS(%edi)

	xorl	%edx, %edx
	movw	%ds, %dx
	movl	%edx, CTX_DS(%edi)
	movw	%es, %dx
	movl	%edx, CTX_ES(%edi)
	movw	%fs, %dx
	movl	%edx, CTX_FS(%edi)
	movw	%gs, %dx
	movl	%edx, CTX_GS(%edi)
	movw	%ss, %dx
	movl	%edx, CTX_SS(%edi)
	fxsave	CTX_FXSTATE(%edi)

	incl	_inside_int

	/* arch_irq_dispatch(vector, context, error_code), with an aligned i386
	   call site. The normalized error code is at +0x24 in the saved frame. */
	andl	$-16, %esp
	subl	$4, %esp
	pushl	0x24(%ebp)
	pushl	%edi
	pushl	0x20(%ebp)
	call	_arch_irq_dispatch

	decl	_inside_int

	/* A handler may have selected another context for exception return. */
	andl	$-16, %esp
	call	_arch_irq_get_context
	jmp	_arch_irq_restore_context

	.macro IRQ_NOERR vector
	.p2align 4
	.globl _irq_stub_\vector
_irq_stub_\vector:
	pushl	$0
	pushl	$\vector
	jmp	_irq_common_entry
	.endm

	.macro IRQ_ERR vector
	.p2align 4
	.globl _irq_stub_\vector
_irq_stub_\vector:
	pushl	$\vector
	jmp	_irq_common_entry
	.endm

	IRQ_NOERR 0
	IRQ_NOERR 1
	IRQ_NOERR 2
	IRQ_NOERR 3
	IRQ_NOERR 4
	IRQ_NOERR 5
	IRQ_NOERR 6
	IRQ_NOERR 7
	IRQ_ERR   8
	IRQ_NOERR 9
	IRQ_ERR   10
	IRQ_ERR   11
	IRQ_ERR   12
	IRQ_ERR   13
	IRQ_ERR   14
	IRQ_NOERR 15
	IRQ_NOERR 16
	IRQ_ERR   17
	IRQ_NOERR 18
	IRQ_NOERR 19
	IRQ_NOERR 20
	IRQ_NOERR 21
	IRQ_NOERR 22
	IRQ_NOERR 23
	IRQ_NOERR 24
	IRQ_NOERR 25
	IRQ_NOERR 26
	IRQ_NOERR 27
	IRQ_NOERR 28
	IRQ_NOERR 29
	IRQ_NOERR 30
	IRQ_NOERR 31

	/*
	 * Hardware interrupt and software-defined vectors never carry a
	 * CPU-generated exception error code. Generate vectors 32..255 without
	 * maintaining a second hand-written entry implementation.
	 */
	.altmacro
	.set irq_vector, 32
	.rept 224
	IRQ_NOERR %irq_vector
	.set irq_vector, irq_vector + 1
	.endr
	.noaltmacro

	.section .rdata
	.p2align 2
	.globl _irq_stubs
_irq_stubs:
	.long _irq_stub_0,  _irq_stub_1,  _irq_stub_2,  _irq_stub_3
	.long _irq_stub_4,  _irq_stub_5,  _irq_stub_6,  _irq_stub_7
	.long _irq_stub_8,  _irq_stub_9,  _irq_stub_10, _irq_stub_11
	.long _irq_stub_12, _irq_stub_13, _irq_stub_14, _irq_stub_15
	.long _irq_stub_16, _irq_stub_17, _irq_stub_18, _irq_stub_19
	.long _irq_stub_20, _irq_stub_21, _irq_stub_22, _irq_stub_23
	.long _irq_stub_24, _irq_stub_25, _irq_stub_26, _irq_stub_27
	.long _irq_stub_28, _irq_stub_29, _irq_stub_30, _irq_stub_31

	.altmacro
	.macro IRQ_STUB_POINTER vector
	.long _irq_stub_\vector
	.endm
	.set irq_vector, 32
	.rept 224
	IRQ_STUB_POINTER %irq_vector
	.set irq_vector, irq_vector + 1
	.endr
	.noaltmacro
