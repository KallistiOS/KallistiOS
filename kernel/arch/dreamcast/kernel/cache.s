
! KallistiOS ##version##
!
! arch/dreamcast/kernel/cache.s
!
! Copyright (C) 2001 Megan Potter
! Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev
! Copyright (C) 2023, 2024 Andy Barajas
! Copyright (C) 2024 Paul Cercueil
!
! Optimized assembler code for managing the cache.
!

    .text
    .globl _arch_icache_inval_range
    .globl _arch_icache_sync_range

! This routine goes through and flushes/invalidates the icache
! for a given range.
!
! r4 is starting address
! r5 is size
    .align 2
_arch_icache_inval_range:
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
_arch_icache_sync_range:
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
    bt/s     .iflush_loop
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
p2_mask:
    .long    0xa0000000
ormask:
    .long    0x100000f0
align_mask:
    .long    ~31           ! Align address to 32-byte boundary

