/* KallistiOS ##version##

   utils/ldscripts/xbox.ld
   Copyright (C) 2026 Andress Barajas

   GNU ld linker script for the original Microsoft Xbox.

   This is the PE/COFF counterpart of the Dreamcast's shlelf.xc. It produces a
   32-bit PE image (pei-i386) laid out for later conversion to an .xbe by a
   cleanroom converter (e.g. cxbe). A future environ_xbox.sh wires it in the
   same way the Dreamcast does, e.g.:

       export KOS_LD_SCRIPT="-T${KOS_BASE}/utils/ldscripts/xbox.ld"

   What it pins down:
     - the entry point is _start (arch/xbox/kernel/startup.S);
     - the image base (conventional Xbox load address 0x00010000) and the
       0x20 file/section alignment come from the toolchain LINK_SPEC
       (--image-base / --section-alignment / --file-alignment) and reach this
       script through ld's __image_base__ / __section_alignment__ variables, so
       they are deliberately NOT hard-coded here;
     - GCC's .ctors/.dtors global-constructor mechanism (walked by libgcc's
       __main(), which the compiler calls from main()'s prologue);
     - the BSS and image-end bounds the KOS arch needs.

   Symbols exported for the KOS arch / CRT (names as written in C; PE prepends
   one underscore, so the object-level names defined below carry an extra '_'):
       _bss_start, _bss_end   BSS bounds for arch_main() to zero
       end                    first free byte; base of the KOS heap

   NOTE: early port. Companion pieces are arch/xbox/kernel/startup.S and a
   not-yet-written kernel/arch/xbox arch_main() (which is responsible for
   zeroing BSS, like the Dreamcast's init.c). A cleanroom PE->XBE converter is
   still required to turn the linked image into a bootable .xbe.
*/

OUTPUT_FORMAT(pei-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
  /* With sub-page (0x20) section alignment, keep the virtual address and file
     offset in step, exactly as ld's built-in PE script does. */
  . = SIZEOF_HEADERS;
  . = ALIGN(__section_alignment__);

  .text __image_base__ + ( __section_alignment__ < 0x1000 ? . : __section_alignment__ ) :
  {
    KEEP (*(SORT_NONE(.init)))
    *(.text .text$* .text.*)
    *(.gnu.linkonce.t.*)
    *(.glue_7t) *(.glue_7)
    KEEP (*(SORT_NONE(.fini)))
    PROVIDE (_etext = .);
    PROVIDE (etext = .);
  }

  .rdata BLOCK(__section_alignment__) :
  {
    *(.rdata .rdata$* .rdata.*)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)

    /* GCC global constructors / destructors. The markers are *defined* (not
       PROVIDEd): libgcc declares __CTOR_LIST__/__DTOR_LIST__ as common symbols
       expecting this script to override them, and __main() walks the list.
       __CTOR_LIST__[0] == -1 tells __do_global_ctors to scan to the NULL
       terminator. */
    . = ALIGN(4);
    ___CTOR_LIST__ = .; __CTOR_LIST__ = .;
    LONG (-1);
    KEEP (*(.ctors)); KEEP (*(.ctor)); KEEP (*(SORT_BY_NAME(.ctors.*)));
    LONG (0);
    ___DTOR_LIST__ = .; __DTOR_LIST__ = .;
    LONG (-1);
    KEEP (*(.dtors)); KEEP (*(.dtor)); KEEP (*(SORT_BY_NAME(.dtors.*)));
    LONG (0);

    /* DWARF-2 exception-handling frames. There is no crtbegin.o here (it
       would inject a second -1 sentinel into the hand-built ctor list
       above), so the KOS arch startup must do what crtbegin's
       __gcc_register_frame would have done:

           static struct object frame_obj;
           __register_frame_info (__EH_FRAME_BEGIN__, &frame_obj);

       The zero terminator below ends the scan. Without this registration,
       throwing a C++ exception calls std::terminate. */
    . = ALIGN(4);
    ___EH_FRAME_BEGIN__ = .; __EH_FRAME_BEGIN__ = .;
    KEEP (*(.eh_frame));
    LONG (0);
  }

  .data BLOCK(__section_alignment__) :
  {
    __data_start__ = . ;
    *(.data .data$* .data.*)
    *(.gnu.linkonce.d.*)
    KEEP (*(.jcr))
    __data_end__ = . ;
  }

  /* Unwind / exception-handling tables (required once C++ or -fexceptions
     code is linked). */
  .eh_frame BLOCK(__section_alignment__) :
  {
    KEEP (*(.eh_frame*))
  }
  .gcc_except_table BLOCK(__section_alignment__) :
  {
    *(.gcc_except_table .gcc_except_table.*)
  }

  .bss BLOCK(__section_alignment__) :
  {
    __bss_start__ = . ;   /* PE-standard name                          */
    __bss_start  = . ;    /* KOS: C "extern char _bss_start[]"         */
    *(.bss .bss$* .bss.*)
    *(COMMON)
    __bss_end__  = . ;    /* PE-standard name                          */
    __bss_end    = . ;    /* KOS: C "extern char _bss_end[]"           */
  }

  /* PE import/export/relocation/resource directories. A static XBE homebrew
     image normally leaves these empty (Xbox kernel calls go through the XBE
     thunk table built by the converter, not PE imports), but the collectors
     are kept so any stray PE section is placed predictably instead of being
     orphaned. Empty sections are dropped by ld. */
  .edata BLOCK(__section_alignment__) :
  {
    *(.edata)
  }
  .idata BLOCK(__section_alignment__) :
  {
    KEEP (SORT(*)(.idata$2))
    KEEP (SORT(*)(.idata$3))
    KEEP (SORT(*)(.idata$4))
    KEEP (SORT(*)(.idata$5))
    KEEP (SORT(*)(.idata$6))
    KEEP (SORT(*)(.idata$7))
  }
  .rsrc BLOCK(__section_alignment__) :
  {
    KEEP (*(.rsrc))
    KEEP (*(.rsrc$*))
  }
  .reloc BLOCK(__section_alignment__) :
  {
    *(.reloc)
  }

  /* First free byte after the whole image: base of the KOS heap. Defined
     last, so it sits past every loadable section (including any .idata /
     .reloc above) rather than overlapping them. */
  . = ALIGN(__section_alignment__);
  PROVIDE (_end = .);     /* KOS: C "extern char end[]"  -> heap base   */
  PROVIDE (end  = .);
  __end__ = .;

  /* Toolchain bookkeeping we never want in the image. */
  /DISCARD/ :
  {
    *(.drectve)
    *(.note.GNU-stack)
    *(.comment)
    *(.gnu.lto_*)
  }
}
