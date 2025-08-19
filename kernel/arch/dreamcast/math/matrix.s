! Simple Matrix Math Module
!
! NOTE: The majority of these routines are assuming that the FP mode is
!       always set to single-precision upon entry. They were designed with
!       the GCC flag `-m4-single-only` in mind.
!
! This module consists of a number of matrix operations that operate on an
! "internal" matrix and potentially one "external" matrix. The internal
! matrix is represented by floating point registers and the external one by
! an array of floats.
!
! This is based on code originally by Marcus Comstedt.
!
! Some optimizations by Andrew Kieschnick
! More optimizations made by Mathieu Legris
!

! matrix arguments to _mat_store and _mat_load MUST be 8-byte aligned
! They should be 32-byte aligned for performance reasons, but they MUST be 
! 8-byte (or better) aligned to work at all

! Mathieu Legris:
! In the Hitachi doc (page 22, at the top), it is specified that the sh4 does
! not write 64 bits words in little endian mode. Instead, the two 32 bits
! words are written in big endian mode but the bytes in each of them are
! written in little endian mode:
! Register                     Memory
! [b7|b6|b5|b4|b3|b2|b1|b0] -> [b4|b5|b6|b7|b0|b1|b2|b3]
! [    l1     |    l0     ] -> [    l1     |    l0     ]
! So, we can use fmov with pair of single precision floats for storing and
! loading data in _mat_store and _mat_load.


! -The lines:
!   nop
! indicates a place where you can put a parallelizable instruction with an issue
! rate of 1.
! -The lines:
!   nop nop
! indicates a place where you can put a non parallelizable instruction with
! an issue rate of 1, or 2 parallelizables instructions each with an issue
! rate of 1.


! Important notes about _mat_transform:
!
! If you want to obtain optimal performances:
! - Align the source and destination pointer on a 32 bytes boundary.
! - The value modulo 16384 (cache size) of the destination pointer must
! be offsetted by at least 32 or 64 bytes (1 or 2 cache line, but 2
! is better) according to the value modulo 16384 of the source pointer.
!
! So, you could have:
! DestinationPointer%16384=(SourcePointer-64)%16384
! Or, if you prefer (and you should):
! DestinationPointer&0x00003fff=(SourcePointer-64)%&0x00003fff
!
! BUT YOU MUST NOT HAVE:
! DestinationPointer&0x00003fff=(SourcePointer+32)%&0x00003fff
! OR:
! DestinationPointer&0x00003fff=(SourcePointer+64)%&0x00003fff
! OTHERWISE CACHE TRASHING WILL OCCUR AND THE PERFORMANCE WILL BE DIVIDED
! BY 2 OR 3, MAYBE MORE.
!
! This does not occur if DestinationPointer and SourcePointer are not
! distant of more than 16K (the cache size).
!
! Cache trashing is when an access in a memory area (16k for the sh4)
! is done, before another access in another memory area is made at the
! same offset.
! Example:
! r0=x and r1=16384+x, with 0<=x<32 (cache line size).
!   mov     @r0,r2
!   mov     @r1,r3 ! Cache trashing !!!!!
! Or:
!   mov     #1,r3
!   mov     @r0,r2 ! Read a memory word. The corresponding cache line is filled
!   mov     r3,@r1 ! The same cache line is filled again for writeback.
!   mov     @r0,r4 ! Cache trashing: the cache line need to be filled again.
! With this 3 lines, the same cache line is filled 3 times with
! the corresponding memory penalties (high with the SH4).


! Copy the internal matrix out to a memory one
.globl _mat_store
! Number of cycles: ~12 cycles.
_mat_store:
    fschg
    add         #4*16, r4       ! Start at the end

    fmov        xd14, @-r4
    fmov        xd12, @-r4
    fmov        xd10, @-r4
    fmov        xd8, @-r4
    fmov        xd6, @-r4
    fmov        xd4, @-r4
    fmov        xd2, @-r4
    fmov        xd0, @-r4

    rts
    fschg

