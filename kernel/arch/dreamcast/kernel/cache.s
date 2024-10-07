
! KallistiOS ##version##
!
! arch/dreamcast/kernel/cache.s
!
! Copyright (C) 2001 Megan Potter
! Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev
! Copyright (C) 2023, 2024 Andy Barajas
! Copyright (C) 2024, 2025 Paul Cercueil
! Copyright (C) 2025 Matt Slevinsky
! Copyright (C) 2025 TapamN
!
! Optimized assembler code for managing the cache.
!

    .text
    .globl _icache_inval_range
    .globl _icache_flush_range
    .globl _dcache_inval_range
    .globl _dcache_flush_range
    .globl _dcache_flush_all
    .globl _dcache_purge_range
    .globl _dcache_purge_all
    .globl _dcache_purge_all_with_buffer
    .globl _cache_write_ccr


! This routine goes through and flushes/invalidates the icache
! for a given range.
!
! r4 is starting address
! r5 is size
    .align 2
_icache_inval_range:
    tst      r5, r5          ! Test if size is 0
    mov.l    iir_addr, r0

    bt       .iinval_exit    ! Exit early if no blocks to flush

    mov.l    p2_mask, r1
    or       r1, r0
    jmp      @r0
    nop

.iinval_real:
    ! Save old SR and disable interrupts
    stc      sr, r0
    mov.l    r0, @-r15
    mov.l    ormask, r1
    or       r1, r0
    ldc      r0, sr

    ! Get ending address from size and align start address
    add      r4, r5
    mov.l    align_mask, r0
    and      r0, r4
    mov.l    ica_addr, r1
    mov.l    ic_entry_mask, r2
    mov.l    ic_valid_mask, r3

    .align 2
.iinval_loop:
    ! Invalidate I cache
    mov      r4, r6
    add      #32, r4       ! Move on to next cache block
    mov      r6, r7
    and      r2, r6        ! v & CACHE_IC_ENTRY_MASK
    cmp/hi   r4, r5
    or       r1, r6        ! CACHE_IC_ADDRESS_ARRAY | (v & CACHE_IC_ENTRY_MASK)
    and      r3, r7        ! v & 0xfffffc00
    bt/s     .iinval_loop
    mov.l    r7, @r6       ! Invalidate cache entry

    ! make sure we have enough instrs before returning to P1
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    ! Restore old SR
    mov.l    @r15+, r0
    ldc      r0, sr

.iinval_exit:
    rts
    nop

! This routine goes through and flushes/invalidates the icache 
! for a given range.
!
! r4 is starting address
! r5 is size
    .align 2
_icache_flush_range:
    tst      r5, r5          ! Test if size is 0
    mov.l    ifr_addr, r0

    bt       .iflush_exit    ! Exit early if no blocks to flush
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

    ! Get ending address from size and align start address
    add      r4, r5
    mov.l    align_mask, r0
    and      r0, r4
    mov.l    ica_addr, r1
    mov.l    ic_entry_mask, r2
    mov.l    ic_valid_mask, r3

.iflush_loop:
    ! Invalidate I cache
    mov      r4, r6
    add      #32, r4       ! Move on to next cache block
    mov      r6, r7
    and      r2, r6        ! v & CACHE_IC_ENTRY_MASK
    cmp/hi   r4, r5
    or       r1, r6        ! CACHE_IC_ADDRESS_ARRAY | (v & CACHE_IC_ENTRY_MASK)
    ocbwb    @r7           ! Write back D cache
    and      r3, r7        ! v & 0xfffffc00
    bt/s     .iinval_loop
    mov.l    r7, @r6       ! Invalidate cache entry

    ! make sure we have enough instrs before returning to P1
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    ! Restore old SR
    mov.l    @r15+, r0
    ldc      r0, sr

.iflush_exit:
    rts
    nop


! This routine goes through and invalidates the dcache for a given 
! range of RAM. Make sure that you've called dcache_flush_range first
! if you care about the contents.
!
! r4 is starting address
! r5 is size
    .align 2
_dcache_inval_range:
    tst      r5, r5         ! Test if size is 0
    mov.l    align_mask, r0   

    bt       .dinval_exit   ! Exit early if no blocks to inval
    add      r4, r5         ! Get ending address from size
    
    and      r0, r4         ! Align start address

