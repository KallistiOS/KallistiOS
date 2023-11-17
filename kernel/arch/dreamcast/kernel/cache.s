
! KallistiOS ##version##
!
! arch/dreamcast/kernel/cache.s
!
! Copyright (C) 2001 Megan Potter
! Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev
! Copyright (C) 2023 Andy Barajas
!
! Optimized assembler code for managing the cache.
!

    .text
    .globl _icache_flush_range
    .globl _dcache_inval_range
    .globl _dcache_flush_range
    .globl _dcache_flush_all
    .globl _dcache_purge_range
    .globl _dcache_purge_all
    .globl _dcache_purge_all_with_buffer

! Routine to flush parts of cache.. Thanks to the Linux-SH guys
! for the algorithm. The original version of this routine was
! taken from sh-stub.c.
!
! r4 is starting address
! r5 is count
    .align 2
_icache_flush_range:
    mov.l    ifr_addr, r0
    mov.l    p2_mask, r1
    or       r1, r0
    jmp      @r0
    nop

.iflush_real:
    ! Save old SR and disable interrupts
    stc      sr, r0
    mov.l    r0, @-r15
    mov.l    ormask, r1
    or       r1, r0
    ldc      r0, sr

    ! Get ending address from count and align start address
    add      r4, r5
    mov.l    align_mask, r0
    and      r0, r4
    mov.l    ica_addr, r1
    mov.l    ic_entry_mask, r2
    mov.l    ic_valid_mask, r3

.flush_loop:
    ! Write back D cache
    ocbwb    @r4

    ! Invalidate I cache
    mov      r4, r6        ! v & CACHE_IC_ENTRY_MASK
    and      r2, r6
    or       r1, r6        ! CACHE_IC_ADDRESS_ARRAY | ^

    mov      r4, r7        ! v & 0xfffffc00
    and      r3, r7

    add      #32, r4       ! Move on to next cache block
    cmp/hs   r4, r5
    bt/s     .flush_loop
    mov.l    r7, @r6       ! *addr = data    

    ! Restore old SR
    mov.l    @r15+, r0
    ldc      r0, sr

    ! make sure we have enough instrs before returning to P1
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    rts
    nop


! This routine goes through and invalidates the dcache for a given 
! range of RAM. Make sure that you've called dcache_flush_range first
! if you care about the contents.
!
! r4 is starting address
! r5 is count
    .align 2
_dcache_inval_range:
    ! Get ending address from count and align start address
    add      r4, r5
    mov.l    align_mask, r0
    and      r0, r4

.dinval_loop:
    ! Invalidate the dcache
    ocbi     @r4
    cmp/hs   r4, r5
    bt/s     .dinval_loop
    add      #32, r4        ! Move on to next cache block

    rts
    nop


! This routine goes through and forces a write-back on the
! specified data range. Use prior to dcache_inval_range if you
! care about the contents. If the range is bigger than the dcache,
! we flush the whole cache instead.
!
! r4 is starting address
! r5 is count
    .align 2
_dcache_flush_range:
    ! Divide byte count by 32 
    mov      #-5, r1
    shad     r1, r5           

    ! Compare with flush_check
    mov.w    flush_check, r2
    cmp/hi   r2, r5
    bt       _dcache_flush_all  ! If lines > flush_check, jump to _dcache_flush_all

    ! Align start address
    mov.l    align_mask, r0
    and      r0, r4

.dflush_loop:
    ! Write back the dcache
    ocbwb    @r4
    dt       r5
    bf/s     .dflush_loop
    add      #32, r4        ! Move on to next cache block

    rts
    nop


! This routine uses the OC address array to have direct access to the
! dcache entries.  It forces a write-back on all dcache entries where
! the U bit and V bit are set to 1.  Then updates the entry with
! U bit cleared.
    .align 2
_dcache_flush_all:
    mov.l    dca_addr, r1
    mov.w    cache_lines, r2
    mov.l    dc_ubit_mask, r3

.dflush_all_loop:
    mov.l    @r1, r0     ! Get dcache array entry value
    and      r3, r0      ! Zero out U bit
    dt       r2
    mov.l    r0, @r1     ! Update dcache entry

    bf/s     .dflush_all_loop
    add      #32, r1     ! Move on to next entry

    rts
    nop


! This routine goes through and forces a write-back and invalidate
! on the specified data range. If the range is bigger than the dcache,
! we purge the whole cache instead.
!
! r4 is starting address
! r5 is count
    .align 2
_dcache_purge_range:
    ! Divide byte count by 32 
    mov      #-5, r1
    shad     r1, r5           

    ! Compare with purge_check
    mov.w    purge_check, r2
    cmp/hi   r2, r5
    bt       _dcache_purge_all  ! If lines > purge_check, jump to _dcache_purge_all

    ! Align start address
    mov.l    align_mask, r0
    and      r0, r4

.dpurge_loop:
    ! Write back and invalidate the D cache
    ocbp     @r4
    dt       r5
    bf/s     .dpurge_loop
    add      #32, r4     ! Move on to next cache block

    rts
    nop


! This routine uses the OC address array to have direct access to the
! dcache entries.  It goes through and forces a write-back and invalidate
! on all of the dcache.
    .align 2
_dcache_purge_all:
    mov.l    dca_addr, r1
    mov.w    cache_lines, r2
    mov      #0, r3
    
.dpurge_all_loop:
    mov.l    r3, @r1     ! Update dcache entry
    dt       r2
    bf/s     .dpurge_all_loop
    add      #32, r1     ! Move on to next entry

    rts
    nop


! This routine forces a write-back and invalidate all dcache
! using a 8kb or 16kb 32-byte aligned buffer.
!
! r4 is address for temporary buffer 32-byte aligned
! r5 is size of temporary buffer (8 KB or 16 KB)
    .align 2
_dcache_purge_all_with_buffer:
    mov      #0, r0
    add      r4, r5

.dpurge_all_buffer_loop:
    ! Allocate and then invalidate the dcache line
    movca.l  r0, @r4
    ocbi     @r4
    cmp/hs   r4, r5
    bt/s     .dpurge_all_buffer_loop
    add      #32, r4        ! Move on to next cache block

    rts
    nop


! Variables
    .align    2

! I-cache (Instruction cache)
ica_addr:
    .long    0xf0000000    ! icache array address
ic_entry_mask:
    .long    0x1fe0        ! CACHE_IC_ENTRY_MASK
ic_valid_mask:
    .long    0xfffffc00
ifr_addr:    
    .long    .iflush_real

! D-cache (Data cache)
dca_addr:
    .long    0xf4000000    ! dcache array address
dc_ubit_mask:
    .long    0xfffffffd    ! Mask to zero out U bit

! Shared    
p2_mask:    
    .long    0xa0000000
ormask:
    .long    0x100000f0
align_mask:
    .long    ~31           ! Align address to 32-byte boundary
cache_lines:
    .word    512           ! Total number of cache lines in dcache

! _dcache_flush_range can execute up to this amount of loops and 
! beat execution time of _dcache_flush_all.  This means that 
! dcache_flush_range can have count param set up to 66560 bytes 
! and still be faster than dcache_flush_all.
flush_check:
    .word    2080
    
! _dcache_purge_range can execute up to this amount of loops and 
! beat execution time of _dcache_purge_all.  This means that 
! dcache_purge_range can have count param set up to 39936 bytes 
! and still be faster than dcache_purge_all.
purge_check:
    .word    1248        

