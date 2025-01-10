/* KallistiOS ##version##

   arch/dreamcast/kernel/mmu.c
   (c)2001 Megan Potter
*/

/* SH-4 MMU related functions, ported up from KOS-MMU */

#include <string.h>
#include <malloc.h>

#include <kos/thread.h>
#include <arch/arch.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <arch/memory.h>
#include <arch/mmu.h>
#include <kos/dbgio.h>
#include <kos/regfield.h>
#include <arch/cache.h>

#define MMU_TOP_MASK GENMASK(30, 21)            /**< \brief Top-level mask */
#define MMU_BOT_MASK GENMASK(20, 12)            /**< \brief Bottom mask */
#define MMU_IND_BITS 12                         /**< \brief Index bits */

/********************************************************************************/
/* Register definitions */

static volatile uint32_t * const pteh = (uint32_t *)(SH4_REG_MMU_PTEH);
static volatile uint32_t * const ptel = (uint32_t *)(SH4_REG_MMU_PTEL);
//static volatile uint32_t * const ptea = (uint32_t *)(SH4_REG_MMU_PTEA);
static volatile uint32_t * const ttb = (uint32_t *)(SH4_REG_MMU_TTB);
static volatile uint32_t * const tea = (uint32_t *)(SH4_REG_MMU_TEA);
static volatile uint32_t * const mmucr = (uint32_t *)(SH4_REG_MMU_CR);

#define BUILD_PTEH(VA, ASID) \
    ( ((VA) & 0xfffffc00) | ((ASID) & 0xff) )

#define SET_PTEH(VA, ASID) \
    do { *pteh = BUILD_PTEH(VA, ASID); } while(0)

#define BUILD_PTEL(PA, V, SZ, PR, C, D, SH, WT) \
    ( ((PA) & 0x1ffffc00) | ((V) << 8) \
      | ( ((SZ) & 2) << 6 ) | ( ((SZ) & 1) << 4 ) \
      | ( (PR) << 5 ) \
      | ( (C) << 3 ) \
      | ( (D) << 2 ) \
      | ( (SH) << 1 ) \
      | ( (WT) << 0 ) )

#define SET_TTB(TTB) \
    do { *ttb = TTB; } while(0)

#define SET_MMUCR(URB, URC, SQMD, SV, TI, AT) \
    do { *mmucr = ((URB) << 18) \
                      | ((URC) << 10) \
                      | ((SQMD) << 9) \
                      | ((SV) << 8) \
                      | ((TI) << 2) \
                      | ((AT) << 0); } while(0)

#define SET_URC(URC) \
    do { *mmucr = (*mmucr & ~(63 << 10)) \
                      | (((URC) & 63) << 10); } while(0)

#define GET_URC() ((*mmucr >> 10) & 63)

#define INCR_URC() \
    do { SET_URC(GET_URC() + 1); } while(0)

/********************************************************************************/

/* "Current" page tables (for TLB exception handling) */
mmucontext_t *mmu_cxt_current = NULL;

/* This value will be non-zero if we can safely shortcut the standard tlb-miss
   exception handling. */
int mmu_shortcut_ok = 0;

/* The last URC value we used */
static int last_urc;

/* Our TLB mapping function */
static mmu_mapfunc_t map_func;


/********************************************************************************/
/* Physical hardware management */

static inline void mmu_ldtlb_quick(uint32_t ptehv, uint32_t ptelv) {
    *pteh = ptehv;
    *ptel = ptelv;
    __asm__("ldtlb");
}

static inline void mmu_ldtlb(int asid, uint32_t virt, uint32_t phys, int sz, int pr, int c, int d,
                             int sh, int wt) {
    mmu_ldtlb_quick(BUILD_PTEH(virt, asid), BUILD_PTEL(phys, 1, sz, pr, c, d, sh, wt));
}

static inline void mmu_ldtlb_wait(void) {
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
}

/* Defined in mmuitlb.s */
void mmu_reset_itlb(void);

/* Defined below */
static mmupage_t *map_virt(mmucontext_t *context, int virtpage);

/********************************************************************************/
/* Table management */

/* Set the "current" page tables for TLB handling */
void mmu_use_table(mmucontext_t *context) {
    mmu_cxt_current = context;

    if(mmu_cxt_current && map_func == map_virt)
        mmu_shortcut_ok = 1;
    else
        mmu_shortcut_ok = 0;
}

