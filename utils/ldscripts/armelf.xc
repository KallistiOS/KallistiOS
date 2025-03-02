ENTRY(reset)

SECTIONS
{
	.text 0x00000000 : {
		KEEP(*(.text.init))
		*(.text*)
	}

	__rodata_start = ALIGN(0x4);
	.rodata : {
		*(.rodata*)
	}

	__data_start = ALIGN(0x4);
	.data : {
		*(.data*)
	}

	__bss_start = ALIGN(0x4);
	.bss : {
		*(.bss)
		*(COMMON)
	}

	__bss_end = ALIGN(0x4);

	__stack = 0x1FF000;
	__fiq_stack = 0x1FFFFC;
}