! Copy a memory matrix into the internal one
.globl _mat_load
! Number of cycles: ~11 cycles.
_mat_load:
    fschg
    fmov        @r4+, xd0

    fmov        @r4+, xd2
    fmov        @r4+, xd4
    fmov        @r4+, xd6
    fmov        @r4+, xd8
    fmov        @r4+, xd10
    fmov        @r4+, xd12
    fmov        @r4+, xd14

    rts
    fschg

! Clear internal to an identity matrix
! Number of cycles: ~17 cycles.
.globl _mat_identity
_mat_identity:
! Load up identity numbers as 'doubles'
    fldi0       fr0         ! Entirely zero.
    fldi0       fr1
    fldi1       fr2         ! First float is one.
    fldi0       fr3
    fldi0       fr4

    fldi1       fr5         ! Second float is one.
    fschg

    fmov        dr2, xd0    ! Setup matrix.
    fmov        dr0, xd2
    fmov        dr4, xd4
    fmov        dr0, xd6
    fmov        dr0, xd8
    fmov        dr2, xd10
    fmov        dr0, xd12
    fmov        dr4, xd14

    rts
    fschg

! "Apply" a matrix: multiply a matrix onto the "internal" one.
! Number of cycles: ~32.
.globl _mat_apply
_mat_apply:
        fmov.s          fr15, @-r15
        fmov.s          fr14, @-r15
        fmov.s          fr13, @-r15
        fmov.s          fr12, @-r15
    fschg
!   nop

    fmov        @r4+,dr0 ! Load up first row.
!   nop
    fmov        @r4+, dr2
!   nop
    fmov        @r4+, dr4 ! Load up second row.
!   nop
    fmov        @r4+, dr6
!   nop

    ftrv        xmtrx, fv0
!   nop

    fmov        @r4+, dr8
!   nop
    fmov        @r4+, dr10
!   nop

!   nop nop

    ftrv        xmtrx, fv4
!   nop

    fmov        @r4+, dr12
!   nop
    fmov        @r4+, dr14
!   nop

!   nop nop

    ftrv        xmtrx, fv8
!   nop

!   nop nop
!   nop nop
!   nop nop

    ftrv        xmtrx, fv12
!   nop

    fmov        dr0, xd0    ! Copy the resulting matrix back to internal.
!   nop
    fmov        dr2, xd2
!   nop
    fmov        dr4, xd4
!   nop
    fmov        dr6, xd6
!   nop
    fmov        dr8, xd8
!   nop
    fmov        dr10, xd10
!   nop
    fmov        dr12, xd12
!   nop
    fmov        dr14, xd14
!   nop

    fschg
        fmov.s      @r15+, fr12
        fmov.s      @r15+, fr13
        fmov.s      @r15+, fr14
        rts
        fmov.s      @r15+, fr15

.globl _mat_multiply
_mat_multiply:
    fmov.s          fr15, @-r15
    fmov.s          fr14, @-r15
    fmov.s          fr13, @-r15
    fmov.s          fr12, @-r15
    fschg
!   nop

    fmov        @r5+,dr0 ! Load up first row.
!   nop
    fmov        @r5+, dr2
!   nop
    fmov        @r5+, dr4 ! Load up second row.
!   nop
    fmov        @r5+, dr6
!   nop

    ftrv        xmtrx, fv0
!   nop

    fmov        @r5+, dr8
!   nop
    fmov        @r5+, dr10
!   nop

!   nop nop

    ftrv        xmtrx, fv4
!   nop

    fmov        @r5+, dr12
!   nop
    fmov        @r5+, dr14
!   nop

!   nop nop

    ftrv        xmtrx, fv8
!   nop

!   nop nop
!   nop nop
!   nop nop

    ftrv        xmtrx, fv12
!   nop

    fmov        dr0, @r4
    add         #8, r4
    fmov        dr2, @r4
    add         #8, r4
    fmov        dr4, @r4
    add         #8, r4
    fmov        dr6, @r4
    add         #8, r4
    fmov        dr8, @r4
    add         #8, r4
    fmov        dr10, @r4
    add         #8, r4
    fmov        dr12, @r4
    add         #8, r4
    fmov        dr14, @r4
!   nop

    fschg
    fmov.s      @r15+, fr12
    fmov.s      @r15+, fr13
    fmov.s      @r15+, fr14
    rts
    fmov.s      @r15+, fr15