/* Allocate a page table shell; no actual sub-contexts will be allocated
   until a mapping is performed. */
mmucontext_t *mmu_context_create(int asid) {
    mmucontext_t    *cont;
    int     i;

    cont = (mmucontext_t*)malloc(sizeof(mmucontext_t));

    if(cont == NULL)
        return NULL;

    cont->asid = asid;

    for(i = 0; i < MMU_PAGES; i++)
        cont->sub[i] = NULL;

    return cont;
}

/* Destroy an MMU context when a process is being destroyed. */
void mmu_context_destroy(mmucontext_t *context) {
    int i;

    for(i = 0; i < MMU_PAGES; i++) {
        if(context->sub[i] != NULL)
            free(context->sub[i]);
    }

    free(context);
}

/* Using the given page tables, return a pointer to the page entry
   matching the given virtual page ID, or return NULL if there
   isn't one. */
static mmupage_t *map_virt(mmucontext_t *context, int virtpage) {
    mmusubcontext_t *sub;
    mmupage_t   *page;
    int     top, bot;

    /* Get back the virtual address */
    virtpage = virtpage << MMU_IND_BITS;

    /* Mask out and grab the top and bottom indices */
    top = FIELD_GET(virtpage, MMU_TOP_MASK);
    bot = FIELD_GET(virtpage, MMU_BOT_MASK);

    /* Look up the top-level sub-context */
    sub = context->sub[top];

    if(sub == NULL)
        return NULL;

    /* Look up the bottom-level page */
    page = sub->page + bot;

    if(!page->valid)
        return NULL;

    /* Return the physical page number */
    return page;
}

/* Using the given page tables, translate the virtual page ID to a
   physical page ID. Return -1 on failure. */
int mmu_virt_to_phys(mmucontext_t *context, int virtpage) {
    mmupage_t   *page;

    page = map_virt(context, virtpage);

    if(!page)
        return -1;
    else
        return page->physical;
}

/* Switch to the given context; invalidate any caches as necessary */
void mmu_switch_context(mmucontext_t *context) {
    SET_PTEH(0, context->asid);
}

/* Set the given virtual page to map to the given physical page; implies
   turning on the "valid" bit. */
static void mmu_page_map_single(mmucontext_t *context,
                                int virtpage, int physpage,
                                page_prot_t prot, page_cache_t cache,
                                bool share, bool dirty) {
    mmusubcontext_t *sub;
    mmupage_t   *page;
    int     top, bot, i;

    (void)dirty;

    /* Get back the virtual address */
    virtpage = virtpage << MMU_IND_BITS;

    /* Mask out and grab the top and bottom indices */
    top = FIELD_GET(virtpage, MMU_TOP_MASK);
    bot = FIELD_GET(virtpage, MMU_BOT_MASK);

    /* Look up the top-level sub-context; if there isn't one, create one. */
    sub = context->sub[top];

    if(sub == NULL) {
        sub = (mmusubcontext_t *)malloc(sizeof(mmusubcontext_t));

        for(i = 0; i < MMU_SUB_PAGES; i++)
            sub->page[i].valid = 0;

        context->sub[top] = sub;
    }

    /* Look up the bottom-level page */
    page = sub->page + bot;

    /* XXX Invalidate ITLB if necessary when page->valid == 1 */
    page->physical = physpage;
    page->prkey = prot;

    switch(cache) {
        case MMU_NO_CACHE:
            page->cache = 0;
            break;
        case MMU_CACHE_BACK:
            page->cache = 0;
            page->wthru = 0;
            break;    /* XXX tmp */
        case MMU_CACHE_WT:
            page->cache = 0;
            page->wthru = 1;
            break;
        default:
            page->cache = 0;
            page->wthru = 0;
            break;
    }

    page->dirty = 1;    /* XXX Initial-write exception not called */
    page->blank = 0;
    page->shared = share;
    page->valid = 1;

    page->pteh = BUILD_PTEH(virtpage, 0);
    page->ptel = BUILD_PTEL(page->physical << PAGESIZE_BITS, 1, 1, page->prkey,
                            page->cache, page->dirty, page->shared, page->wthru);
}

/* Map N pages sequentially */
void mmu_page_map(mmucontext_t *context,
                  int virtpage, int physpage, int count,
                  page_prot_t prot, page_cache_t cache,
                  bool share, bool dirty) {
    while(count > 0) {
        mmu_page_map_single(context,
                            virtpage, physpage,
                            prot, cache, share, dirty);
        virtpage++;
        physpage++;
        count--;
    }
}

#if 0   /* Only applies to KOS-MMU */
/* Syscall version of mmu_page_map; all parameters are adjusted to
   even page boundaries; if src is NULL, anonymous pages are mapped
   (allocated from the heap pool); if src is non-NULL, the address
   is considered to be a physical address. Use munmap to free them. */
void sc_mmu_mmap(uint32_t dst, size_t len, uint32_t src) {
    int anon = 0;

    /* Adjust length to page boundary */
    if(len & PAGEMASK)
        len = (len & ~PAGEMASK) + PAGESIZE;

    len >>= PAGESIZE_BITS;

    /* If no src pointer, then allocate anonymous pages */
    if(!src) {
        src = (uint32_t)mm_palloc(len, proc_current->pid);

        if(src == 0)
            RETURN(0);

        anon = 1;
    }

    /* Do the actual mapping */
    /*dbgio_printf("sc: mmu_page_map(%08x,%08x,%08x,%08x,%d,%d,%d,%d)\n",
        proc_current->pt, dst >> PAGESIZE_BITS, src >> PAGESIZE_BITS, len,
        MMU_ALL_RDWR, anon ? MMU_CACHEABLE : MMU_NO_CACHE, MMU_SHARED, MMU_DIRTY); */
    mmu_page_map(proc_current->pt,
                 dst >> PAGESIZE_BITS, src >> PAGESIZE_BITS, len,
                 MMU_ALL_RDWR,
                 anon ? MMU_CACHEABLE : MMU_NO_CACHE,
                 MMU_NOT_SHARED,
                 MMU_DIRTY);
    RETURN(dst);
}
#endif /* 0 */

/* Copy a chunk of data from a process' address space into a
   kernel buffer, taking into account page mappings.

   This routine is pretty nasty.. this is completely platform
   generic but should probably be replaced by a nice assembly
   routine for each platform as appropriate. */
int mmu_copyin(mmucontext_t *context, uint32_t srcaddr, uint32_t srccnt, void *buffer) {
    mmupage_t *srcpage;
    uint32_t srcptr;
    uint32_t src, run;
    int copied, srckrn;
    uint8_t *dst;

    /* Setup source pointers */
    srcptr = (uint32_t)srcaddr;

    if(!(srcptr & 0x8000000)) {
        srcpage = map_virt(context, srcptr >> PAGESIZE_BITS);

        if(srcpage == NULL)
            arch_panic("mmu_copyv with invalid source page");

        src = (srcpage->physical << PAGESIZE_BITS) | (srcptr & PAGEMASK);
        srckrn = 0;
    }
    else {
        src = srcptr;
        srckrn = 1;
    }

    /* Setup destination pointers */
    dst = (uint8_t*)buffer;

    /* Do the actual copy */
    copied = 0;

    while(srccnt > 0) {
        /* Determine the largest run we can get away with */

        /* What's left of source page */
        run = PAGESIZE - (srcptr & PAGEMASK);

        /* What's left of source count */
        if(srccnt < run)
            run = srccnt;

        /* Do the segment copy */
        memcpy(dst, (void*)(src | 0x80000000), run);

        /* Adjust all the pointers */
        src += run;
        srcptr += run;
        dst += run;

        /* Check for overruns */
        srccnt -= run;

        if(!srckrn && (srcptr & ~PAGEMASK) != ((srcptr - run) & ~PAGEMASK)) {
            srcpage = map_virt(context, srcptr >> PAGESIZE_BITS);

            if(srcpage == NULL)
                arch_panic("mmu_copyv with invalid source page (in loop)");

            src = (srcpage->physical << PAGESIZE_BITS)
                  | (srcptr - (srcptr & ~PAGEMASK));
        }

        copied += run;
    }

    return copied;
}

/* Copy a chunk of data from one process' address space to another
   process' address space, taking into account page mappings.

   This routine is pretty nasty.. this is completely platform
   generic but should probably be replaced by a nice assembly
   routine for each platform as appropriate. */