.dinval_loop:
    ! Invalidate the dcache
    ocbi     @r4
    add      #32, r4        ! Move on to next cache block
    cmp/hi   r4, r5
    bt       .dinval_loop

.dinval_exit:
    rts
    nop


! This routine goes through and forces a write-back on the
! specified data range. Use prior to dcache_inval_range if you
! care about the contents. If the range is bigger than the dcache,
! we flush the whole cache instead.
!
! r4 is starting address
! r5 is size
    .align 2
_dcache_flush_range:
    ! Check that 0 < size < flush_check
    tst      r5, r5
    mov.l    flush_check, r2
    
    bt       .dflush_exit       ! Exit early if no blocks to flush
    mov.l    align_mask, r0

    cmp/hi   r2, r5             ! Compare with flush_check
    add      r4, r5             ! Get ending address from size
    
    bt       _dcache_flush_all  ! If size > flush_check, jump to _dcache_flush_all
    and      r0, r4             ! Align start address

.dflush_loop:
    ! Write back the dcache
    ocbwb    @r4
    add      #32, r4        ! Move on to next cache block
    cmp/hi   r4, r5
    bt       .dflush_loop

.dflush_exit:
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
! r5 is size
    .align 2
_dcache_purge_range:
    ! Check that 0 < size < purge_check  
    tst      r5, r5
    mov.l    purge_check, r2
    
    bt       .dpurge_exit       ! Exit early if no blocks to purge
    mov.l    align_mask, r0 

    cmp/hi   r2, r5             ! Compare with purge_check
    add      r4, r5             ! Get ending address from size

    bt       _dcache_purge_all  ! If size > purge_check, jump to _dcache_purge_all
    and      r0, r4             ! Align start address

.dpurge_loop:
    ! Write back and invalidate the D cache
    ocbp     @r4
    add      #32, r4     ! Move on to next cache block
    cmp/hi   r4, r5
    bt       .dpurge_loop

.dpurge_exit:
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
    cmp/hi   r4, r5
    ocbi     @r4
    bt.s    .dpurge_all_buffer_loop
    add      #32, r4        ! Move on to next cache block

    rts
    nop

_cache_write_ccr:
    mov.l    ccr_addr, r6
    mov.l    @r6, r0

    ! Clear mask
    or       r4, r0
    xor      r4, r0

    ! Set bits
    or       r5, r0

    mov      r0, r4

    !Block IRQs
    mov.l    block_bit, r0
    stc      sr, r7
    or       r7, r0
    ldc      r0, sr

    !Jump to uncached P2 area before writing CCN
    mova     1f, r0
    mov      #0xa0, r1
    shll16   r1
    shll8    r1
    or       r1, r0
    jmp      @r0
    nop

.align 2
1:
    !Flush and invalidate data cache
    mov.l    loc_tags, r0
    mov      #2, r1  ! 512 >> 8 = 2
    shll8    r1
    mov      #0, r2
1:
    mov.l    r2, @r0
    dt       r1
    add      #32, r0   ! cache_line_size is 32
    bf       1b

    !Write to CCR
    mov.l    r4, @r6

    !Can't touch cache for a while after writing CCR
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    !Restore SR (unblock IRQs)
    ldc      r7, sr
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
iir_addr:
    .long    .iinval_real

! D-cache (Data cache)
dca_addr:
    .long    0xf4000008    ! dcache array address
dc_ubit_mask:
    .long    0xfffffffd    ! Mask to zero out U bit

! _dcache_flush_range can have size param set up to 66560 bytes 
! and still be faster than dcache_flush_all.
flush_check:
    .long    66560
    
! _dcache_purge_range can have size param set up to 39936 bytes 
! and still be faster than dcache_purge_all.
purge_check:
    .long    39936 

! Shared    
p2_mask:    
    .long    0xa0000000
ormask:
    .long    0x100000f0
align_mask:
    .long    ~31           ! Align address to 32-byte boundary
ccr_addr:
    .long    0xff00001c    ! Cache control register
block_bit:
    .long    0x10000000
loc_tags:
    .long    0xf4000000
cache_lines:
    .word    512           ! Total number of cache lines in dcache