! Transform zero or more sets of vectors using the current internal
! matrix. Each vector is three floats long.
! Number of cycles in the loop in the best case: ~38 cycles.
! Number of vertices per second: ~15,000,000
! Minimum number of vertices: 1.
!
! r4: Input vectors
! r5: Output vectors
! r6: Number of vectors
! r7: Vector stride (bytes between vectors)
!
.globl _mat_transform
_mat_transform:
    ! Save registers and setup pointers.
    pref        @r4
    mov         r7, r2          ! r2=Stride.
    fmov        fr12, @-r15
    add         #-8, r2         ! r2=Stride-12=read stride.
    fmov        fr13, @-r15
    mov         r7, r3          ! r3=Stride.
    fmov        fr14, @-r15
    add         #12, r3         ! r3=Stride+12=write stride.

    ! Load the first vertex.
    fmov        @r4+, fr0
    add         #12, r5         ! End of the first destination vector.
    fmov        @r4+, fr1
!   nop
    fmov        @r4, fr2
    add         r2, r4
    fldi1       fr3
    dt          r6

    pref        @r4
    ftrv        xmtrx, fv0
    bt          .only1Vertex
    fldi1       fr12

    ! Load the second vertex.
    fmov        @r4+, fr4
!   nop
    fmov        @r4+, fr5
!   nop
    fmov        @r4, fr6
    add         r2, r4
    fldi1       fr7
!   nop

    pref        @r4
    fdiv        fr3, fr12
    dt          r6
    ftrv        xmtrx, fv4
    bt          .loopEnd

.loop:
    pref        @r5
!   nop

    fldi1       fr13
!   nop

    ! Load a vector.
    fmov        @r4+, fr8
!   nop
    fmov        @r4+, fr9
!   nop
    fmov        @r4, fr10
    add         r2, r4
    fldi1       fr11
!   nop

    pref        @r4
    fdiv        fr7, fr13
    dt          r6
    fmul        fr12, fr1
    fmul        fr12, fr0
!   nop

    ! Store a vector.
    ftrv        xmtrx, fv8
    fmov        fr12, @-r5
    fmov        fr1, @-r5
!   nop
    fmov        fr0, @-r5
    bt/s        .firstInLoop
    add         r3, r5
    pref        @r5

    fldi1       fr14
!   nop

    ! Load a vector.
    fmov        @r4+, fr0
!   nop
    fmov        @r4+, fr1
!   nop
    fmov        @r4, fr2
    add         r2, r4
    fldi1       fr3
!   nop

    pref        @r4
    fdiv        fr11, fr14
    dt          r6
    fmul        fr13, fr5
    fmul        fr13, fr4
!   nop

    ! Store a vector.
    ftrv        xmtrx, fv0
    fmov        fr13, @-r5
    fmov        fr5, @-r5
!   nop
    fmov        fr4, @-r5
    bt/s        .secondInLoop
     add        r3, r5
    pref        @r5

    fldi1       fr12
!   nop

    ! Load a vector.
    fmov        @r4+, fr4
!   nop
    fmov        @r4+, fr5
!   nop
    fmov        @r4, fr6
    add         r2, r4
    fldi1       fr7
!   nop

    pref        @r4
    fdiv        fr3, fr12
    dt          r6
    fmul        fr14, fr9
    fmul        fr14, fr8
!   nop

    ! Store a vector.
    ftrv        xmtrx, fv4
    fmov        fr14, @-r5
    fmov        fr9, @-r5
!   nop
    fmov        fr8, @-r5
    bf/s        .loop
    add         r3, r5

.loopEnd:
    pref        @r5

    fldi1       fr13
    fdiv        fr7, fr13

    fmul        fr12, fr1
!   nop
    fmul        fr12, fr0
    fmov        fr12, @-r5
!   nop nop
!   nop nop
    fmov        fr1, @-r5
!   nop
    fmov        fr0, @-r5
!   nop

!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop

    fmov        @r15+, fr14
    fmul        fr13, fr5
    add         r3, r5
    fmul        fr13, fr4
    fmov        fr13, @-r5
!   nop
    fmov        @r15+, fr13
