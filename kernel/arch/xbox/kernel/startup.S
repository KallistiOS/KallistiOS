/* KallistiOS ##version##

   arch/xbox/kernel/startup.S
   Copyright (C) 2026 Andress Barajas

   Entry point and C-runtime bootstrap for the original Microsoft Xbox.

   The Xbox runs XBE images. Its kernel loads the image at its base address
   (the KOS link spec uses the conventional 0x00010000), relocates the kernel
   thunk table referenced by the XBE header, and transfers control to the
   image entry point. At that moment the CPU is already in 32-bit protected
   mode with paging enabled and a flat memory model, and -- unlike a PC -- it
   is running in ring 0 (the title has full privilege on the Xbox).

   This stub plays the same role startup.S does on the Dreamcast: it
   establishes a sane C environment and jumps into the KOS kernel. It

       - masks interrupts until the KOS IRQ layer is ready,
       - clears the direction flag (required by the SysV i386 C ABI),
       - saves the loader's stack so arch_real_exit() can hand control back,
       - switches to a private, 16-byte-aligned stack,
       - normalizes the x87/SSE floating-point environment, and
       - calls arch_main(), the KOS kernel entry.

   The entry symbol is _start, selected by "-e _start" in the toolchain's
   LINK_SPEC; 'start' is provided as an alias to match the Dreamcast naming.

   xbox.ld and arch_main() provide the companion image layout and C bootstrap.
   Proper native-title shutdown/reboot must eventually go through the kernel
   thunk table (HalReturnToFirmware). During loader-hosted bring-up,
   arch_real_exit() restores the loader's stack and returns to its caller.
*/

/* Size of the private kernel stack reserved below, in bytes. */
#define ARCH_STACK_SIZE     (256 * 1024)

	.code32
	.text

	.globl	start
	.globl	_start
	.globl	_arch_real_exit
	.globl	_arch_old_stack
	.globl	_arch_stack_base
	.globl	_arch_stack_top

/* XBE entry point. */
_start:
start:
	cli				/* no interrupts until KOS sets up its IRQ layer */
	cld				/* SysV i386 ABI requires DF = 0                  */

	/* Remember the loader's stack so we can return to it on exit. */
	movl	%esp, _arch_old_stack

	/* Switch to our own stack and keep it 16-byte aligned, as the
	   SSE/SysV i386 ABI expects at a call boundary. */
	movl	$_arch_stack_top, %esp
	andl	$-16, %esp

	/* The Xbox CPU is a Coppermine-class Pentium III: x87 + MMX + SSE, but
	   no SSE2. Because -march=pentium3 emits SSE, make sure the FPU and SSE
	   are usable before we enter C. We run in ring 0, so touching CR0/CR4 is
	   permitted; the retail kernel normally enables this already, but we do
	   it defensively and idempotently. */
	fninit				/* reset the x87 FPU                         */
	mov	%cr0, %eax
	andl	$~0x00000004, %eax	/* CR0.EM = 0 (no x87 emulation)            */
	orl	$0x00000002, %eax	/* CR0.MP = 1 (monitor coprocessor)         */
	mov	%eax, %cr0
	mov	%cr4, %eax
	orl	$0x00000600, %eax	/* CR4.OSFXSR | CR4.OSXMMEXCPT              */
	mov	%eax, %cr4
	ldmxcsr	mxcsr_init		/* known-good SSE control/status (all masked) */

	/* Terminate the call-frame chain so stack tracers stop here. */
	xorl	%ebp, %ebp

	/* Enter KOS. arch_main() is not expected to return. */
	call	_arch_main

	/* ...but if it does, fall through to a clean exit with code 0. */
	pushl	$0
	call	_arch_real_exit

/* void arch_real_exit(int code)

   Return a loader-hosted guest through the exact stack supplied at entry.
   Native XBE shutdown never uses this path: arch_exit_handler() invokes the
   isolated HalReturnToFirmware thunk after KOS teardown. */
_arch_real_exit:
	movl	4(%esp), %eax
	movl	_arch_old_stack, %esp
	ret

	.align	4
mxcsr_init:
	.long	0x00001f80		/* default MXCSR: all SIMD FP exceptions masked */

	/*
	 * Unique module identity used by __cxa_atexit() and
	 * __cxa_thread_atexit(). PE/COFF does not supply crtbegin.o, which
	 * normally defines this symbol for ELF targets.
	 */
	.data
	.align	4
	.globl	___dso_handle
___dso_handle:
	.long	___dso_handle

	/*
	   Keep the bootstrap stack outside .bss. arch_main() clears .bss while
	   running on this stack, so placing it there would erase the live call
	   frames. xbox.ld maps this input section as an uninitialized, writable
	   part of the image that is not included in the BSS clearing range.

	   _arch_old_stack lives here for the same reason: it is written before
	   arch_main() and must survive the BSS clear for arch_real_exit().
	*/
	.section .stack,"bw"

	/* Saved loader stack pointer (set at entry, used by arch_real_exit). */
	.align	4
_arch_old_stack:
	.space	4

	/* Private kernel stack. _arch_stack_top is one past the highest byte,
	   i.e. the initial (full-descending) stack pointer. */
	.align	16
_arch_stack_base:
	.space	ARCH_STACK_SIZE
_arch_stack_top:
