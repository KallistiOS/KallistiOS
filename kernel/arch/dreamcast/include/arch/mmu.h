/* KallistiOS ##version##

   arch/dreamcast/include/arch/mmu.h
   Copyright (C) 2001 Megan Potter

*/

/** \file    arch/mmu.h
    \brief   Memory Management Unit and Translation Lookaside Buffer handling.
    \ingroup mmu

    This file defines the interface to the Memory Management Unit (MMU) in the
    SH4. The MMU, while not used normally by KOS, is available for virtual
    memory use, if you so desire. While using this functionality is probably
    overkill for most homebrew, there are a few very interesting things that
    this functionality could be used for (like mapping large files into memory
    that wouldn't otherwise fit).

    The whole system is set up as a normal paged memory virtual->physical
    address translation. KOS implements the page table as a sparse, two-level
    page table. By default, pages are 4KB in size. Each top-level page table
    entry has 512 2nd level entries (there are 1024 entries in the top-level
    entry). This works out to about 2KB of space needed for one top-level entry.

    The SH4 itself has 4 TLB entries for instruction fetches, and 64 "unified"
    TLB entries (for combined instructions + data). Essentially, the UTLB acts
    both as the TLB for data accesses (from mov instructions) and as a cache for
    entries for the ITLB. If there is no entry in the ITLB for an instruction
    access, the UTLB will automatically be searched. If no entry is found still,
    an ITLB miss exception will be generated. Data accesses are handled
    similarly to this (although additional complications are involved due to
    write accesses, and of course the ITLB doesn't play into data accesses).

    For more information about how the MMU works, refer to the Hitachi/Renesas
    SH4 programming manual. It has much more detailed information than what is
    in here, for obvious reasons.

    This functionality was ported over to mainline KOS from the KOS-MMU project
    of Megan Potter. Unfortunately, KOS-MMU never reached a real phase of maturity
    and usefulness, but this piece can be quite useful on its own.

    \author Megan Potter
*/

#ifndef __ARCH_MMU_H
#define __ARCH_MMU_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>

#include <arch/types.h>
#include <sys/uio.h>

/** \defgroup mmu   MMU
    \brief          Driver for the SH4's MMU (disabled by default).
    \ingroup        system

    Since the software has to handle TLB misses on the SH-4, we have freedom
    to use any page table format we want (and thus save space), but we must
    make it quick to access. The SH-4 can address a maximum of 512M of address
    space per "area", but we only care about one area, so this is the total
    maximum addressable space. With 4K pages, that works out to 2^17 pages
    that must be mappable, or 17 bits. We use 18 bits just to be sure (there
    are a few left over).

    Page tables (per-process) are a sparse two-level array. The virtual address
    space is actually 2^30 bytes, or 2^(30-12)=2^18 pages, so there must be
    a possibility of having that many page entries per process space. A full
    page table for a process would be 1M, so this is obviously too big!! Thus
    the sparse array.

    The bottom layer of the page tables consists of a sub-context array for
    512 pages, which translates into 2K of storage space. The process then
    has the possibility of using one or more of the 512 top-level slots. For
    a very small process (using one page for code/data and one for stack), it
    should be possible to achieve a page table footprint of one page. The tables
    can grow from there as necessary.

    Virtual addresses are broken up as follows:
    - Bits 31 - 22     10 bits top-level page directory
    - Bits 21 - 13     9 bits bottom-level page entry
    - Bits 11 - 0      Byte index into page

*/

/** \defgroup mmu_bit_macros        Address Bits
    \brief                          Definitions and masks for address pages
    \ingroup                        mmu

    The MMU code uses these to determine the page of a request.

    @{
*/
#define MMU_TOP_SHIFT 21                        /**< \brief Top-level shift */
#define MMU_TOP_BITS 10                         /**< \brief Top-level bits */
#define MMU_TOP_MASK ((1 << MMU_TOP_BITS) - 1)  /**< \brief Top-level mask */
#define MMU_BOT_SHIFT 12                        /**< \brief Bottom shift */
#define MMU_BOT_BITS 9                          /**< \brief Bottom bits */
#define MMU_BOT_MASK ((1 << MMU_BOT_BITS) - 1)  /**< \brief Bottom mask */
#define MMU_IND_SHIFT 0                         /**< \brief Index shift */
#define MMU_IND_BITS 12                         /**< \brief Index bits */
#define MMU_IND_MASK ((1 << MMU_IND_BITS) - 1)  /**< \brief Index mask */
/** @} */

/** \defgroup mmu_prot_values       Protection Settings
    \brief                          SH4 MMU page protection settings values
    \ingroup                        mmu

    Each page mapped via the MMU can be protected in a couple of different ways,
    as specified here.

    @{
*/
#define MMU_KERNEL_RDONLY   0   /**< \brief No user access, kernel read-only */
#define MMU_KERNEL_RDWR     1   /**< \brief No user access, kernel full */
#define MMU_ALL_RDONLY      2   /**< \brief Read-only user and kernel */
#define MMU_ALL_RDWR        3   /**< \brief Full access, user and kernel */
/** @} */

/** \defgroup mmu_cache_values      Cacheability Settings
    \brief                          SH4 MMU page cachability settings values
    \ingroup                        mmu

    Each page mapped via the MMU can have its cacheability set individually.

    @{
*/
#define MMU_NO_CACHE    1               /**< \brief Cache disabled */
#define MMU_CACHE_BACK  2               /**< \brief Write-back caching */
#define MMU_CACHE_WT    3               /**< \brief Write-through caching */
#define MMU_CACHEABLE   MMU_CACHE_BACK  /**< \brief Default caching */
/** @} */

/** \brief   MMU TLB entry for a single page.
    \ingroup mmu

    The TLB entries on the SH4 are a single 32-bit dword in length. We store
    some other data here too for ease of use.

    \headerfile arch/mmu.h
*/
typedef struct mmupage {
    /* Explicit pieces, used for reference */
    /*uint32    virtual; */ /* implicit */
    uint32  physical: 18;   /**< \brief Physical page ID -- 18 bits */
    uint32  prkey: 2;       /**< \brief Protection key data -- 2 bits */
    uint32  valid: 1;       /**< \brief Valid mapping -- 1 bit */
    uint32  shared: 1;      /**< \brief Shared between procs -- 1 bit */
    uint32  cache: 1;       /**< \brief Cacheable -- 1 bit */
    uint32  dirty: 1;       /**< \brief Dirty -- 1 bit */
    uint32  wthru: 1;       /**< \brief Write-thru enable -- 1 bit */
    uint32  blank: 7;       /**< \brief Reserved -- 7 bits */

    /* Pre-compiled pieces. These waste a bit of ram, but they also
       speed loading immensely at runtime. */
    uint32  pteh;           /**< \brief Pre-built PTEH value */
    uint32  ptel;           /**< \brief Pre-built PTEL value */
} mmupage_t;

/** \brief   The number of pages in a sub-context.
    \ingroup mmu
 */
#define MMU_SUB_PAGES   512

/** \brief   MMU sub-context type.
    \ingroup mmu

    We have two-level page tables on SH4, and each sub-context contains 512
    entries.

    \headerfile arch/mmu.h
*/
typedef struct mmusubcontext {
    mmupage_t   page[MMU_SUB_PAGES];    /**< \brief 512 page entries */
} mmusubcontext_t;

/** \brief  The number of sub-contexts in the main level context. */
#define MMU_PAGES   1024

/** \brief   MMU context type.
    \ingroup mmu

    This type is the top-level context that makes up the page table. There is
    one of these, with 1024 sub-contexts.

    \headerfile arch/mmu.h
*/
typedef struct mmucontext {
    mmusubcontext_t *sub[MMU_PAGES];    /**< \brief 1024 sub-contexts */
    int             asid;               /**< \brief Address Space ID */
} mmucontext_t;

/** \cond
    You should not modify this directly, but rather use the functions provided
    to do so.
*/
extern mmucontext_t *mmu_cxt_current;
/** \endcond */

/** \brief   Set the "current" page tables for TLB handling.
    \ingroup mmu

    This function is useful if you're trying to implement a process model or
    something of the like on top of KOS. Essentially, this allows you to
    completely boot the MMU context in use out and replace it with another. You
    will need to call the mmu_switch_context() function afterwards to set the
    address space id.

    \param  context         The context to make current.
*/
void mmu_use_table(mmucontext_t *context);

/** \brief   Allocate a new MMU context.
    \ingroup mmu

    Each process should have exactly one of these, and these should not exist
    without a process. Since KOS doesn't actually have a process model of its
    own, that means you will only ever have one of these, if any.

    \param  asid            The address space ID of this process.
    \return                 The newly created context or NULL on fail.
*/
mmucontext_t *mmu_context_create(int asid);

/** \brief   Destroy an MMU context when a process is being destroyed.
    \ingroup mmu

    This function cleans up a MMU context, deallocating any memory its using.

    \param  context         The context to clean up after.
*/
void mmu_context_destroy(mmucontext_t *context);

/** \brief   Using the given page tables, translate the virtual page ID to a
             physical page ID.
    \ingroup mmu

    \param  context         The context to look in.
    \param  virtpage        The virtual page number to look for.
    \return                 The physical page number, or -1 on failure.
    \see    mmu_phys_to_virt()
*/
int mmu_virt_to_phys(mmucontext_t *context, int virtpage);

/** \brief   Using the given page tables, translate the physical page ID to a
             virtual page ID.
    \ingroup mmu

    \param  context         The context to look in.
    \param  physpage        The physical page number to look for.
    \return                 The virtual page number, or -1 on failure.
    \see    mmu_virt_to_phys()
*/
int mmu_phys_to_virt(mmucontext_t *context, int physpage);

/** \brief   Switch to the given context.
    \ingroup mmu

    This function switches to the given context's address space ID. The context
    should have already been made current with mmu_use_table().
    You are responsible for invalidating any caches as necessary, as well as
    invalidating any stale TLB entries.

    \param  context         The context to make current.
*/
void mmu_switch_context(mmucontext_t *context);

/** \brief   Set the given virtual page to map to the given physical page.
    \ingroup mmu

    This implies turning on the "valid" bit. Also sets the other named
    attributes as specified.

    \param  context         The context to modify.
    \param  virtpage        The first virtual page to map.
    \param  physpage        The first physical page to map.
    \param  count           The number of sequential pages to map.
    \param  prot            Memory protection for page (see
                            \ref mmu_prot_values).
    \param  cache           Cache scheme for page (see \ref mmu_cache_values).
    \param  share           Set to 1 to share between processes (meaningless),
                            otherwise set to 0.
    \param  dirty           Set to 1 to mark the page as dirty, otherwise set to
                            0.
*/
void mmu_page_map(mmucontext_t *context, int virtpage, int physpage,
                  int count, int prot, int cache, int share, int dirty);

/** \brief   Copy a chunk of data from a process' address space into a kernel
             buffer, taking into account page mappings.
    \ingroup mmu

    \param  context         The context to use.
    \param  srcaddr         Source, in the mapped memory space.
    \param  srccnt          The number of bytes to copy.
    \param  buffer          The kernel buffer to copy into (should be in P1).
    \return                 The number of bytes copied (failure causes arch_panic).
*/
int mmu_copyin(mmucontext_t *context, uint32 srcaddr, uint32 srccnt,
               void *buffer);

/** \brief   Copy a chunk of data from one process' address space to another
             process' address space, taking into account page mappings.
    \ingroup mmu

    \param  context1        The source's context.
    \param  iov1            The scatter/gather array to copy from.
    \param  iovcnt1         The number of entries in iov1.
    \param  context2        The destination's context.
    \param  iov2            The scatter/gather array to copy to.
    \param  iovcnt2         The number of entries in iov2.
    \return                 The number of bytes copied (failure causes arch_panic).
*/
int mmu_copyv(mmucontext_t *context1, struct iovec *iov1, int iovcnt1,
              mmucontext_t *context2, struct iovec *iov2, int iovcnt2);

/** \brief   MMU mapping handler.
    \ingroup mmu

    This type is used for functions that will take over the mapping for the
    kernel. In general, there shouldn't be much use for taking this over
    yourself, unless you want to change the size of the page table entries or
    something of the like.

    \param  context         The context in use.
    \param  virtpage        The virtual page to map.
    \return                 The page table entry, or NULL if none exists.
*/
typedef mmupage_t * (*mmu_mapfunc_t)(mmucontext_t * context, int virtpage);

/** \brief   Get the current mapping function.
    \ingroup mmu
    \return                 The current function that maps pages.
*/
mmu_mapfunc_t mmu_map_get_callback(void);

/** \brief   Set a new MMU mapping handler.
    \ingroup mmu

    This function will allow you to set a new function to handle mapping for
    memory pages. There's not much of a reason to do this unless you really do
    not like the way KOS handles the page mapping internally.

    \param  newfunc         The new function to handle mapping.
    \return                 The old function that did mapping.
*/
mmu_mapfunc_t mmu_map_set_callback(mmu_mapfunc_t newfunc);

/** \brief   Initialize MMU support.
    \ingroup mmu

    Unlike most things in KOS, the MMU is not initialized by a normal startup.
    This is because for most homebrew, its not needed.
*/
void mmu_init(void);

/** \brief   Shutdown MMU support.
    \ingroup mmu

    Turn off the MMU after it was initialized. You should try to make sure this
    gets done if you initialize the MMU in your program, so as to play nice with
    loaders and the like (that will not expect that its on, in general).
*/
void mmu_shutdown(void);

/** \brief   Reset ITLB.
    \ingroup mmu
 */
void mmu_reset_itlb(void);

/** \brief   Check if MMU translation is enabled.
    \ingroup mmu

    \return                 True if MMU translation is enabled, false otherwise.
 */
bool mmu_enabled(void);

/** \brief   Reset the base target address for store queues.
 *  \ingroup mmu
 *
 *  \param  addr            The base address to reset to */
void mmu_set_sq_addr(void *addr);

__END_DECLS

#endif  /* __ARCH_MMU_H */