!   nop
    fmov        @r15+, fr12
!   nop
    fmov        fr5, @-r5
    rts
     fmov       fr4, @-r5


.only1Vertex:
    fldi1       fr12
    fdiv        fr3, fr12
    fmov        @r15+, fr14
!   nop
    fmov        @r15+, fr13
!   nop

!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop

    fmul        fr12, fr1
!   nop
    fmul        fr12, fr0
    fmov        fr12, @-r5
    fmov        @r15+, fr12
!   nop

!   nop nop

    fmov        fr1, @-r5
    rts
    fmov        fr0, @-r5


.firstInLoop:
    pref        @r5

    fldi1       fr14
    fdiv        fr11, fr14

    fmul        fr13, fr5
!   nop
    fmul        fr13, fr4
    fmov        fr13, @-r5
    fmov        fr5, @-r5
!   nop

!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop

    fmov        fr4, @-r5
    fmul        fr14, fr9
    add         r3, r5
    fmul        fr14, fr8
    fmov        fr14, @-r5
!   nop
    fmov        fr9, @-r5
!   nop
    fmov        fr8, @-r5
!   nop
    fmov        @r15+, fr14
!   nop
    fmov        @r15+, fr13
    rts
    fmov        @r15+, fr12


.secondInLoop:
    pref        @r5

    fldi1       fr12
    fdiv        fr3, fr12

    fmul        fr14, fr9
!   nop
    fmul        fr14, fr8
    fmov        fr14, @-r5

!   nop nop
!   nop nop

    fmov        fr9, @-r5
!   nop
    fmov        fr8, @-r5
!   nop
    fmov        @r15+, fr14
!   nop

!   nop nop
!   nop nop
!   nop nop
!   nop nop
!   nop nop

    fmov        @r15+, fr13
    fmul        fr12, fr1
    add         r3, r5
    fmul        fr12, fr0
    fmov        fr12, @-r5
!   nop
    fmov        @r15+, fr12
!   nop
    fmov        fr1, @-r5
    rts
    fmov        fr0, @-r5


! Transform zero or more sets of vertices using the current internal
! matrix, directly to the store queues. Each vertex is 32 bytes long.
! All non-xyz data will be copied over along with the transformed
! coordinates. Minimum number of vertices: 1.
!
! r4: Input vectors
! r5: Output (should be a store queue address)
! r6: Number of vectors
!
! Note that the QACRx registers must have already been set.
!
! This was contributed by Jim Ursetto.
!
.globl _mat_transform_sq
_mat_transform_sq:

    pref @r4
!   nop

    fldi1       fr9
    add         #4, r4      ! skip over flags

.loop1:
    ! Load a vector.

    fmov        @r4+, fr0   ! x
    add         #32, r5     ! end of destination sq
    fmov        @r4+, fr1   ! y
!   nop
    fmov        @r4+, fr2   ! z
    ! nop
    fldi1       fr3         ! w (1)
    ! nop
    ! nop nop
    ! nop nop               ! 2 cycle stall: fldi1->ftrv dependency

    ftrv        xmtrx, fv0
    fmov        @r4+, fr4   ! u
    fmov        @r4+, fr5   ! v
!   nop
    fmov        @r4+, fr6   ! argb
!   nop
    fmov        @r4+, fr7   ! oargb
!   nop
!   nop nop

    fdiv        fr3, fr9
    add         #-32, r4

    ! store a vector

    fmov        @r4,fr8     ! flags
    add         #32,r4
    fmov        fr7, @-r5   ! oargb
    fmov        fr6, @-r5   ! argb
    fmov        fr5, @-r5   ! v
    fmov        fr4, @-r5   ! u

    pref        @r4
    add         #4, r4
    dt          r6
    ! nop
    ! 5x nop nop

    fmul        fr9, fr1
    fmul        fr9, fr0
!   nop

    fmov        fr9, @-r5   ! z (1/w)
    fmov        fr1, @-r5   ! y
    fmov        fr0, @-r5   ! x
    fmov        fr8, @-r5   ! flags
    pref        @r5
    add         #32, r5

    bf/s        .loop1
    fldi1       fr9

    rts
    nop