int mmu_copyv(mmucontext_t *context1, struct iovec *iov1, int iovcnt1,
              mmucontext_t *context2, struct iovec *iov2, int iovcnt2) {
    mmupage_t *srcpage, *dstpage;
    int srciov, dstiov;
    uint32_t srccnt, dstcnt;
    uint32_t srcptr, dstptr;
    uint32_t src, dst, run;
    int copied;
    int srckrn, dstkrn;
    /* static int   sproket = 0; */

    /* timer_disable_primary();
    irq_enable(); */

    /* Setup source pointers */
    srciov = 0;
    srccnt = iov1[srciov].iov_len;
    srcptr = (uint32_t)iov1[srciov].iov_base;

    if(!(srcptr & 0x80000000)) {
        srcpage = map_virt(context1, srcptr >> PAGESIZE_BITS);

        if(srcpage == NULL)
            arch_panic("mmu_copyv with invalid source page");

        src = (srcpage->physical << PAGESIZE_BITS) | (srcptr & PAGEMASK);
        srckrn = 0;
    }
    else {
        src = srcptr;
        srckrn = 1;
    }

    /* Setup destination pointers */
    dstiov = 0;
    dstcnt = iov2[dstiov].iov_len;
    dstptr = (uint32_t)iov2[dstiov].iov_base;

    if(!(dstptr & 0x80000000)) {
        dstpage = map_virt(context2, dstptr >> PAGESIZE_BITS);

        if(dstpage == NULL)
            arch_panic("mmu_copyv with invalid destination page");

        dst = (dstpage->physical << PAGESIZE_BITS) | (dstptr & PAGEMASK);
        dstkrn = 0;
    }
    else {
        dst = dstptr;
        dstkrn = 1;
    }

    /* Do the actual copy */
    copied = 0;

    while(srciov < iovcnt1 && dstiov < iovcnt2) {
        /* Determine the largest run we can get away with */

        /* What's left of source page */
        run = PAGESIZE - (srcptr & PAGEMASK);

        /* What's left of destination page */
        if((PAGESIZE - (dstptr & PAGEMASK)) < run)
            run = PAGESIZE - (dstptr & PAGEMASK);

        /* What's left of source iov */
        if(srccnt < run)
            run = srccnt;

        /* What's left of dest iov */
        if(dstcnt < run)
            run = dstcnt;

        /* Do the segment copy */
        /* if(!sproket) {
            dbgio_printf("Copying %08lx -> %08lx (%08lx -> %08lx), %d bytes\n",
                srcptr, dstptr, src, dst, run);
            dbgio_flush();
            dbgio_flush();
            dbgio_flush();
            dbgio_flush();
            sproket = 1;
        } */
        //debug();
        memcpy((void*)(dst | 0xa0000000), (void*)(src | 0x80000000), run);
        /* dcache_inval_range(dstptr, run); */
        dcache_inval_range(dst | 0x80000000, run);
        //undebug();

        /* Adjust all the pointers */
        src += run;
        srcptr += run;
        dst += run;
        dstptr += run;
        copied += run;

        /* Check for overruns */
        srccnt -= run;

        if(srccnt <= 0) {
            srciov++;

            if(srciov >= iovcnt1) break;

            srccnt = iov1[srciov].iov_len;
            srcptr = (uint32_t)iov1[srciov].iov_base;

            if(!srckrn) {
                srcpage = map_virt(context1, srcptr >> PAGESIZE_BITS);

                if(srcpage == NULL)
                    arch_panic("mmu_copyv with invalid source page (in loop)");

                src = (srcpage->physical << PAGESIZE_BITS) | (srcptr & PAGEMASK);
            }
            else {
                src = srcptr;
            }
        }
        else {
            if(!srckrn && (srcptr & ~PAGEMASK) != ((srcptr - run) & ~PAGEMASK)) {
                srcpage = map_virt(context1, srcptr >> PAGESIZE_BITS);

                if(srcpage == NULL)
                    arch_panic("mmu_copyv with invalid source page (in loop)");

                src = (srcpage->physical << PAGESIZE_BITS)
                      | (srcptr - (srcptr & ~PAGEMASK));
            }
        }

        dstcnt -= run;

        if(dstcnt <= 0) {
            dstiov++;

            if(dstiov >= iovcnt2) break;

            dstcnt = iov2[dstiov].iov_len;
            dstptr = (uint32_t)iov2[dstiov].iov_base;

            if(!dstkrn) {
                dstpage = map_virt(context2, dstptr >> PAGESIZE_BITS);

                if(dstpage == NULL)
                    arch_panic("mmu_copyv with invalid destination page (in loop)");

                dst = (dstpage->physical << PAGESIZE_BITS) | (dstptr & PAGEMASK);
            }
            else {
                dst = dstptr;
            }
        }
        else {
            if(!dstkrn && (dstptr & ~PAGEMASK) != ((dstptr - run) & ~PAGEMASK)) {
                dstpage = map_virt(context2, dstptr >> PAGESIZE_BITS);

                if(dstpage == NULL)
                    arch_panic("mmu_copyv with invalid destination page (in loop)");

                dst = (dstpage->physical << PAGESIZE_BITS)
                      | (dstptr - (dstptr & ~PAGEMASK));
            }
        }
    }

    return copied;
}


