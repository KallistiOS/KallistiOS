/* KallistiOS ##version##

   malloc.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2015 Lawrence Sebald

*/

/** \file    malloc.h
    \brief   Standard C Malloc functionality
    \ingroup system_allocator

    This implements standard C heap allocation, deallocation, and stats.

    \author Megan Potter
    \author Lawrence Sebald
*/

#ifndef __MALLOC_H
#define __MALLOC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup system_allocator  Allocator
    \brief                      Dynamic memory heap management and allocation
    \ingroup                    system

    @{
*/

/* Unlike previous versions, we totally decouple the implementation from
   the declarations. */

/** \brief ANSI C functions */
struct mallinfo {
    /** \brief non-mmapped space allocated from system */
    int arena;
    /** \brief number of free chunks */
    int ordblks;
    /** \brief number of fastbin blocks */
    int smblks;
    /** \brief number of mmapped regions */
    int hblks;
    /** \brief space in mmapped regions */
    int hblkhd;
    /** \brief maximum total allocated space */
    int usmblks;
    /** \brief space available in freed fastbin blocks */
    int fsmblks;
    /** \brief total allocated space */
    int uordblks;
    /** \brief total free space */
    int fordblks;
    /** \brief top-most, releasable (via malloc_trim) space */
    int keepcost;
};

/** \brief  Allocate a block of memory.

    This function allocates a block of memory of the specified number of bytes
    on the heap. This memory must later be freed by way of the free() function.
    The memory *is not* freed simply because a pointer to it goes out of scope.

    \param  size            Number of bytes of memory to allocate.
    \return                 A pointer to the newly allocated block of memory, or
                            NULL on failure.

    \see                    free
    \see                    calloc
    \note                   The block of memory returned is completely
                            uninitialized and may contain random data.
*/
void *malloc(size_t size);

/** \brief  Allocate a block of memory and initialize it to 0.

    This function allocates a block of memory of size * nmemb bytes,
    initializing it to all zero bytes in the process. In other words, this
    function allocates an array of nmemb elements that are each size bytes in
    length. Just as with malloc(), you are responsible for calling free() on the
    block of memory when you are done with it.

    \param  nmemb           Number of elements to allocate space for.
    \param  size            Size of each element in the array.
    \return                 A pointer to the newly allocated block of memory, or
                            NULL on failure.

    \see                    free
    \see                    malloc
*/
void *calloc(size_t nmemb, size_t size);

/** \brief  Release a previously allocated block of memory.

    This function frees memory that has been previously allocated by way of any
    of the allocation functions in this file (malloc(), calloc(), memalign(),
    valloc(), or aligned_alloc()), releasing the memory to be potentially used
    for any future allocations.

    \param  ptr             A pointer to the block of memory to be freed.

    \note                   Passing a NULL pointer to this function has no
                            effect.
    \note                   Attempting to free the same block of memory twice
                            exhibits undefined behavior (i.e, it may crash, it
                            may do something else evil, etc).
    \note                   Attempting to free a block of memory that was not
                            allocated by way of the normal allocation functions
                            exhibits undefined behavior.
*/
void free(void *ptr);

/** \brief  Change the size of a previously allocated block of memory.

    This function changes the size of the previously allocated block of memory
    from its current size to the specified size. This may involve reallocating
    and copying the data from the original pointer to a new location in memory.

    If this function returns non-NULL, the old pointer is considered invalid and
    should not be used (unless, of course, the returned pointer is the same as
    the old pointer). No action is needed to clean up the old pointer, as this
    function will have freed it if necessary.

    If this function returns NULL, the old pointer is still valid and has not
    had its size changed.

    \param  ptr             A pointer to the block of memory to resize. It must
                            have been previously allocated by way of one of the
                            memory allocation functions in this file (or must
                            be NULL).
    \param  size            The requested size of the new block of memory.
    \return                 A pointer to the newly allocated/resized block of
                            memory on success, or NULL on failure.

    \note                   If ptr is NULL on a call to this function, the
                            function acts the same as malloc(size).
    \note                   If ptr is non-NULL and size is zero, this function
                            *does not* free the block of memory -- it simply
                            returns a block that is of minimal size. By the
                            standard, this pointer must not be dereferenced.
*/
void *realloc(void *ptr, size_t size);

/** \brief  Allocate a block of memory aligned to a specified block size.

    This function allocates a block of memory such that the lowest address in
    the block is aligned to a multiple of alignment bytes. This is useful, for
    instance, for things like DMA that require aligned blocks of memory to work.

    Over-reliance on memalign will most certainly cause memory fragmentation, so
    you should only use it when it is actually necessary.

    \param  alignment       The alignment requested for the block of memory.
                            This must be a power-of-two.
    \param  size            The number of bytes of memory to allocate.
    \return                 A pointer to the newly allocated block of memory on
                            success, or NULL on failure.

    \note                   All memory allocation functions will have their
                            blocks aligned on 8-byte boundaries. There is no
                            reason to call this function if you need less than
                            16-byte alignment.
*/
void *memalign(size_t alignment, size_t size);

/** \brief  Allocate a block of memory aligned to the system page size.

    This function allocates a block of memory such that the lowest address in
    the block is aligned to the system page size (typically 4096 bytes). It
    basically ends up doing return memalign(PAGESIZE, size).

    \param  size            The number of bytes of memory to allocate.
    \return                 A pointer to the newly allocated block of memory on
                            success, or NULL on failure.

    \see                    PAGESIZE
    \see                    <arch/arch.h>
*/
void *valloc(size_t size);

#if !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L)

/** \brief  Allocate memory aligned to a specified block size.

    This function is the standard-compliant C11 method for allocating aligned
    blocks of memory. It works mostly the same as memalign(), although the
    size must be a multiple of the alignment.

    \param      alignment   Required alignment of the memory block.
    \param      size        Number of bytes of memory to allocate.
    \return                 A pointer to the newly allocated memory block.
*/
void *aligned_alloc(size_t alignment, size_t size);

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) */

/** \brief Sets tunable parameters for malloc related options.
*/
struct mallinfo mallinfo();

/* mallopt defines */
#define M_MXFAST 1
#define DEFAULT_MXFAST 64

#define M_TRIM_THRESHOLD -1
#define DEFAULT_TRIM_THRESHOLD (256*1024)

#define M_TOP_PAD -2
#define DEFAULT_TOP_PAD 0

#define M_MMAP_THRESHOLD -3
#define DEFAULT_MMAP_THRESHOLD (256*1024)

#define M_MMAP_MAX -4
#define DEFAULT_MMAP_MAX 65536
int  mallopt(int, int);

/** \brief Debug function
*/
void malloc_stats(void);

/** \brief  Determine if it is safe to call malloc() in an IRQ context.

    This function checks the value of the internal spinlock that is used for
    malloc() to ensure that a call to it will not freeze the running process.
    This is only really useful in an IRQ context to ensure that a call to
    malloc() (or some other memory allocation function) won't cause a deadlock.

    \retval     1           If it is safe to call malloc() in the current IRQ.
    \retval     0           Otherwise.
*/
int malloc_irq_safe(void);

/** \brief Only available with KM_DBG
*/
int mem_check_block(void *p);

/** \brief Only available with KM_DBG
 */
int mem_check_all(void);

/** @} */

__END_DECLS

#endif  /* __MALLOC_H */
