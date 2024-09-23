! KallistiOS ##version##
!
! arch/dreamcast/hardware/sq_fast_cpy.s
! Copyright (C) 2024 Andy Barajas
!
! Optimized SH4 assembler function for copying 32 bytes of data 
! (8 bytes at a time) using pair single-precision data transfer
! specifically for store queues.
!

.globl _sq_fast_cpy

!
! void *sq_fast_cpy(uint32_t *dest, uint32_t *src, size_t n);
!
! r4: dest (should be 32-byte aligned store queue address)
! r5: src (should be 8-byte aligned address)
! r6: n (how many 32-byte blocks of data you want to copy)
!
    .align 2
_sq_fast_cpy:
    fschg              ! Change to pair single-precision data
    tst    r6, r6
    bt/s   .exit       ! Exit if size is 0
    mov    r4, r0
1:
    fmov.d @r5+, dr0
    mov    r4, r1 
    fmov.d @r5+, dr2
    add    #32, r1 
    fmov.d @r5+, dr4
    fmov.d @r5+, dr6
    pref   @r5         ! Prefetch 32 bytes for next loop
    dt     r6          ! while(n--)
    fmov.d dr6, @-r1
    fmov.d dr4, @-r1
    fmov.d dr2, @-r1
    fmov.d dr0, @-r1
    add    #32, r4
    bf.s   1b
    pref   @r1         ! Fire off store queue

.exit:
    rts     
    fschg