/********************************************************************************/
/* Exception handlers */

mmu_mapfunc_t mmu_map_get_callback(void) {
    return map_func;
}

mmu_mapfunc_t mmu_map_set_callback(mmu_mapfunc_t newfunc) {
    mmu_mapfunc_t tmp = map_func;
    map_func = newfunc;

    if(mmu_cxt_current && map_func == map_virt)
        mmu_shortcut_ok = 1;
    else
        mmu_shortcut_ok = 0;

    return tmp;
}

static void unhandled_mmu(irq_t source, irq_context_t *context) {
    int i;

    (void)source;

    dbgio_printf("Exception happened in tid %d at PC %08lx, SR %08lx\n",
                 thd_current->tid, context->pc, context->sr);
    dbgio_printf(" PTEH = %08lx, PTEL = %08lx\n", *pteh, *ptel);
    dbgio_printf(" TTB = %08lx, TEA = %08lx\n", *ttb, *tea);
    dbgio_printf(" MMUCR = %08lx\n", *mmucr);
    dbgio_printf(" PR = %08lx\n", context->pr);

    for(i = 0; i < 512; i++)
        dbgio_flush();

    arch_panic("unhandled MMU exception");
}

/* Generic handler that takes a missed TLB exception and loads the
   appropriate entry into the UTLB. */
void mmu_gen_tlb_miss(const char *what, irq_t source, irq_context_t *context) {
    mmupage_t *page;
    uint32_t addr, ptehv, ptelv;

    /* Get the offending reference */
    addr = *tea;

    /* Do we have a mapping func? */
    if(!map_func) {
        dbgio_printf("%s: no mapping function to map address %08lx!\n",
                     what, addr);
        unhandled_mmu(source, context);
    }

    /* Do we have page tables? */
    if(map_func == map_virt && !mmu_cxt_current) {
        dbgio_printf("%s: no page tables installed to map address %08lx!\n",
                     what, addr);
        unhandled_mmu(source, context);
    }

    /* Translate it to the proper physical address */
    page = map_func(mmu_cxt_current, addr >> PAGESIZE_BITS);

    if(!page) {
        dbgio_printf("%s: cannot map virtual address %08lx\n", what, addr);
        unhandled_mmu(source, context);
    }

    /* Make sure we don't overwrite the last TLB entry */
    /* if(GET_URC() == last_urc) {
        last_urc++;
        SET_URC(last_urc);
    } else {
        last_urc = GET_URC();
    } */

    /* Load the mapping */
    //dbgio_printf("asid %d: loading up mapping %08x -> %08x, prkey=%d into %x\n",
    //  proc_current->pt->asid, *tea, page->physical << PAGESIZE_BITS, page->prkey, last_urc);
    ptehv = page->pteh | mmu_cxt_current->asid;
    ptelv = page->ptel;
    mmu_ldtlb_quick(ptehv, ptelv);
    /* mmu_ldtlb(mmu_cxt_current->asid, *tea, page->physical << PAGESIZE_BITS, 1, page->prkey,
        page->cache, page->dirty, page->shared, page->wthru);
    mmu_ldtlb_wait(); */
}

/* Instruction TLB miss exception */
static void itlb_miss(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    mmu_gen_tlb_miss("itlb_miss", source, context);
}

/* Instruction TLB protection violation */
static void itlb_pv(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    dbgio_printf("itlb_pv\n");
    unhandled_mmu(source, context);
}

