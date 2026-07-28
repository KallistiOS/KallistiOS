/* KallistiOS ##version##

   arch/xbox/kernel/gdtload.S
   Copyright (C) 2026 The KallistiOS Contributors
*/

	.code32
	.text
	.p2align 4
	.globl _xbox_gdt_activate

/* void xbox_gdt_activate(const descriptor_register_t *gdtr, uint16_t fs,
                          uint16_t gs, uint16_t tr) */
_xbox_gdt_activate:
	movl	4(%esp), %eax
	lgdt	(%eax)
	ljmp	$0x08, $1f
1:
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	8(%esp), %ax
	movw	%ax, %fs
	movw	12(%esp), %ax
	movw	%ax, %gs
	movw	16(%esp), %ax
	testw	%ax, %ax
	jz	2f
	ltr	%ax
2:
	ret
