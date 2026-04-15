/* Script for -z combreloc: combine and sort reloc sections */
OUTPUT_FORMAT("elf32-shl", "elf32-shl",
	      "elf32-shl")
OUTPUT_ARCH(sh)
STARTUP(_kos_startup.o)
INPUT(crti.o)
INPUT(crtbegin.o)
INPUT(crtend.o)
INPUT(crtn.o)
LOAD_OFFSET = DEFINED(LOAD_OFFSET) ? LOAD_OFFSET : 0x8c010000 ;
ICACHE_SIZE = 0x2000 ;

SECTIONS
{
  /* Read-only sections, merged into text segment: */
  PROVIDE (__executable_start = LOAD_OFFSET); . = LOAD_OFFSET;

  INCLUDE common.xc

  /* Custom sections, aligned to the icache size */
  .sub0 : ALIGN(ICACHE_SIZE) {
    __sub0_start = .;
    *(.sub0*)
    __sub0_end = .;
  }
  ASSERT(__sub0_end - __sub0_start <= ICACHE_SIZE, "Error: Sub-section .sub0 is bigger than icache size")
  .sub1 : ALIGN(ICACHE_SIZE) {
    __sub1_start = .;
    *(.sub1*)
    __sub1_end = .;
  }
  ASSERT(__sub1_end - __sub1_start <= ICACHE_SIZE, "Error: Sub-section .sub1 is bigger than icache size")
  .sub2 : ALIGN(ICACHE_SIZE) {
    __sub2_start = .;
    *(.sub2*)
    __sub2_end = .;
  }
  ASSERT(__sub2_end - __sub2_start <= ICACHE_SIZE, "Error: Sub-section .sub2 is bigger than icache size")
  .sub3 : ALIGN(ICACHE_SIZE) {
    __sub3_start = .;
    *(.sub3*)
    __sub3_end = .;
  }
  ASSERT(__sub3_end - __sub3_start <= ICACHE_SIZE, "Error: Sub-section .sub3 is bigger than icache size")
  .sub4 : ALIGN(ICACHE_SIZE) {
    __sub4_start = .;
    *(.sub4*)
    __sub4_end = .;
  }
  ASSERT(__sub4_end - __sub4_start <= ICACHE_SIZE, "Error: Sub-section .sub4 is bigger than icache size")
  .sub5 : ALIGN(ICACHE_SIZE) {
    __sub5_start = .;
    *(.sub5*)
    __sub5_end = .;
  }
  ASSERT(__sub5_end - __sub5_start <= ICACHE_SIZE, "Error: Sub-section .sub5 is bigger than icache size")
  .sub6 : ALIGN(ICACHE_SIZE) {
    __sub6_start = .;
    *(.sub6*)
    __sub6_end = .;
  }
  ASSERT(__sub6_end - __sub6_start <= ICACHE_SIZE, "Error: Sub-section .sub6 is bigger than icache size")
  .sub7 : ALIGN(ICACHE_SIZE) {
    __sub7_start = .;
    *(.sub7*)
    __sub7_end = .;
  }
  ASSERT(__sub7_end - __sub7_start <= ICACHE_SIZE, "Error: Sub-section .sub7 is bigger than icache size")
  .sub8 : ALIGN(ICACHE_SIZE) {
    __sub8_start = .;
    *(.sub8*)
    __sub8_end = .;
  }
  ASSERT(__sub8_end - __sub8_start <= ICACHE_SIZE, "Error: Sub-section .sub8 is bigger than icache size")
  .sub9 : ALIGN(ICACHE_SIZE) {
    __sub9_start = .;
    *(.sub9*)
    __sub9_end = .;
  }
  ASSERT(__sub9_end - __sub9_start <= ICACHE_SIZE, "Error: Sub-section .sub9 is bigger than icache size")

  .ocram 0x7c001000 (NOLOAD) :
  {
    *(.ocram)
    /* We have 8kb of operand cache RAM. The next line lets ld throw
       an error if we exceed that size.  */
    . = . > 0x2000 ? 0x2000 : .;
  }

  .data : {
    . = ALIGN(8);
    __monitors_start = .;
    *(.monitors)
    __monitors_end = .;
  }
}