/* Should eventually handle data address read/write here */

/* Data TLB miss (read) */
static void dtlb_miss_read(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    mmu_gen_tlb_miss("dtlb_miss_read", source, context);
}

/* Data TLB miss (write) */
static void dtlb_miss_write(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    mmu_gen_tlb_miss("dtlb_miss_write", source, context);
}

/* Data TLB protection violation (read) */
static void dtlb_pv_read(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    dbgio_printf("dtlb_pv_read\n");
    unhandled_mmu(source, context);
}

/* Data TLB protection violation (write) */
static void dtlb_pv_write(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    dbgio_printf("dtlb_pv_write\n");
    unhandled_mmu(source, context);
}

/* Initial page write exception */
static void initial_page_write(irq_t source, irq_context_t *context, void *data) {
    (void)data;
    dbgio_printf("initial_page_write\n");
    unhandled_mmu(source, context);
}

/********************************************************************************/
/* Init routine */
void mmu_init(void) {
    /* Setup last URC counter (to make sure we don't thrash the
       TLB caches accidentally) */
    last_urc = 0;

    /* Set the default mapping func */
    map_func = map_virt;

    /* No context yet */
    mmu_cxt_current = NULL;

    /* No context -- shortcuts not OK yet */
    mmu_shortcut_ok = 0;

    /* Set up interrupt handlers */
    irq_set_handler(EXC_ITLB_MISS, itlb_miss, NULL);
    irq_set_handler(EXC_ITLB_PV, itlb_pv, NULL);
    irq_set_handler(EXC_DTLB_MISS_READ, dtlb_miss_read, NULL);
    irq_set_handler(EXC_DTLB_MISS_WRITE, dtlb_miss_write, NULL);
    irq_set_handler(EXC_DTLB_PV_READ, dtlb_pv_read, NULL);
    irq_set_handler(EXC_DTLB_PV_WRITE, dtlb_pv_write, NULL);
    irq_set_handler(EXC_INITIAL_PAGE_WRITE, initial_page_write, NULL);

    /* Reserve TLB entries 62-63 for SQ translation. Register them as read-write
     * (since there's no write-only flag) with a 1 MiB page. */
    SET_MMUCR(0x3e, 0x3e, 1, 0, 1, 1);
    mmu_ldtlb(0, 0xe0000000, 0, 3, 1, 0, 0, 0, 0);
    SET_MMUCR(0x3f, 0x3f, 1, 0, 1, 1);
    mmu_ldtlb(0, 0xe0100000, 0, 3, 1, 0, 0, 0, 0);

    /* Set URB to 0x3d to not overwrite the SQ config, reset URC, enable MMU */
    SET_MMUCR(0x3d, 0, 1, 0, 1, 1);

    /* Clear the ITLB */
    mmu_reset_itlb();
}

/* Shutdown */
void mmu_shutdown(void) {
    /* Turn off MMU */
    *mmucr = 0x00000204;

    /* No more shortcuts */
    mmu_shortcut_ok = 0;

    /* Unhook the IRQ handlers */
    irq_set_handler(EXC_ITLB_MISS, NULL, NULL);
    irq_set_handler(EXC_ITLB_PV, NULL, NULL);
    irq_set_handler(EXC_DTLB_MISS_READ, NULL, NULL);
    irq_set_handler(EXC_DTLB_MISS_WRITE, NULL, NULL);
    irq_set_handler(EXC_DTLB_PV_READ, NULL, NULL);
    irq_set_handler(EXC_DTLB_PV_WRITE, NULL, NULL);
    irq_set_handler(EXC_INITIAL_PAGE_WRITE, NULL, NULL);
}

bool mmu_enabled(void) {
    return *mmucr & 0x1;
}

void mmu_set_sq_addr(void *addr) {
    uint32_t ppn1 = (uint32_t)addr & 0x1ff00000;
    uint32_t ppn2 = ppn1 + 0x00100000;

    /* Reset the base target address for the SQs */
    *(uint32_t *)(MEM_AREA_UTLB_DATA_ARRAY1_BASE + (0x3e << 8)) = ppn1 | 0x1fc;
    *(uint32_t *)(MEM_AREA_UTLB_DATA_ARRAY1_BASE + (0x3f << 8)) = ppn2 | 0x1fc;
}
