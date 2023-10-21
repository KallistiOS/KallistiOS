! This routine was such a PIA to get working in inside the C program
! that I finally gave up and moved it out to an assembler file.

! Routine to flush parts of cache.. thanks to the Linux-SH guys
! for the algorithm. The original version of this routine was
! taken from sh-stub.c.

	.text
	.globl _icache_flush_range
	.globl _dcache_inval_range
	.globl _dcache_flush_range
	.globl _dcache_purge_range
	.globl _dcache_purge_all

! r4 is starting address
! r5 is count
_icache_flush_range:
	mov.l	fraddr,r0
	mov.l	p2mask,r1
	or	r1,r0
	jmp	@r0
	nop

	.align	2
fraddr:	.long	flush_real
p2mask:	.long	0x20000000
	

flush_real:
	! Save old SR and disable interrupts
	stc	sr,r0
	mov.l	r0,@-r15
	mov.l	ormask,r1
	or	r1,r0
	ldc	r0,sr

	! Get ending address from count and align start address
	add	r4,r5
	mov.l	l1align,r0
	and	r0,r4
	mov.l	addrarray,r1
	mov.l	entrymask,r2
	mov.l	validmask,r3

flush_loop:
	! Write back O cache
	ocbwb	@r4

	! Invalidate I cache
	mov	r4,r6		! v & CACHE_IC_ENTRY_MASK
	and	r2,r6
	or	r1,r6		! CACHE_IC_ADDRESS_ARRAY | ^

	mov	r4,r7		! v & 0xfffffc00
	and	r3,r7

	add	#32,r4		! += CPU_CACHE_BLOCK_SIZE
	cmp/hs	r4,r5
	bt/s	flush_loop
	mov.l	r7,@r6		! *addr = data	

	! Restore old SR
	mov.l	@r15+,r0
	ldc	r0,sr

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

	.align	2
ormask:
	.long	0x100000f0
addrarray:
	.long	0xf0000000	! CACHE_IC_ADDRESS_ARRAY
entrymask:
	.long	0x1fe0		! CACHE_IC_ENTRY_MASK
validmask:
	.long	0xfffffc00
	

! Goes through and invalidates the O-cache for a given block of
! RAM. Make sure that you've called dcache_flush_range first if
! you care about the contents.
! r4 is starting address
! r5 is count
_dcache_inval_range:
	! Get ending address from count and align start address
	add	r4,r5
	mov.l	l1align,r0
	and	r0,r4

dinval_loop:
	! Invalidate the O cache
	ocbi	@r4
	cmp/hs	r4,r5
	bt/s	dinval_loop
	add	#32,r4		! += CPU_CACHE_BLOCK_SIZE

	rts
	nop


! This routine just goes through and forces a write-back on the
! specified data range. Use prior to dcache_inval_range if you
! care about the contents.
! r4 is starting address
! r5 is count
_dcache_flush_range:
	! Get ending address from count and align start address
	add	r4,r5
	mov.l	l1align,r0
	and	r0,r4

dflush_loop:
	! Write back the O cache
	ocbwb	@r4
	cmp/hs	r4,r5
	bt/s	dflush_loop
	add	#32,r4		! += CPU_CACHE_BLOCK_SIZE

	rts
	nop


! This routine just goes through and forces a write-back and invalidate
! on the specified data range.
! r4 is starting address
! r5 is count
_dcache_purge_range:
	! Get ending address from count and align start address
	add	r4,r5
	mov.l	l1align,r0
	and	r0,r4

dpurge_loop:
	! Write back and invalidate the O cache
	ocbp	@r4
	cmp/hs	r4,r5
	bt/s	dpurge_loop
	add	#32,r4		! += CPU_CACHE_BLOCK_SIZE

	rts
	nop


! This routine just forces a write-back and invalidate all O cache.
! r4 is address for temporary buffer 32-byte aligned
! r5 is size of temporary buffer (8 KB or 16 KB)
_dcache_purge_all:
	mov #0, r0
	add r4, r5
dpurge_all_loop:
	! Allocate and then invalidate the O cache block
	movca.l r0, @r4
	ocbi @r4
	cmp/hs r4, r5
	bt/s dpurge_all_loop
	add #32, r4		! += CPU_CACHE_BLOCK_SIZE
	rts
	nop


	.align	2
l1align:
	.long	~31		! ~(CPU_CACHE_BLOCK_SIZE-1)
