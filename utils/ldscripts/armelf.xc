ENTRY(reset)

SECTIONS
{
	.text 0x00000000 : {
		KEEP(*(.text.init))
		*(.text*)
	}

	.init_table : {
		__init_table_start = ALIGN(0x4);
		KEEP(*(.init_table*))
		__init_table_end = ALIGN(0x4);
	}

	__rodata_start = .;
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
	__heap_start = __bss_end;

	__heap_end = 0x1FE000;
	__stack = 0x1FF000;
	__fiq_stack = 0x1FFFFC;
}
