/* KallistiOS ##version##

   arch/dreamcast/kernel/tls.c
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2025 Donald Haase
*/

/* Functions to initialize and manage TLS data. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <arch/tls_static.h>

/* TLS Section ELF data - exported from linker script. */
extern int _tdata_start, _tdata_size;
extern int _tbss_size;
extern long _tdata_align, _tbss_align;

/* Utility function for aligning an address or offset. */
static inline size_t align_to(size_t address, size_t alignment) {
    return (address + (alignment - 1)) & ~(alignment - 1);
}

/*  Thread Control Block Header

    Header preceding the static TLS data segments as defined by
    the SH-ELF TLS ABI (version 1). This is what the thread pointer
    (GBR) points to for compiler access to thread-local data.
*/
typedef struct tcbhead {
    void *dtv;               /* Dynamic TLS vector (unused) */
    uintptr_t pointer_guard; /* Pointer guard (unused) */
} tcbhead_t;

void arch_tls_init(void) {
    /* Initialize GBR register for Main Thread */
    __builtin_set_thread_pointer((void*)(thd_get_current()->context.gbr));
}

/* Creates and initializes the static TLS segment for a thread,
   composed of a Thread Control Block (TCB), followed by .TDATA,
   followed by .TBSS, very carefully ensuring alignment of each
   subchunk.
*/
bool arch_tls_setup_data(kthread_t *thd) {
    size_t align, tdata_offset, tdata_end, tbss_offset, 
        tbss_end, align_rem, tls_size;

    tcbhead_t *tcbhead;
    void *tdata_segment, *tbss_segment;

    /* Cached and typed local copies of TLS segment data for sizes,
       alignments, and initial value data pointer, exported by the
       linker script.

       SIZES MUST BE VOLATILE or the optimizer on non-debug builds will
       optimize zero-check conditionals away, since why would the 
       address of a variable be NULL? (Linker script magic, it can be.)
   */
    const volatile size_t   tdata_size  = (size_t)(&_tdata_size);
    const volatile size_t   tbss_size   = (size_t)(&_tbss_size);
    const          size_t   tdata_align = tdata_size ? (size_t)_tdata_align : 1;
    const          size_t   tbss_align  = tbss_size ? (size_t)_tbss_align : 1;
    const          uint8_t *tdata_start = (const uint8_t *)(&_tdata_start);

    /* Each subsegment of the requested memory chunk must be aligned
       by the largest segment's memory alignment requirements.
   */
    align = 8;               /* tcbhead_t has to be aligned by 8. */
    if(tdata_align > align)
        align = tdata_align; /* .TDATA segment's alignment */
    if(tbss_align > align)
        align = tbss_align;  /* .TBSS segment's alignment */

    /* Calculate the sizing and offset location of each subsegment. */
    tdata_offset = align_to(sizeof(tcbhead_t), align);
    tdata_end    = tdata_offset + tdata_size;
    tbss_offset  = align_to(tdata_end, tbss_align);
    tbss_end     = tbss_offset + tbss_size; 

    /* Calculate final aligned size requirement. */
    align_rem = tbss_end % align;
    tls_size  = tbss_end;

    if(align_rem)
        tls_size += (align - align_rem);

    /* Allocate combined chunk with calculated size and alignment. */
    tcbhead = aligned_alloc(align, tls_size);

    if(!tcbhead)
        return false;

    assert(!((uintptr_t)tcbhead % 8));

    /* Since we aren't using either member within it, zero out tcbhead. */
    memset(tcbhead, 0, sizeof(tcbhead_t));

    /* Initialize .TDATA */
    if(tdata_size) {
        tdata_segment = (uint8_t *)tcbhead + tdata_offset;

        /* Verify proper alignment. */
        assert(!((uintptr_t)tdata_segment % tdata_align));

        /* Initialize tdata_segment with .tdata bytes from ELF. */
        memcpy(tdata_segment, tdata_start, tdata_size);
    }

    /* Initialize .TBSS */
    if(tbss_size) {
        tbss_segment = (uint8_t *)tcbhead + tbss_offset;

        /* Verify proper alignment. */
        assert(!((uintptr_t)tbss_segment % tbss_align));

        /* Zero-initialize tbss_segment. */
        memset(tbss_segment, 0, tbss_size);
    }

    /* Set Thread Pointer and store starting value. */
    thd->context.gbr = (uint32_t)tcbhead;
    thd->tls_hnd = (void *)tcbhead;

    return true;
}

void arch_tls_destroy_data(kthread_t *thd) {
    free(thd->tls_hnd);
}
