@ KallistiOS 2.0.0
@
@ syscalls.s
@ (c)2002 Gil Megidish
@

.globl bios_halt
.globl bios_fastset
.globl bios_vblank

	.text

bios_halt:
	swi	#2
	bx	lr

bios_vblank:
	swi	#5
	bx	lr

bios_fastset:
	swi	#12
	bx	lr
