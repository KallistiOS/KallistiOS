/* KallistiOS ##version##

   utils/ldscripts/xbox.ld
   Copyright (C) 2026 Andress Barajas

   GNU ld linker script for the original Microsoft Xbox.

   This is the PE/COFF counterpart of the Dreamcast's shlelf.xc. It produces a
   32-bit PE image (pei-i386) laid out for later conversion to an .xbe by a
   cleanroom converter (e.g. cxbe). environ_xbox.sh wires it in in the same
   way the Dreamcast environment selects its architecture linker script:

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

   Companion pieces are arch/xbox/kernel/startup.S and arch/xbox/kernel/init.c,
   whose arch_main() clears BSS like the Dreamcast implementation. A cleanroom
   PE->XBE converter is still required to turn a linked image into a native
   bootable .xbe; the network loader may consume a converted ELF directly.
*/

OUTPUT_FORMAT(pei-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)
STARTUP(_kos_startup.o)

SECTIONS
{
  /* With sub-page (0x20) section alignment, keep the virtual address and file
     offset in step, exactly as ld's built-in PE script does. */
  . = SIZEOF_HEADERS;
  . = ALIGN(__section_alignment__);

  .text __image_base__ + ( __section_alignment__ < 0x1000 ? . : __section_alignment__ ) :
  {
    PROVIDE (__executable_start = .);
    PROVIDE (_executable_start = .);
    KEEP (*(SORT_NONE(.init)))
    *(.text .text$* .text.*)
    *(.gnu.linkonce.t.*)
    *(.glue_7t) *(.glue_7)
    KEEP (*(SORT_NONE(.fini)))
    PROVIDE (__etext = .);
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

  /* Standard writable XBE kernel-thunk table. The PE-to-XBE converter points
     the native image header here so the Xbox kernel can resolve the ordinals
     before entry. Loader-hosted ELF execution does not patch or use it. */
  .xbethnk BLOCK(__section_alignment__) :
  {
    KEEP (*(.xbethnk))
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

  /*
     Bootstrap stack storage is deliberately outside .bss: arch_main() clears
     the BSS while executing on this stack. The input section is marked as
     uninitialized storage, reserving virtual address space without storing
     256 KiB of zeroes in the PE/XBE file.

     Keep this before `end` so the KOS heap begins after the reserved stack.
     This works whether the XBE is entered directly by the Xbox kernel or by a
     loader; neither path needs to initialize the stack's previous contents.
  */
  .stack BLOCK(__section_alignment__) :
  {
    __stack_start__ = .;
    KEEP (*(.stack))
    KEEP (*(.stack.*))
    __stack_end__ = .;
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
  __heap_reserve_size__ = DEFINED(__xbox_native_heap_top)
                            ? __xbox_native_heap_top - .
                            : 0;

  /*
     A directly loaded XBE receives mappings only for ranges described by its
     section table. Native links define __xbox_native_heap_top so the XBE
     contains a writable, fileless reservation covering KOS's heap. Loader
     ELF links omit the symbol and this output section remains empty because
     xbox-load-ip already owns and maps its bounded guest arena.

     Keep `end` above at the real image end: mm_init() must start allocation
     at the beginning of this reserved range, not after it.
  */
  .heap BLOCK(__section_alignment__) (NOLOAD) :
  {
    __heap_reserve_start__ = .;
    . += __heap_reserve_size__;
  }
  __heap_reserve_end__ = .;

  /*
     Keep DWARF data in the PE for source-level debugging, but place it
     explicitly after the loadable image. PE ld otherwise orphans these
     sections at VMA zero and warns that they are below the image base.
     Their recognized names keep them non-allocatable, but some kos-tool ELF
     upload paths still transmit any section with a nonzero address. Loader
     ELF conversion must therefore strip these sections while retaining the
     PE image as the source-level-debug symbol file.
  */
  .debug_aranges BLOCK(__section_alignment__) :
  {
    *(.debug_aranges .debug_aranges.*)
  }
  .debug_info BLOCK(__section_alignment__) :
  {
    *(.debug_info .debug_info.* .gnu.linkonce.wi.*)
  }
  .debug_abbrev BLOCK(__section_alignment__) :
  {
    *(.debug_abbrev .debug_abbrev.*)
  }
  .debug_line BLOCK(__section_alignment__) :
  {
    *(.debug_line .debug_line.*)
  }
  .debug_frame BLOCK(__section_alignment__) :
  {
    *(.debug_frame .debug_frame.*)
  }
  .debug_str BLOCK(__section_alignment__) :
  {
    *(.debug_str .debug_str.*)
  }
  .debug_line_str BLOCK(__section_alignment__) :
  {
    *(.debug_line_str .debug_line_str.*)
  }
  .debug_loc BLOCK(__section_alignment__) :
  {
    *(.debug_loc .debug_loc.*)
  }
  .debug_loclists BLOCK(__section_alignment__) :
  {
    *(.debug_loclists .debug_loclists.*)
  }
  .debug_ranges BLOCK(__section_alignment__) :
  {
    *(.debug_ranges .debug_ranges.*)
  }
  .debug_rnglists BLOCK(__section_alignment__) :
  {
    *(.debug_rnglists .debug_rnglists.*)
  }
  .debug_addr BLOCK(__section_alignment__) :
  {
    *(.debug_addr .debug_addr.*)
  }
  .debug_pubnames BLOCK(__section_alignment__) :
  {
    *(.debug_pubnames .debug_pubnames.*)
  }
  .debug_pubtypes BLOCK(__section_alignment__) :
  {
    *(.debug_pubtypes .debug_pubtypes.*)
  }
  .debug_macinfo BLOCK(__section_alignment__) :
  {
    *(.debug_macinfo .debug_macinfo.*)
  }
  .debug_macro BLOCK(__section_alignment__) :
  {
    *(.debug_macro .debug_macro.*)
  }
  .debug_names BLOCK(__section_alignment__) :
  {
    *(.debug_names .debug_names.*)
  }
  .debug_types BLOCK(__section_alignment__) :
  {
    *(.debug_types .debug_types.* .gnu.linkonce.wt.*)
  }
  .gdb_index BLOCK(__section_alignment__) :
  {
    *(.gdb_index)
  }

  /* Toolchain bookkeeping we never want in the image. */
  /DISCARD/ :
  {
    *(.drectve)
    *(.note.GNU-stack)
    *(.comment)
    *(.gnu.lto_*)
  }
}
