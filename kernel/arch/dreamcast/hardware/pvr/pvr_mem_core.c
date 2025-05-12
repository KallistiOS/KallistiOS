/* KallistiOS ##version##

   pvr_mem_core.c
   (c)2002 Megan Potter

   This was adapted from the system-wide malloc, but optimized for
   usage with the PVR texture memory pool. So that end there are a couple
   of special considerations here:

   - Default memory alignment has been changed to 32 bytes (I'm not sure
     what the PVR itself needs, but this will allow DMA in the future)
   - MORECORE is pvr_sbrk(), defined in pvr_mem.c
   - A lot of functions have been trimmed out of here since they simply
     aren't useful (or they won't be used commonly)
   - Since some symbols will collide with the system-wide malloc, we
     cannot include both headers in the same place; thus this interface
     is a private interface inside the PVR module, and the public one
     will be defined in pvr_mem.c
   - One new function, pvr_int_mem_reset() has been included to handle
     reinitializing the malloc structures when the structure of the
     PVR memory changes (resolution switch, etc).


   Original header notice:

   Most of this module was written by Doug Lea and released into the
   public domain. We have incorporated it under the KallistiOS
   copyright umbrella for release under the BSD-like license.

   This beast looks daunting, but a lot of it is comments and it has some
   very nice thread safety support.
 */

#include "pvr_mem_core.h"

#include <kos/dbglog.h>
#include <kos/opts.h>

#undef DEBUG

#ifdef PVR_MALLOC_DEBUG
#define DEBUG 1
#endif

/* Defined in pvr_mem.c */
extern void * pvr_int_sbrk(size_t amt);

/* --------------------- public wrappers ---------------------- */

#ifdef USE_PUBLIC_MALLOC_WRAPPERS

/* Declare all routines as internal */
#if __STD_C
static Void_t*  mALLOc(size_t);
static void     fREe(Void_t*);
static Void_t*  rEALLOc(Void_t*, size_t);
static Void_t*  mEMALIGn(size_t, size_t);
static Void_t*  vALLOc(size_t);
static Void_t*  pVALLOc(size_t);
static Void_t*  cALLOc(size_t, size_t);
static Void_t** iCALLOc(size_t, size_t, Void_t**);
static Void_t** iCOMALLOc(size_t, size_t*, Void_t**);
static void     cFREe(Void_t*);
static int      mTRIm(size_t);
static size_t   mUSABLe(Void_t*);
static void     mSTATs();
static int      mALLOPt(int, int);
static struct mallinfo mALLINFo(void);
#else
static Void_t*  mALLOc();
static void     fREe();
static Void_t*  rEALLOc();
static Void_t*  mEMALIGn();
static Void_t*  vALLOc();
static Void_t*  pVALLOc();
static Void_t*  cALLOc();
static Void_t** iCALLOc();
static Void_t** iCOMALLOc();
static void     cFREe();
static int      mTRIm();
static size_t   mUSABLe();
static void     mSTATs();
static int      mALLOPt();
static struct mallinfo mALLINFo();
#endif

/*
  MALLOC_PREACTION and MALLOC_POSTACTION should be
  defined to return 0 on success, and nonzero on failure.
  The return value of MALLOC_POSTACTION is currently ignored
  in wrapper functions since there is no reasonable default
  action to take on failure.
*/


#ifdef USE_MALLOC_LOCK

#include <arch/spinlock.h>

static spinlock_t mALLOC_MUTEx = SPINLOCK_INITIALIZER;

/* This is pretty GCC dependent ^_^; */
#define MALLOC_PREACTION   ({ spinlock_lock(&mALLOC_MUTEx); 0; })
#define MALLOC_POSTACTION  ({ spinlock_unlock(&mALLOC_MUTEx); 0; })

#else

/* Substitute anything you like for these */

#define MALLOC_PREACTION   (0)
#define MALLOC_POSTACTION  (0)

#endif /* USE_MALLOC_LOCK */

Void_t* public_pvr_mALLOc(size_t bytes) {
    Void_t* m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = mALLOc(bytes);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

void public_pvr_fREe(Void_t* m) {
    if(MALLOC_PREACTION != 0) {
        return;
    }

    fREe(m);

    if(MALLOC_POSTACTION != 0) {
    }
}

Void_t* public_pvr_rEALLOc(Void_t* m, size_t bytes) {
    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = rEALLOc(m, bytes);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

Void_t* public_pvr_mEMALIGn(size_t alignment, size_t bytes) {
    Void_t* m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = mEMALIGn(alignment, bytes);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

Void_t* public_pvr_vALLOc(size_t bytes) {
    Void_t* m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = vALLOc(bytes);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

Void_t* public_pvr_pVALLOc(size_t bytes) {
    Void_t* m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = pVALLOc(bytes);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

Void_t* public_pvr_cALLOc(size_t n, size_t elem_size) {
    Void_t* m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = cALLOc(n, elem_size);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}


Void_t** public_pvr_iCALLOc(size_t n, size_t elem_size, Void_t** chunks) {
    Void_t** m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = iCALLOc(n, elem_size, chunks);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

Void_t** public_pvr_iCOMALLOc(size_t n, size_t sizes[], Void_t** chunks) {
    Void_t** m;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    m = iCOMALLOc(n, sizes, chunks);

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

void public_pvr_cFREe(Void_t* m) {
    if(MALLOC_PREACTION != 0) {
        return;
    }

    cFREe(m);

    if(MALLOC_POSTACTION != 0) {
    }
}

int public_pvr_mTRIm(size_t s) {
    int result;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    result = mTRIm(s);

    if(MALLOC_POSTACTION != 0) {
    }

    return result;
}

size_t public_pvr_mUSABLe(Void_t* m) {
    size_t result;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    result = mUSABLe(m);

    if(MALLOC_POSTACTION != 0) {
    }

    return result;
}

void public_pvr_mSTATs(void) {
    if(MALLOC_PREACTION != 0) {
        return;
    }

    mSTATs();

    if(MALLOC_POSTACTION != 0) {
    }
}

struct mallinfo public_pvr_mALLINFo(void) {
    struct mallinfo m;

    if(MALLOC_PREACTION != 0) {
        struct mallinfo nm = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        return nm;
    }

    m = mALLINFo();

    if(MALLOC_POSTACTION != 0) {
    }

    return m;
}

int public_pvr_mALLOPt(int p, int v) {
    int result;

    if(MALLOC_PREACTION != 0) {
        return 0;
    }

    result = mALLOPt(p, v);

    if(MALLOC_POSTACTION != 0) {
    }

    return result;
}

#endif



/* ------------- Optional versions of memcopy ---------------- */


#if USE_MEMCPY

/*
  Note: memcpy is ONLY invoked with non-overlapping regions,
  so the (usually slower) memmove is not needed.
*/

#define MALLOC_COPY(dest, src, nbytes)  memcpy(dest, src, nbytes)
#define MALLOC_ZERO(dest, nbytes)       memset(dest, 0,   nbytes)

#else /* !USE_MEMCPY */

/* Use Duff's device for good zeroing/copying performance. */

#define MALLOC_ZERO(charp, nbytes)                                            \
    do {                                                                          \
        INTERNAL_SIZE_T* mzp = (INTERNAL_SIZE_T*)(charp);                           \
        unsigned long mctmp = (nbytes)/sizeof(INTERNAL_SIZE_T);                     \
        long mcn;                                                                   \
        if(mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
        switch (mctmp) {                                                            \
            case 0: for(;;) { *mzp++ = 0;                                             \
                case 7:           *mzp++ = 0;                                             \
                case 6:           *mzp++ = 0;                                             \
                case 5:           *mzp++ = 0;                                             \
                case 4:           *mzp++ = 0;                                             \
                case 3:           *mzp++ = 0;                                             \
                case 2:           *mzp++ = 0;                                             \
                case 1:           *mzp++ = 0; if(mcn <= 0) break; mcn--; }                \
        }                                                                           \
    } while(0)

define MALLOC_COPY(dest, src, nbytes)                                           \

do {
    \
    INTERNAL_SIZE_T* mcsrc = (INTERNAL_SIZE_T*) src;
    \
    INTERNAL_SIZE_T* mcdst = (INTERNAL_SIZE_T*) dest;
    \
    unsigned long mctmp = (nbytes) / sizeof(INTERNAL_SIZE_T);
    \
    long mcn;
    \

    if(mctmp < 8) mcn = 0;
    else {
        mcn = (mctmp - 1) / 8;
        mctmp %= 8;
    }             \

    switch(mctmp) {
            \
        case 0:

            for(;;) {
                *mcdst++ = *mcsrc++;
                \
            case 7:
                *mcdst++ = *mcsrc++;
                \
            case 6:
                *mcdst++ = *mcsrc++;
                \
            case 5:
                *mcdst++ = *mcsrc++;
                \
            case 4:
                *mcdst++ = *mcsrc++;
                \
            case 3:
                *mcdst++ = *mcsrc++;
                \
            case 2:
                *mcdst++ = *mcsrc++;
                \
            case 1:
                *mcdst++ = *mcsrc++;

                if(mcn <= 0) break;

                mcn--;
            }       \
    }                                                                           \
}
while(0)

#endif

/* ------------------ MMAP support ------------------  */


#if HAVE_MMAP

#include <fcntl.h>
#ifndef LACKS_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

/*
   Nearly all versions of mmap support MAP_ANONYMOUS,
   so the following is unlikely to be needed, but is
   supplied just in case.
*/

#ifndef MAP_ANONYMOUS

static int dev_zero_fd = -1; /* Cached file descriptor for /dev/zero. */

#define MMAP(addr, size, prot, flags) ((dev_zero_fd < 0) ? \
                                       (dev_zero_fd = open("/dev/zero", O_RDWR), \
                                        mmap((addr), (size), (prot), (flags), dev_zero_fd, 0)) : \
                                       mmap((addr), (size), (prot), (flags), dev_zero_fd, 0))

#else

#define MMAP(addr, size, prot, flags) \
    (mmap((addr), (size), (prot), (flags)|MAP_ANONYMOUS, -1, 0))

#endif


#endif /* HAVE_MMAP */


/*
  -----------------------  Chunk representations -----------------------
*/


/*
  This struct declaration is misleading (but accurate and necessary).
  It declares a "view" into memory allowing access to necessary
  fields at known offsets from a given base. See explanation below.
*/

struct malloc_chunk {

    INTERNAL_SIZE_T      prev_size;  /* Size of previous chunk (if free).  */
    INTERNAL_SIZE_T      size;       /* Size in bytes, including overhead. */

    struct malloc_chunk* fd;         /* double links -- used only if free. */
    struct malloc_chunk* bk;
};


typedef struct malloc_chunk* mchunkptr;

/*
   malloc_chunk details:

    (The following includes lightly edited explanations by Colin Plumb.)

    Chunks of memory are maintained using a `boundary tag' method as
    described in e.g., Knuth or Standish.  (See the paper by Paul
    Wilson ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a
    survey of such techniques.)  Sizes of free chunks are stored both
    in the front of each chunk and at the end.  This makes
    consolidating fragmented chunks into bigger chunks very fast.  The
    size fields also hold bits representing whether chunks are free or
    in use.

    An allocated chunk looks like this:


    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk, if allocated            | |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             User data starts here...                          .
            .                                                               .
            .             (malloc_usable_space() bytes)                     .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of chunk                                     |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


    Where "chunk" is the front of the chunk for the purpose of most of
    the malloc code, but "mem" is the pointer that is returned to the
    user.  "Nextchunk" is the beginning of the next contiguous chunk.

    Chunks always begin on even word boundaries, so the mem portion
    (which is returned to the user) is also on an even word boundary, and
    thus at least double-word aligned.

    Free chunks are stored in circular doubly-linked lists, and look like this:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk in list             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk in list            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space (may be 0 bytes long)                .
            .                                                               .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    The P (PREV_INUSE) bit, stored in the unused low-order bit of the
    chunk size (which is always a multiple of two words), is an in-use
    bit for the *previous* chunk.  If that bit is *clear*, then the
    word before the current chunk size contains the previous chunk
    size, and can be used to find the front of the previous chunk.
    The very first chunk allocated always has this bit set,
    preventing access to non-existent (or non-owned) memory. If
    prev_inuse is set for any given chunk, then you CANNOT determine
    the size of the previous chunk, and might even get a memory
    addressing fault when trying to do so.

    Note that the `foot' of the current chunk is actually represented
    as the prev_size of the NEXT chunk. This makes it easier to
    deal with alignments etc but can be very confusing when trying
    to extend or adapt this code.

    The two exceptions to all this are

     1. The special chunk `top' doesn't bother using the
        trailing size field since there is no next contiguous chunk
        that would have to index off it. After initialization, `top'
        is forced to always exist.  If it would become less than
        MINSIZE bytes long, it is replenished.

     2. Chunks allocated via mmap, which have the second-lowest-order
        bit (IS_MMAPPED) set in their size fields.  Because they are
        allocated one-by-one, each must contain its own trailing size field.

*/

/*
  ---------- Size and alignment checks and conversions ----------
*/

/* conversion from malloc headers to user pointers, and back */

#define chunk2mem(p)   ((Void_t*)((char*)(p) + 2*SIZE_SZ))
#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*SIZE_SZ))

/* The smallest possible chunk */
#define MIN_CHUNK_SIZE        (sizeof(struct malloc_chunk))

/* The smallest size we can malloc is an aligned minimal chunk */

#define MINSIZE  \
    (unsigned long)(((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))

/* Check if m has acceptable alignment */

#define aligned_OK(m)  (((unsigned long)((m)) & (MALLOC_ALIGN_MASK)) == 0)


/*
   Check if a request is so large that it would wrap around zero when
   padded and aligned. To simplify some other code, the bound is made
   low enough so that adding MINSIZE will also not wrap around sero.
*/

#define REQUEST_OUT_OF_RANGE(req)                                 \
    ((unsigned long)(req) >=                                        \
     (unsigned long)(INTERNAL_SIZE_T)(-2 * MINSIZE))

/* pad request bytes into a usable size -- internal version */

#define request2size(req)                                         \
    (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE)  ?             \
     MINSIZE :                                                      \
     ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

/*  Same, except also perform argument check */

#define checked_request2size(req, sz)                             \
    if(REQUEST_OUT_OF_RANGE(req)) {                                \
        MALLOC_FAILURE_ACTION;                                        \
        return 0;                                                     \
    }                                                               \
    (sz) = request2size(req);

/*
  --------------- Physical chunk operations ---------------
*/


/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */
#define PREV_INUSE 0x1

/* extract inuse bit of previous chunk */
#define prev_inuse(p)       ((p)->size & PREV_INUSE)


/* size field is or'ed with IS_MMAPPED if the chunk was obtained with mmap() */
#define IS_MMAPPED 0x2

/* check for mmap()'ed chunk */
#define chunk_is_mmapped(p) ((p)->size & IS_MMAPPED)

/*
  Bits to mask off when extracting size

  Note: IS_MMAPPED is intentionally not masked off from size field in
  macros for which mmapped chunks should never be seen. This should
  cause helpful core dumps to occur if it is tried by accident by
  people extending or adapting this malloc.
*/
#define SIZE_BITS (PREV_INUSE|IS_MMAPPED)

/* Get size, ignoring use bits */
#define chunksize(p)         ((p)->size & ~(SIZE_BITS))


/* Ptr to next physical malloc_chunk. */
#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->size & ~PREV_INUSE) ))

/* Ptr to previous physical malloc_chunk */
#define prev_chunk(p) ((mchunkptr)( ((char*)(p)) - ((p)->prev_size) ))

/* Treat space at ptr + offset as a chunk */
#define chunk_at_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))

/* extract p's inuse bit */
#define inuse(p)\
    ((((mchunkptr)(((char*)(p))+((p)->size & ~PREV_INUSE)))->size) & PREV_INUSE)

/* set/clear chunk as being inuse without otherwise disturbing */
#define set_inuse(p)\
    ((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size |= PREV_INUSE

#define clear_inuse(p)\
    ((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size &= ~(PREV_INUSE)


/* check/set/clear inuse bits in known places */
#define inuse_bit_at_offset(p, s)\
    (((mchunkptr)(((char*)(p)) + (s)))->size & PREV_INUSE)

#define set_inuse_bit_at_offset(p, s)\
    (((mchunkptr)(((char*)(p)) + (s)))->size |= PREV_INUSE)

#define clear_inuse_bit_at_offset(p, s)\
    (((mchunkptr)(((char*)(p)) + (s)))->size &= ~(PREV_INUSE))


/* Set size at head, without disturbing its use bit */
#define set_head_size(p, s)  ((p)->size = (((p)->size & PREV_INUSE) | (s)))

/* Set size/use field */
#define set_head(p, s)       ((p)->size = (s))

/* Set size at footer (only when chunk is not in use) */
#define set_foot(p, s)       (((mchunkptr)((char*)(p) + (s)))->prev_size = (s))


/*
  -------------------- Internal data structures --------------------

   All internal state is held in an instance of malloc_state defined
   below. There are no other static variables, except in two optional
   cases:
   * If USE_MALLOC_LOCK is defined, the mALLOC_MUTEx declared above.
   * If HAVE_MMAP is true, but mmap doesn't support
     MAP_ANONYMOUS, a dummy file descriptor for mmap.

   Beware of lots of tricks that minimize the total bookkeeping space
   requirements. The result is a little over 1K bytes (for 4byte
   pointers and size_t.)
*/

/*
  Bins

    An array of bin headers for free chunks. Each bin is doubly
    linked.  The bins are approximately proportionally (log) spaced.
    There are a lot of these bins (128). This may look excessive, but
    works very well in practice.  Most bins hold sizes that are
    unusual as malloc request sizes, but are more usual for fragments
    and consolidated sets of chunks, which is what these bins hold, so
    they can be found quickly.  All procedures maintain the invariant
    that no consolidated chunk physically borders another one, so each
    chunk in a list is known to be preceded and followed by either
    inuse chunks or the ends of memory.

    Chunks in bins are kept in size order, with ties going to the
    approximately least recently used chunk. Ordering isn't needed
    for the small bins, which all contain the same-sized chunks, but
    facilitates best-fit allocation for larger chunks. These lists
    are just sequential. Keeping them in order almost never requires
    enough traversal to warrant using fancier ordered data
    structures.

    Chunks of the same size are linked with the most
    recently freed at the front, and allocations are taken from the
    back.  This results in LRU (FIFO) allocation order, which tends
    to give each chunk an equal opportunity to be consolidated with
    adjacent freed chunks, resulting in larger free chunks and less
    fragmentation.

    To simplify use in double-linked lists, each bin header acts
    as a malloc_chunk. This avoids special-casing for headers.
    But to conserve space and improve locality, we allocate
    only the fd/bk pointers of bins, and then use repositioning tricks
    to treat these as the fields of a malloc_chunk*.
*/

typedef struct malloc_chunk* mbinptr;

/* addressing -- note that bin_at(0) does not exist */
#define bin_at(m, i) ((mbinptr)((char*)&((m)->bins[(i)<<1]) - (SIZE_SZ<<1)))

/* analog of ++bin */
#define next_bin(b)  ((mbinptr)((char*)(b) + (sizeof(mchunkptr)<<1)))

/* Reminders about list directionality within bins */
#define first(b)     ((b)->fd)
#define last(b)      ((b)->bk)

/* Take a chunk off a bin list */
#define unlink(P, BK, FD) {                                            \
        FD = P->fd;                                                          \
        BK = P->bk;                                                          \
        FD->bk = BK;                                                         \
        BK->fd = FD;                                                         \
    }

/*
  Indexing

    Bins for sizes < 512 bytes contain chunks of all the same size, spaced
    8 bytes apart. Larger bins are approximately logarithmically spaced:

    64 bins of size       8
    32 bins of size      64
    16 bins of size     512
     8 bins of size    4096
     4 bins of size   32768
     2 bins of size  262144
     1 bin  of size what's left

    There is actually a little bit of slop in the numbers in bin_index
    for the sake of speed. This makes no difference elsewhere.

    The bins top out around 1MB because we expect to service large
    requests via mmap.
*/

#define NBINS             128
#define NSMALLBINS         64
#define SMALLBIN_WIDTH      8
#define MIN_LARGE_SIZE    512

#define in_smallbin_range(sz)  \
    ((unsigned long)(sz) < (unsigned long)MIN_LARGE_SIZE)

#define smallbin_index(sz)     (((unsigned)(sz)) >> 3)

#define largebin_index(sz)                                                   \
    (((((unsigned long)(sz)) >>  6) <= 32)?  56 + (((unsigned long)(sz)) >>  6): \
     ((((unsigned long)(sz)) >>  9) <= 20)?  91 + (((unsigned long)(sz)) >>  9): \
     ((((unsigned long)(sz)) >> 12) <= 10)? 110 + (((unsigned long)(sz)) >> 12): \
     ((((unsigned long)(sz)) >> 15) <=  4)? 119 + (((unsigned long)(sz)) >> 15): \
     ((((unsigned long)(sz)) >> 18) <=  2)? 124 + (((unsigned long)(sz)) >> 18): \
     126)

#define bin_index(sz) \
    ((in_smallbin_range(sz)) ? smallbin_index(sz) : largebin_index(sz))


/*
  Unsorted chunks

    All remainders from chunk splits, as well as all returned chunks,
    are first placed in the "unsorted" bin. They are then placed
    in regular bins after malloc gives them ONE chance to be used before
    binning. So, basically, the unsorted_chunks list acts as a queue,
    with chunks being placed on it in free (and malloc_consolidate),
    and taken off (to be either used or placed in bins) in malloc.
*/

/* The otherwise unindexable 1-bin is used to hold unsorted chunks. */
#define unsorted_chunks(M)          (bin_at(M, 1))

/*
  Top

    The top-most available chunk (i.e., the one bordering the end of
    available memory) is treated specially. It is never included in
    any bin, is used only if no other chunk is available, and is
    released back to the system if it is very large (see
    M_TRIM_THRESHOLD).  Because top initially
    points to its own bin with initial zero size, thus forcing
    extension on the first malloc request, we avoid having any special
    code in malloc to check whether it even exists yet. But we still
    need to do so when getting memory from system, so we make
    initial_top treat the bin as a legal but unusable chunk during the
    interval between initialization and the first call to
    sYSMALLOc. (This is somewhat delicate, since it relies on
    the 2 preceding words to be zero during this interval as well.)
*/

/* Conveniently, the unsorted bin can be used as dummy top on first call */
#define initial_top(M)              (unsorted_chunks(M))

/*
  Binmap

    To help compensate for the large number of bins, a one-level index
    structure is used for bin-by-bin searching.  `binmap' is a
    bitvector recording whether bins are definitely empty so they can
    be skipped over during during traversals.  The bits are NOT always
    cleared as soon as bins are empty, but instead only
    when they are noticed to be empty during traversal in malloc.
*/

/* Conservatively use 32 bits per map word, even if on 64bit system */
#define BINMAPSHIFT      5
#define BITSPERMAP       (1U << BINMAPSHIFT)
#define BINMAPSIZE       (NBINS / BITSPERMAP)

#define idx2block(i)     ((i) >> BINMAPSHIFT)
#define idx2bit(i)       ((1U << ((i) & ((1U << BINMAPSHIFT)-1))))

#define mark_bin(m,i)    ((m)->binmap[idx2block(i)] |=  idx2bit(i))
#define unmark_bin(m,i)  ((m)->binmap[idx2block(i)] &= ~(idx2bit(i)))
#define get_binmap(m,i)  ((m)->binmap[idx2block(i)] &   idx2bit(i))

/*
  Fastbins

    An array of lists holding recently freed small chunks.  Fastbins
    are not doubly linked.  It is faster to single-link them, and
    since chunks are never removed from the middles of these lists,
    double linking is not necessary. Also, unlike regular bins, they
    are not even processed in FIFO order (they use faster LIFO) since
    ordering doesn't much matter in the transient contexts in which
    fastbins are normally used.

    Chunks in fastbins keep their inuse bit set, so they cannot
    be consolidated with other free chunks. malloc_consolidate
    releases all chunks in fastbins and consolidates them with
    other free chunks.
*/

typedef struct malloc_chunk* mfastbinptr;

/* offset 2 to use otherwise unindexable first 2 bins */
#define fastbin_index(sz)        ((((unsigned int)(sz)) >> 3) - 2)

/* The maximum fastbin request size we support */
#define MAX_FAST_SIZE     80

#define NFASTBINS  (fastbin_index(request2size(MAX_FAST_SIZE))+1)

/*
  FASTBIN_CONSOLIDATION_THRESHOLD is the size of a chunk in free()
  that triggers automatic consolidation of possibly-surrounding
  fastbin chunks. This is a heuristic, so the exact value should not
  matter too much. It is defined at half the default trim threshold as a
  compromise heuristic to only attempt consolidation if it is likely
  to lead to trimming. However, it is not dynamically tunable, since
  consolidation reduces fragmentation surrounding loarge chunks even
  if trimming is not used.
*/

#define FASTBIN_CONSOLIDATION_THRESHOLD  (65536UL)

/*
  Since the lowest 2 bits in max_fast don't matter in size comparisons,
  they are used as flags.
*/

/*
  FASTCHUNKS_BIT held in max_fast indicates that there are probably
  some fastbin chunks. It is set true on entering a chunk into any
  fastbin, and cleared only in malloc_consolidate.

  The truth value is inverted so that have_fastchunks will be true
  upon startup (since statics are zero-filled), simplifying
  initialization checks.
*/

#define FASTCHUNKS_BIT        (1U)

#define have_fastchunks(M)     (((M)->max_fast &  FASTCHUNKS_BIT) == 0)
#define clear_fastchunks(M)    ((M)->max_fast |=  FASTCHUNKS_BIT)
#define set_fastchunks(M)      ((M)->max_fast &= ~FASTCHUNKS_BIT)

/*
  NONCONTIGUOUS_BIT indicates that MORECORE does not return contiguous
  regions.  Otherwise, contiguity is exploited in merging together,
  when possible, results from consecutive MORECORE calls.

  The initial value comes from MORECORE_CONTIGUOUS, but is
  changed dynamically if mmap is ever used as an sbrk substitute.
*/

#define NONCONTIGUOUS_BIT     (2U)

#define contiguous(M)          (((M)->max_fast &  NONCONTIGUOUS_BIT) == 0)
#define noncontiguous(M)       (((M)->max_fast &  NONCONTIGUOUS_BIT) != 0)
#define set_noncontiguous(M)   ((M)->max_fast |=  NONCONTIGUOUS_BIT)
#define set_contiguous(M)      ((M)->max_fast &= ~NONCONTIGUOUS_BIT)

/*
   Set value of max_fast.
   Use impossibly small value if 0.
   Precondition: there are no existing fastbin chunks.
   Setting the value clears fastchunk bit but preserves noncontiguous bit.
*/

#define set_max_fast(M, s) \
    (M)->max_fast = (((s) == 0)? SMALLBIN_WIDTH: request2size(s)) | \
                    FASTCHUNKS_BIT | \
                    ((M)->max_fast &  NONCONTIGUOUS_BIT)


/*
   ----------- Internal state representation and initialization -----------
*/

struct malloc_state {

    /* The maximum chunk size to be eligible for fastbin */
    INTERNAL_SIZE_T  max_fast;   /* low 2 bits used as flags */

    /* Fastbins */
    mfastbinptr      fastbins[NFASTBINS];

    /* Base of the topmost chunk -- not otherwise kept in a bin */
    mchunkptr        top;

    /* The remainder from the most recent split of a small request */
    mchunkptr        last_remainder;

    /* Normal bins packed as described above */
    mchunkptr        bins[NBINS * 2];

    /* Bitmap of bins */
    unsigned int     binmap[BINMAPSIZE];

    /* Tunable parameters */
    unsigned long    trim_threshold;
    INTERNAL_SIZE_T  top_pad;
    INTERNAL_SIZE_T  mmap_threshold;

    /* Memory map support */
    int              n_mmaps;
    int              n_mmaps_max;
    int              max_n_mmaps;

    /* Cache malloc_getpagesize */
    unsigned int     pagesize;

    /* Statistics */
    INTERNAL_SIZE_T  mmapped_mem;
    INTERNAL_SIZE_T  sbrked_mem;
    INTERNAL_SIZE_T  max_sbrked_mem;
    INTERNAL_SIZE_T  max_mmapped_mem;
    INTERNAL_SIZE_T  max_total_mem;
};

typedef struct malloc_state *mstate;

/*
   There is exactly one instance of this struct in this malloc.
   If you are adapting this malloc in a way that does NOT use a static
   malloc_state, you MUST explicitly zero-fill it before using. This
   malloc relies on the property that malloc_state is initialized to
   all zeroes (as is true of C statics).
*/

static struct malloc_state av_;  /* never directly referenced */

/*
   All uses of av_ are via get_malloc_state().
   At most one "call" to get_malloc_state is made per invocation of
   the public versions of malloc and free, but other routines
   that in turn invoke malloc and/or free may call more then once.
   Also, it is called in check* routines if DEBUG is set.
*/

#define get_malloc_state() (&(av_))

/*
  Initialize a malloc_state struct.

  This is called only from within malloc_consolidate, which needs
  be called in the same contexts anyway.  It is never called directly
  outside of malloc_consolidate because some optimizing compilers try
  to inline it at all call points, which turns out not to be an
  optimization at all. (Inlining it in malloc_consolidate is fine though.)
*/

#if __STD_C
static void malloc_init_state(mstate av)
#else
static void malloc_init_state(av) mstate av;
#endif
{
    int     i;
    mbinptr bin;

    /* Establish circular links for normal bins */
    for(i = 1; i < NBINS; ++i) {
        bin = bin_at(av, i);
        bin->fd = bin->bk = bin;
    }

    av->top_pad        = DEFAULT_TOP_PAD;
    av->n_mmaps_max    = DEFAULT_MMAP_MAX;
    av->mmap_threshold = DEFAULT_MMAP_THRESHOLD;
    av->trim_threshold = DEFAULT_TRIM_THRESHOLD;

#if !MORECORE_CONTIGUOUS
    set_noncontiguous(av);
#endif

    set_max_fast(av, DEFAULT_MXFAST);

    av->top            = initial_top(av);
    av->pagesize       = malloc_getpagesize;
}

/*
   Other internal utilities operating on mstates
*/

#if __STD_C
static Void_t*  sYSMALLOc(INTERNAL_SIZE_T, mstate);
static int      sYSTRIm(size_t, mstate);
static void     malloc_consolidate(mstate);
static Void_t** iALLOc(size_t, size_t*, int, Void_t**);
#else
static Void_t*  sYSMALLOc();
static int      sYSTRIm();
static void     malloc_consolidate();
static Void_t** iALLOc();
#endif

/*
  Debugging support

  These routines make a number of assertions about the states
  of data structures that should be true at all times. If any
  are not true, it's very likely that a user program has somehow
  trashed memory. (It's also possible that there is a coding error
  in malloc. In which case, please report it!)
*/

#if ! DEBUG

#define check_chunk(P)
#define check_free_chunk(P)
#define check_inuse_chunk(P)
#define check_remalloced_chunk(P,N)
#define check_malloced_chunk(P,N)
#define check_malloc_state()

#else
#define check_chunk(P)              do_check_chunk(P)
#define check_free_chunk(P)         do_check_free_chunk(P)
#define check_inuse_chunk(P)        do_check_inuse_chunk(P)
#define check_remalloced_chunk(P,N) do_check_remalloced_chunk(P,N)
#define check_malloced_chunk(P,N)   do_check_malloced_chunk(P,N)
#define check_malloc_state()        do_check_malloc_state()

/*
  Properties of all chunks
*/

#if __STD_C
static void do_check_chunk(mchunkptr p)
#else
static void do_check_chunk(p) mchunkptr p;
#endif
{
    mstate av = get_malloc_state();
    unsigned long sz = chunksize(p);
    /* min and max possible addresses assuming contiguous allocation */
    char* max_address = (char*)(av->top) + chunksize(av->top);
    char* min_address = max_address - av->sbrked_mem;

    if(!chunk_is_mmapped(p)) {

        /* Has legal address ... */
        if(p != av->top) {
            if(contiguous(av)) {
                assert(((char*)p) >= min_address);
                assert(((char*)p + sz) <= ((char*)(av->top)));
            }
        }
        else {
            /* top size is always at least MINSIZE */
            assert((unsigned long)(sz) >= MINSIZE);
            /* top predecessor always marked inuse */
            assert(prev_inuse(p));
        }

    }
    else {
#if HAVE_MMAP

        /* address is outside main heap  */
        if(contiguous(av) && av->top != initial_top(av)) {
            assert(((char*)p) < min_address || ((char*)p) > max_address);
        }

        /* chunk is page-aligned */
        assert(((p->prev_size + sz) & (av->pagesize - 1)) == 0);
        /* mem is aligned */
        assert(aligned_OK(chunk2mem(p)));
#else
        /* force an appropriate assert violation if debug set */
        assert(!chunk_is_mmapped(p));
#endif
    }
}

/*
  Properties of free chunks
*/

#if __STD_C
static void do_check_free_chunk(mchunkptr p)
#else
static void do_check_free_chunk(p) mchunkptr p;
#endif
{
    mstate av = get_malloc_state();

    INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;
    mchunkptr next = chunk_at_offset(p, sz);

    do_check_chunk(p);

    /* Chunk must claim to be free ... */
    assert(!inuse(p));
    assert(!chunk_is_mmapped(p));

    /* Unless a special marker, must have OK fields */
    if((unsigned long)(sz) >= MINSIZE) {
        assert((sz & MALLOC_ALIGN_MASK) == 0);
        assert(aligned_OK(chunk2mem(p)));
        /* ... matching footer field */
        assert(next->prev_size == sz);
        /* ... and is fully consolidated */
        assert(prev_inuse(p));
        assert(next == av->top || inuse(next));

        /* ... and has minimally sane links */
        assert(p->fd->bk == p);
        assert(p->bk->fd == p);
    }
    else /* markers are always of size SIZE_SZ */
        assert(sz == SIZE_SZ);
}

/*
  Properties of inuse chunks
*/

#if __STD_C
static void do_check_inuse_chunk(mchunkptr p)
#else
static void do_check_inuse_chunk(p) mchunkptr p;
#endif
{
    mstate av = get_malloc_state();
    mchunkptr next;
    do_check_chunk(p);

    if(chunk_is_mmapped(p))
        return; /* mmapped chunks have no next/prev */

    /* Check whether it claims to be in use ... */
    assert(inuse(p));

    next = next_chunk(p);

    /* ... and is surrounded by OK chunks.
      Since more things can be checked with free chunks than inuse ones,
      if an inuse chunk borders them and debug is on, it's worth doing them.
    */
    if(!prev_inuse(p)) {
        /* Note that we cannot even look at prev unless it is not inuse */
        mchunkptr prv = prev_chunk(p);
        assert(next_chunk(prv) == p);
        do_check_free_chunk(prv);
    }

    if(next == av->top) {
        assert(prev_inuse(next));
        assert(chunksize(next) >= MINSIZE);
    }
    else if(!inuse(next))
        do_check_free_chunk(next);
}

/*
  Properties of chunks recycled from fastbins
*/

#if __STD_C
static void do_check_remalloced_chunk(mchunkptr p, INTERNAL_SIZE_T s)
#else
static void do_check_remalloced_chunk(p, s) mchunkptr p;
INTERNAL_SIZE_T s;
#endif
{
    INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;

    do_check_inuse_chunk(p);

    /* Legal size ... */
    assert((sz & MALLOC_ALIGN_MASK) == 0);
    assert((unsigned long)(sz) >= MINSIZE);
    /* ... and alignment */
    assert(aligned_OK(chunk2mem(p)));
    /* chunk is less than MINSIZE more than request */
    assert((long)(sz) - (long)(s) >= 0);
    assert((long)(sz) - (long)(s + MINSIZE) < 0);
}

/*
  Properties of nonrecycled chunks at the point they are malloced
*/

#if __STD_C
static void do_check_malloced_chunk(mchunkptr p, INTERNAL_SIZE_T s)
#else
static void do_check_malloced_chunk(p, s) mchunkptr p;
INTERNAL_SIZE_T s;
#endif
{
    /* same as recycled case ... */
    do_check_remalloced_chunk(p, s);

    /*
      ... plus,  must obey implementation invariant that prev_inuse is
      always true of any allocated chunk; i.e., that each allocated
      chunk borders either a previously allocated and still in-use
      chunk, or the base of its memory arena. This is ensured
      by making all allocations from the the `lowest' part of any found
      chunk.  This does not necessarily hold however for chunks
      recycled via fastbins.
    */

    assert(prev_inuse(p));
}


/*
  Properties of malloc_state.

  This may be useful for debugging malloc, as well as detecting user
  programmer errors that somehow write into malloc_state.

  If you are extending or experimenting with this malloc, you can
  probably figure out how to hack this routine to print out or
  display chunk addresses, sizes, bins, and other instrumentation.
*/

static void do_check_malloc_state(void) {
    mstate av = get_malloc_state();
    int i;
    mchunkptr p;
    mchunkptr q;
    mbinptr b;
    unsigned int binbit;
    int empty;
    unsigned int idx;
    INTERNAL_SIZE_T size;
    unsigned long total = 0;
    int max_fast_bin;

    /* internal size_t must be no wider than pointer type */
    assert(sizeof(INTERNAL_SIZE_T) <= sizeof(char*));

    /* alignment is a power of 2 */
    assert((MALLOC_ALIGNMENT & (MALLOC_ALIGNMENT - 1)) == 0);

    /* cannot run remaining checks until fully initialized */
    if(av->top == 0 || av->top == initial_top(av))
        return;

    /* pagesize is a power of 2 */
    assert((av->pagesize & (av->pagesize - 1)) == 0);

    /* properties of fastbins */

    /* max_fast is in allowed range */
    assert((av->max_fast & ~1) <= request2size(MAX_FAST_SIZE));

    max_fast_bin = fastbin_index(av->max_fast);

    for(i = 0; i < (int)NFASTBINS; ++i) {
        p = av->fastbins[i];

        /* all bins past max_fast are empty */
        if(i > max_fast_bin)
            assert(p == 0);

        while(p != 0) {
            /* each chunk claims to be inuse */
            do_check_inuse_chunk(p);
            total += chunksize(p);
            /* chunk belongs in this bin */
            assert(fastbin_index(chunksize(p)) == (unsigned)i);
            p = p->fd;
        }
    }

    if(total != 0)
        assert(have_fastchunks(av));
    else if(!have_fastchunks(av))
        assert(total == 0);

    /* check normal bins */
    for(i = 1; i < NBINS; ++i) {
        b = bin_at(av, i);

        /* binmap is accurate (except for bin 1 == unsorted_chunks) */
        if(i >= 2) {
            binbit = get_binmap(av, i);
            empty = last(b) == b;

            if(!binbit)
                assert(empty);
            else if(!empty)
                assert(binbit);
        }

        for(p = last(b); p != b; p = p->bk) {
            /* each chunk claims to be free */
            do_check_free_chunk(p);
            size = chunksize(p);
            total += size;

            if(i >= 2) {
                /* chunk belongs in bin */
                idx = bin_index(size);
                assert(idx == (unsigned)i);
                /* lists are sorted */
                assert(p->bk == b ||
                       (unsigned long)chunksize(p->bk) >= (unsigned long)chunksize(p));
            }

            /* chunk is followed by a legal chain of inuse chunks */
            for(q = next_chunk(p);
                    (q != av->top && inuse(q) &&
                     (unsigned long)(chunksize(q)) >= MINSIZE);
                    q = next_chunk(q))
                do_check_inuse_chunk(q);
        }
    }

    /* top chunk is OK */
    check_chunk(av->top);

    /* sanity checks for statistics */

    assert(total <= (unsigned long)(av->max_total_mem));
    assert(av->n_mmaps >= 0);
    assert(av->n_mmaps <= av->n_mmaps_max);
    assert(av->n_mmaps <= av->max_n_mmaps);

    assert((unsigned long)(av->sbrked_mem) <=
           (unsigned long)(av->max_sbrked_mem));

    assert((unsigned long)(av->mmapped_mem) <=
           (unsigned long)(av->max_mmapped_mem));

    assert((unsigned long)(av->max_total_mem) >=
           (unsigned long)(av->mmapped_mem) + (unsigned long)(av->sbrked_mem));
}
#endif


/* ----------- Routines dealing with system allocation -------------- */

/*
  sysmalloc handles malloc cases requiring more memory from the system.
  On entry, it is assumed that av->top does not have enough
  space to service request for nb bytes, thus requiring that av->top
  be extended or replaced.
*/

#if __STD_C
static Void_t* sYSMALLOc(INTERNAL_SIZE_T nb, mstate av)
#else
static Void_t* sYSMALLOc(nb, av) INTERNAL_SIZE_T nb;
mstate av;
#endif
{
    mchunkptr       old_top;        /* incoming value of av->top */
    INTERNAL_SIZE_T old_size;       /* its size */
    char*           old_end;        /* its end address */

    long            size;           /* arg to first MORECORE or mmap call */
    char*           brk;            /* return value from MORECORE */

    long            correction;     /* arg to 2nd MORECORE call */
    char*           snd_brk;        /* 2nd return val */

    INTERNAL_SIZE_T front_misalign; /* unusable bytes at front of new space */
    INTERNAL_SIZE_T end_misalign;   /* partial page left at end of new space */
    char*           aligned_brk;    /* aligned offset into brk */

    mchunkptr       p;              /* the allocated/returned chunk */
    mchunkptr       remainder;      /* remainder from allocation */
    unsigned long   remainder_size; /* its size */

    unsigned long   sum;            /* for updating stats */

    size_t          pagemask  = av->pagesize - 1;


#if HAVE_MMAP

    /*
      If have mmap, and the request size meets the mmap threshold, and
      the system supports mmap, and there are few enough currently
      allocated mmapped regions, try to directly map this request
      rather than expanding top.
    */

    if((unsigned long)(nb) >= (unsigned long)(av->mmap_threshold) &&
            (av->n_mmaps < av->n_mmaps_max)) {

        char* mm;             /* return value from mmap call*/

        /*
          Round up size to nearest page.  For mmapped chunks, the overhead
          is one SIZE_SZ unit larger than for normal chunks, because there
          is no following chunk whose prev_size field could be used.
        */
        size = (nb + SIZE_SZ + MALLOC_ALIGN_MASK + pagemask) & ~pagemask;

        /* Don't try if size wraps around 0 */
        if((unsigned long)(size) > (unsigned long)(nb)) {

            mm = (char*)(MMAP(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE));

            if(mm != (char*)(MORECORE_FAILURE)) {

                /*
                  The offset to the start of the mmapped region is stored
                  in the prev_size field of the chunk. This allows us to adjust
                  returned start address to meet alignment requirements here
                  and in memalign(), and still be able to compute proper
                  address argument for later munmap in free() and realloc().
                */

                front_misalign = (INTERNAL_SIZE_T)chunk2mem(mm) & MALLOC_ALIGN_MASK;

                if(front_misalign > 0) {
                    correction = MALLOC_ALIGNMENT - front_misalign;
                    p = (mchunkptr)(mm + correction);
                    p->prev_size = correction;
                    set_head(p, (size - correction) | IS_MMAPPED);
                }
                else {
                    p = (mchunkptr)mm;
                    set_head(p, size | IS_MMAPPED);
                }

                /* update statistics */

                if(++av->n_mmaps > av->max_n_mmaps)
                    av->max_n_mmaps = av->n_mmaps;

                sum = av->mmapped_mem += size;

                if(sum > (unsigned long)(av->max_mmapped_mem))
                    av->max_mmapped_mem = sum;

                sum += av->sbrked_mem;

                if(sum > (unsigned long)(av->max_total_mem))
                    av->max_total_mem = sum;

                check_chunk(p);

                return chunk2mem(p);
            }
        }
    }

#endif

    /* Record incoming configuration of top */

    old_top  = av->top;
    old_size = chunksize(old_top);
    old_end  = (char*)(chunk_at_offset(old_top, old_size));

    brk = snd_brk = (char*)(MORECORE_FAILURE);

    /*
       If not the first time through, we require old_size to be
       at least MINSIZE and to have prev_inuse set.
    */

    assert((old_top == initial_top(av) && old_size == 0) ||
           ((unsigned long)(old_size) >= MINSIZE &&
            prev_inuse(old_top)));

    /* Precondition: not enough current space to satisfy nb request */
    assert((unsigned long)(old_size) < (unsigned long)(nb + MINSIZE));

    /* Precondition: all fastbins are consolidated */
    assert(!have_fastchunks(av));

    /* Request enough space for nb + pad + overhead */

    size = nb + av->top_pad + MINSIZE;

    /*
      If contiguous, we can subtract out existing space that we hope to
      combine with new space. We add it back later only if
      we don't actually get contiguous space.
    */

    if(contiguous(av))
        size -= old_size;

    /*
      Round to a multiple of page size.
      If MORECORE is not contiguous, this ensures that we only call it
      with whole-page arguments.  And if MORECORE is contiguous and
      this is not first time through, this preserves page-alignment of
      previous calls. Otherwise, we correct to page-align below.
    */

    size = (size + pagemask) & ~pagemask;

    /*
      Don't try to call MORECORE if argument is so big as to appear
      negative. Note that since mmap takes size_t arg, it may succeed
      below even if we cannot call MORECORE.
    */

    if(size > 0)
        brk = (char*)(MORECORE(size));

    /*
      If have mmap, try using it as a backup when MORECORE fails or
      cannot be used. This is worth doing on systems that have "holes" in
      address space, so sbrk cannot extend to give contiguous space, but
      space is available elsewhere.  Note that we ignore mmap max count
      and threshold limits, since the space will not be used as a
      segregated mmap region.
    */

#if HAVE_MMAP

    if(brk == (char*)(MORECORE_FAILURE)) {

        /* Cannot merge with old top, so add its size back in */
        if(contiguous(av))
            size = (size + old_size + pagemask) & ~pagemask;

        /* If we are relying on mmap as backup, then use larger units */
        if((unsigned long)(size) < (unsigned long)(MMAP_AS_MORECORE_SIZE))
            size = MMAP_AS_MORECORE_SIZE;

        /* Don't try if size wraps around 0 */
        if((unsigned long)(size) > (unsigned long)(nb)) {

            brk = (char*)(MMAP(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE));

            if(brk != (char*)(MORECORE_FAILURE)) {

                /* We do not need, and cannot use, another sbrk call to find end */
                snd_brk = brk + size;

                /*
                   Record that we no longer have a contiguous sbrk region.
                   After the first time mmap is used as backup, we do not
                   ever rely on contiguous space since this could incorrectly
                   bridge regions.
                */
                set_noncontiguous(av);
            }
        }
    }

#endif

    if(brk != (char*)(MORECORE_FAILURE)) {
        av->sbrked_mem += size;

        /*
          If MORECORE extends previous space, we can likewise extend top size.
        */

        if(brk == old_end && snd_brk == (char*)(MORECORE_FAILURE)) {
            set_head(old_top, (size + old_size) | PREV_INUSE);
        }

        /*
          Otherwise, make adjustments:

          * If the first time through or noncontiguous, we need to call sbrk
            just to find out where the end of memory lies.

          * We need to ensure that all returned chunks from malloc will meet
            MALLOC_ALIGNMENT

          * If there was an intervening foreign sbrk, we need to adjust sbrk
            request size to account for fact that we will not be able to
            combine new space with existing space in old_top.

          * Almost all systems internally allocate whole pages at a time, in
            which case we might as well use the whole last page of request.
            So we allocate enough more memory to hit a page boundary now,
            which in turn causes future contiguous calls to page-align.
        */

        else {
            front_misalign = 0;
            end_misalign = 0;
            correction = 0;
            aligned_brk = brk;

            /* handle contiguous cases */
            if(contiguous(av)) {

                /* Guarantee alignment of first new chunk made from this space */

                front_misalign = (INTERNAL_SIZE_T)chunk2mem(brk) & MALLOC_ALIGN_MASK;

                if(front_misalign > 0) {

                    /*
                      Skip over some bytes to arrive at an aligned position.
                      We don't need to specially mark these wasted front bytes.
                      They will never be accessed anyway because
                      prev_inuse of av->top (and any chunk created from its start)
                      is always true after initialization.
                    */

                    correction = MALLOC_ALIGNMENT - front_misalign;
                    aligned_brk += correction;
                }

                /*
                  If this isn't adjacent to existing space, then we will not
                  be able to merge with old_top space, so must add to 2nd request.
                */

                correction += old_size;

                /* Extend the end address to hit a page boundary */
                end_misalign = (INTERNAL_SIZE_T)(brk + size + correction);
                correction += ((end_misalign + pagemask) & ~pagemask) - end_misalign;

                assert(correction >= 0);
                snd_brk = (char*)(MORECORE(correction));

                /*
                  If can't allocate correction, try to at least find out current
                  brk.  It might be enough to proceed without failing.

                  Note that if second sbrk did NOT fail, we assume that space
                  is contiguous with first sbrk. This is a safe assumption unless
                  program is multithreaded but doesn't use locks and a foreign sbrk
                  occurred between our first and second calls.
                */

                if(snd_brk == (char*)(MORECORE_FAILURE)) {
                    correction = 0;
                    snd_brk = (char*)(MORECORE(0));
                }
            }

            /* handle non-contiguous cases */
            else {
                /* MORECORE/mmap must correctly align */
                assert(((unsigned long)chunk2mem(brk) & MALLOC_ALIGN_MASK) == 0);

                /* Find out current end of memory */
                if(snd_brk == (char*)(MORECORE_FAILURE)) {
                    snd_brk = (char*)(MORECORE(0));
                }
            }

            /* Adjust top based on results of second sbrk */
            if(snd_brk != (char*)(MORECORE_FAILURE)) {
                av->top = (mchunkptr)aligned_brk;
                set_head(av->top, (snd_brk - aligned_brk + correction) | PREV_INUSE);
                av->sbrked_mem += correction;

                /*
                  If not the first time through, we either have a
                  gap due to foreign sbrk or a non-contiguous region.  Insert a
                  double fencepost at old_top to prevent consolidation with space
                  we don't own. These fenceposts are artificial chunks that are
                  marked as inuse and are in any case too small to use.  We need
                  two to make sizes and alignments work out.
                */

                if(old_size != 0) {
                    /*
                       Shrink old_top to insert fenceposts, keeping size a
                       multiple of MALLOC_ALIGNMENT. We know there is at least
                       enough space in old_top to do this.
                    */
                    old_size = (old_size - 3 * SIZE_SZ) & ~MALLOC_ALIGN_MASK;
                    set_head(old_top, old_size | PREV_INUSE);

                    /*
                      Note that the following assignments completely overwrite
                      old_top when old_size was previously MINSIZE.  This is
                      intentional. We need the fencepost, even if old_top otherwise gets
                      lost.
                    */
                    chunk_at_offset(old_top, old_size)->size =
                        SIZE_SZ | PREV_INUSE;

                    chunk_at_offset(old_top, old_size + SIZE_SZ)->size =
                        SIZE_SZ | PREV_INUSE;

                    /* If possible, release the rest. */
                    if(old_size >= MINSIZE) {
                        fREe(chunk2mem(old_top));
                    }

                }
            }
        }

        /* Update statistics */
        sum = av->sbrked_mem;

        if(sum > (unsigned long)(av->max_sbrked_mem))
            av->max_sbrked_mem = sum;

        sum += av->mmapped_mem;

        if(sum > (unsigned long)(av->max_total_mem))
            av->max_total_mem = sum;

        check_malloc_state();

        /* finally, do the allocation */
        p = av->top;
        size = chunksize(p);

        /* check that one of the above allocation paths succeeded */
        if((unsigned long)(size) >= (unsigned long)(nb + MINSIZE)) {
            remainder_size = size - nb;
            remainder = chunk_at_offset(p, nb);
            av->top = remainder;
            set_head(p, nb | PREV_INUSE);
            set_head(remainder, remainder_size | PREV_INUSE);
            check_malloced_chunk(p, nb);
            return chunk2mem(p);
        }
    }

    /* catch all failure paths */
    MALLOC_FAILURE_ACTION;
    return 0;
}


/*
  sYSTRIm is an inverse of sorts to sYSMALLOc.  It gives memory back
  to the system (via negative arguments to sbrk) if there is unused
  memory at the `high' end of the malloc pool. It is called
  automatically by free() when top space exceeds the trim
  threshold. It is also called by the public malloc_trim routine.  It
  returns 1 if it actually released any memory, else 0.
*/

#if __STD_C
static int sYSTRIm(size_t pad, mstate av)
#else
static int sYSTRIm(pad, av) size_t pad;
mstate av;
#endif
{
    long  top_size;        /* Amount of top-most memory */
    long  extra;           /* Amount to release */
    long  released;        /* Amount actually released */
    char* current_brk;     /* address returned by pre-check sbrk call */
    char* new_brk;         /* address returned by post-check sbrk call */
    size_t pagesz;

    pagesz = av->pagesize;
    top_size = chunksize(av->top);

    /* Release in pagesize units, keeping at least one page */
    extra = ((top_size - pad - MINSIZE + (pagesz - 1)) / pagesz - 1) * pagesz;

    if(extra > 0) {

        /*
          Only proceed if end of memory is where we last set it.
          This avoids problems if there were foreign sbrk calls.
        */
        current_brk = (char*)(MORECORE(0));

        if(current_brk == (char*)(av->top) + top_size) {

            /*
              Attempt to release memory. We ignore MORECORE return value,
              and instead call again to find out where new end of memory is.
              This avoids problems if first call releases less than we asked,
              of if failure somehow altered brk value. (We could still
              encounter problems if it altered brk in some very bad way,
              but the only thing we can do is adjust anyway, which will cause
              some downstream failure.)
            */

            MORECORE(-extra);
            new_brk = (char*)(MORECORE(0));

            if(new_brk != (char*)MORECORE_FAILURE) {
                released = (long)(current_brk - new_brk);

                if(released != 0) {
                    /* Success. Adjust top. */
                    av->sbrked_mem -= released;
                    set_head(av->top, (top_size - released) | PREV_INUSE);
                    check_malloc_state();
                    return 1;
                }
            }
        }
    }

    return 0;
}

/*
  ------------------------------ malloc ------------------------------
*/

#if __STD_C
Void_t* mALLOc(size_t bytes)
#else
Void_t* mALLOc(bytes) size_t bytes;
#endif
{
    mstate av = get_malloc_state();

    INTERNAL_SIZE_T nb;               /* normalized request size */
    unsigned int    idx;              /* associated bin index */
    mbinptr         bin;              /* associated bin */
    mfastbinptr*    fb;               /* associated fastbin */

    mchunkptr       victim;           /* inspected/selected chunk */
    INTERNAL_SIZE_T size;             /* its size */
    int             victim_index;     /* its bin index */

    mchunkptr       remainder;        /* remainder from a split */
    unsigned long   remainder_size;   /* its size */

    unsigned int    block;            /* bit map traverser */
    unsigned int    bit;              /* bit map traverser */
    unsigned int    map;              /* current word of binmap */

    mchunkptr       fwd;              /* misc temp for linking */
    mchunkptr       bck;              /* misc temp for linking */

    /*
      Convert request size to internal form by adding SIZE_SZ bytes
      overhead plus possibly more to obtain necessary alignment and/or
      to obtain a size of at least MINSIZE, the smallest allocatable
      size. Also, checked_request2size traps (returning 0) request sizes
      that are so large that they wrap around zero when padded and
      aligned.
    */

    checked_request2size(bytes, nb);

    /*
      If the size qualifies as a fastbin, first check corresponding bin.
      This code is safe to execute even if av is not yet initialized, so we
      can try it without checking, which saves some time on this fast path.
    */

    if((unsigned long)(nb) <= (unsigned long)(av->max_fast)) {
        fb = &(av->fastbins[(fastbin_index(nb))]);

        if((victim = *fb) != 0) {
            *fb = victim->fd;
            check_remalloced_chunk(victim, nb);
            return chunk2mem(victim);
        }
    }

    /*
      If a small request, check regular bin.  Since these "smallbins"
      hold one size each, no searching within bins is necessary.
      (For a large request, we need to wait until unsorted chunks are
      processed to find best fit. But for small ones, fits are exact
      anyway, so we can check now, which is faster.)
    */

    if(in_smallbin_range(nb)) {
        idx = smallbin_index(nb);
        bin = bin_at(av, idx);

        if((victim = last(bin)) != bin) {
            if(victim == 0)  /* initialization check */
                malloc_consolidate(av);
            else {
                bck = victim->bk;
                set_inuse_bit_at_offset(victim, nb);
                bin->bk = bck;
                bck->fd = bin;

                check_malloced_chunk(victim, nb);
                return chunk2mem(victim);
            }
        }
    }

    /*
       If this is a large request, consolidate fastbins before continuing.
       While it might look excessive to kill all fastbins before
       even seeing if there is space available, this avoids
       fragmentation problems normally associated with fastbins.
       Also, in practice, programs tend to have runs of either small or
       large requests, but less often mixtures, so consolidation is not
       invoked all that often in most programs. And the programs that
       it is called frequently in otherwise tend to fragment.
    */

    else {
        idx = largebin_index(nb);

        if(have_fastchunks(av))
            malloc_consolidate(av);
    }

    /*
      Process recently freed or remaindered chunks, taking one only if
      it is exact fit, or, if this a small request, the chunk is remainder from
      the most recent non-exact fit.  Place other traversed chunks in
      bins.  Note that this step is the only place in any routine where
      chunks are placed in bins.

      The outer loop here is needed because we might not realize until
      near the end of malloc that we should have consolidated, so must
      do so and retry. This happens at most once, and only when we would
      otherwise need to expand memory to service a "small" request.
    */

    for(;;) {

        while((victim = unsorted_chunks(av)->bk) != unsorted_chunks(av)) {
            bck = victim->bk;
            size = chunksize(victim);

            /*
               If a small request, try to use last remainder if it is the
               only chunk in unsorted bin.  This helps promote locality for
               runs of consecutive small requests. This is the only
               exception to best-fit, and applies only when there is
               no exact fit for a small chunk.
            */

            if(in_smallbin_range(nb) &&
                    bck == unsorted_chunks(av) &&
                    victim == av->last_remainder &&
                    (unsigned long)(size) > (unsigned long)(nb + MINSIZE)) {

                /* split and reattach remainder */
                remainder_size = size - nb;
                remainder = chunk_at_offset(victim, nb);
                unsorted_chunks(av)->bk = unsorted_chunks(av)->fd = remainder;
                av->last_remainder = remainder;
                remainder->bk = remainder->fd = unsorted_chunks(av);

                set_head(victim, nb | PREV_INUSE);
                set_head(remainder, remainder_size | PREV_INUSE);
                set_foot(remainder, remainder_size);

                check_malloced_chunk(victim, nb);
                return chunk2mem(victim);
            }

            /* remove from unsorted list */
            unsorted_chunks(av)->bk = bck;
            bck->fd = unsorted_chunks(av);

            /* Take now instead of binning if exact fit */

            if(size == nb) {
                set_inuse_bit_at_offset(victim, size);
                check_malloced_chunk(victim, nb);
                return chunk2mem(victim);
            }

            /* place chunk in bin */

            if(in_smallbin_range(size)) {
                victim_index = smallbin_index(size);
                bck = bin_at(av, victim_index);
                fwd = bck->fd;
            }
            else {
                victim_index = largebin_index(size);
                bck = bin_at(av, victim_index);
                fwd = bck->fd;

                /* maintain large bins in sorted order */
                if(fwd != bck) {
                    size |= PREV_INUSE; /* Or with inuse bit to speed comparisons */

                    /* if smaller than smallest, bypass loop below */
                    if((unsigned long)(size) <= (unsigned long)(bck->bk->size)) {
                        fwd = bck;
                        bck = bck->bk;
                    }
                    else {
                        while((unsigned long)(size) < (unsigned long)(fwd->size))
                            fwd = fwd->fd;

                        bck = fwd->bk;
                    }
                }
            }

            mark_bin(av, victim_index);
            victim->bk = bck;
            victim->fd = fwd;
            fwd->bk = victim;
            bck->fd = victim;
        }

        /*
          If a large request, scan through the chunks of current bin in
          sorted order to find smallest that fits.  This is the only step
          where an unbounded number of chunks might be scanned without doing
          anything useful with them. However the lists tend to be short.
        */

        if(!in_smallbin_range(nb)) {
            bin = bin_at(av, idx);

            /* skip scan if empty or largest chunk is too small */
            if((victim = last(bin)) != bin &&
                    (unsigned long)(first(bin)->size) >= (unsigned long)(nb)) {

                while(((unsigned long)(size = chunksize(victim)) <
                        (unsigned long)(nb)))
                    victim = victim->bk;

                remainder_size = size - nb;
                unlink(victim, bck, fwd);

                /* Exhaust */
                if(remainder_size < MINSIZE) {
                    set_inuse_bit_at_offset(victim, size);
                    check_malloced_chunk(victim, nb);
                    return chunk2mem(victim);
                }
                /* Split */
                else {
                    remainder = chunk_at_offset(victim, nb);
                    unsorted_chunks(av)->bk = unsorted_chunks(av)->fd = remainder;
                    remainder->bk = remainder->fd = unsorted_chunks(av);
                    set_head(victim, nb | PREV_INUSE);
                    set_head(remainder, remainder_size | PREV_INUSE);
                    set_foot(remainder, remainder_size);
                    check_malloced_chunk(victim, nb);
                    return chunk2mem(victim);
                }
            }
        }

        /*
          Search for a chunk by scanning bins, starting with next largest
          bin. This search is strictly by best-fit; i.e., the smallest
          (with ties going to approximately the least recently used) chunk
          that fits is selected.

          The bitmap avoids needing to check that most blocks are nonempty.
          The particular case of skipping all bins during warm-up phases
          when no chunks have been returned yet is faster than it might look.
        */

        ++idx;
        bin = bin_at(av, idx);
        block = idx2block(idx);
        map = av->binmap[block];
        bit = idx2bit(idx);

        for(;;) {

            /* Skip rest of block if there are no more set bits in this block.  */
            if(bit > map || bit == 0) {
                do {
                    if(++block >= BINMAPSIZE)   /* out of bins */
                        goto use_top;
                }
                while((map = av->binmap[block]) == 0);

                bin = bin_at(av, (block << BINMAPSHIFT));
                bit = 1;
            }

            /* Advance to bin with set bit. There must be one. */
            while((bit & map) == 0) {
                bin = next_bin(bin);
                bit <<= 1;
                assert(bit != 0);
            }

            /* Inspect the bin. It is likely to be non-empty */
            victim = last(bin);

            /*  If a false alarm (empty bin), clear the bit. */
            if(victim == bin) {
                av->binmap[block] = map &= ~bit; /* Write through */
                bin = next_bin(bin);
                bit <<= 1;
            }

            else {
                size = chunksize(victim);

                /*  We know the first chunk in this bin is big enough to use. */
                assert((unsigned long)(size) >= (unsigned long)(nb));

                remainder_size = size - nb;

                /* unlink */
                bck = victim->bk;
                bin->bk = bck;
                bck->fd = bin;

                /* Exhaust */
                if(remainder_size < MINSIZE) {
                    set_inuse_bit_at_offset(victim, size);
                    check_malloced_chunk(victim, nb);
                    return chunk2mem(victim);
                }

                /* Split */
                else {
                    remainder = chunk_at_offset(victim, nb);

                    unsorted_chunks(av)->bk = unsorted_chunks(av)->fd = remainder;
                    remainder->bk = remainder->fd = unsorted_chunks(av);

                    /* advertise as last remainder */
                    if(in_smallbin_range(nb))
                        av->last_remainder = remainder;

                    set_head(victim, nb | PREV_INUSE);
                    set_head(remainder, remainder_size | PREV_INUSE);
                    set_foot(remainder, remainder_size);
                    check_malloced_chunk(victim, nb);
                    return chunk2mem(victim);
                }
            }
        }

    use_top:
        /*
          If large enough, split off the chunk bordering the end of memory
          (held in av->top). Note that this is in accord with the best-fit
          search rule.  In effect, av->top is treated as larger (and thus
          less well fitting) than any other available chunk since it can
          be extended to be as large as necessary (up to system
          limitations).

          We require that av->top always exists (i.e., has size >=
          MINSIZE) after initialization, so if it would otherwise be
          exhuasted by current request, it is replenished. (The main
          reason for ensuring it exists is that we may need MINSIZE space
          to put in fenceposts in sysmalloc.)
        */

        victim = av->top;
        size = chunksize(victim);

        if((unsigned long)(size) >= (unsigned long)(nb + MINSIZE)) {
            remainder_size = size - nb;
            remainder = chunk_at_offset(victim, nb);
            av->top = remainder;
            set_head(victim, nb | PREV_INUSE);
            set_head(remainder, remainder_size | PREV_INUSE);

            check_malloced_chunk(victim, nb);
            return chunk2mem(victim);
        }

        /*
          If there is space available in fastbins, consolidate and retry,
          to possibly avoid expanding memory. This can occur only if nb is
          in smallbin range so we didn't consolidate upon entry.
        */

        else if(have_fastchunks(av)) {
            assert(in_smallbin_range(nb));
            malloc_consolidate(av);
            idx = smallbin_index(nb); /* restore original bin index */
        }

        /*
           Otherwise, relay to handle system-dependent cases
        */
        else
            return sYSMALLOc(nb, av);
    }
}

/*
  ------------------------------ free ------------------------------
*/

#if __STD_C
void fREe(Void_t* mem)
#else
void fREe(mem) Void_t* mem;
#endif
{
    mstate av = get_malloc_state();

    mchunkptr       p;           /* chunk corresponding to mem */
    INTERNAL_SIZE_T size;        /* its size */
    mfastbinptr*    fb;          /* associated fastbin */
    mchunkptr       nextchunk;   /* next contiguous chunk */
    INTERNAL_SIZE_T nextsize;    /* its size */
    int             nextinuse;   /* true if nextchunk is used */
    INTERNAL_SIZE_T prevsize;    /* size of previous contiguous chunk */
    mchunkptr       bck;         /* misc temp for linking */
    mchunkptr       fwd;         /* misc temp for linking */


    /* free(0) has no effect */
    if(mem != 0) {
        p = mem2chunk(mem);
        size = chunksize(p);

        check_inuse_chunk(p);

        /*
          If eligible, place chunk on a fastbin so it can be found
          and used quickly in malloc.
        */

        if((unsigned long)(size) <= (unsigned long)(av->max_fast)

#if TRIM_FASTBINS
                /*
                   If TRIM_FASTBINS set, don't place chunks
                   bordering top into fastbins
                */
                && (chunk_at_offset(p, size) != av->top)
#endif
          ) {

            set_fastchunks(av);
            fb = &(av->fastbins[fastbin_index(size)]);
            p->fd = *fb;
            *fb = p;
        }

        /*
           Consolidate other non-mmapped chunks as they arrive.
        */

        else if(!chunk_is_mmapped(p)) {
            nextchunk = chunk_at_offset(p, size);
            nextsize = chunksize(nextchunk);

            /* consolidate backward */
            if(!prev_inuse(p)) {
                prevsize = p->prev_size;
                size += prevsize;
                p = chunk_at_offset(p, -((long) prevsize));
                unlink(p, bck, fwd);
            }

            if(nextchunk != av->top) {
                /* get and clear inuse bit */
                nextinuse = inuse_bit_at_offset(nextchunk, nextsize);
                set_head(nextchunk, nextsize);

                /* consolidate forward */
                if(!nextinuse) {
                    unlink(nextchunk, bck, fwd);
                    size += nextsize;
                }

                /*
                  Place the chunk in unsorted chunk list. Chunks are
                  not placed into regular bins until after they have
                  been given one chance to be used in malloc.
                */

                bck = unsorted_chunks(av);
                fwd = bck->fd;
                p->bk = bck;
                p->fd = fwd;
                bck->fd = p;
                fwd->bk = p;

                set_head(p, size | PREV_INUSE);
                set_foot(p, size);

                check_free_chunk(p);
            }

            /*
               If the chunk borders the current high end of memory,
               consolidate into top
            */

            else {
                size += nextsize;
                set_head(p, size | PREV_INUSE);
                av->top = p;
                check_chunk(p);
            }

            /*
              If freeing a large space, consolidate possibly-surrounding
              chunks. Then, if the total unused topmost memory exceeds trim
              threshold, ask malloc_trim to reduce top.

              Unless max_fast is 0, we don't know if there are fastbins
              bordering top, so we cannot tell for sure whether threshold
              has been reached unless fastbins are consolidated.  But we
              don't want to consolidate on each free.  As a compromise,
              consolidation is performed if FASTBIN_CONSOLIDATION_THRESHOLD
              is reached.
            */

            if((unsigned long)(size) >= FASTBIN_CONSOLIDATION_THRESHOLD) {
                if(have_fastchunks(av))
                    malloc_consolidate(av);

#ifndef MORECORE_CANNOT_TRIM

                if((unsigned long)(chunksize(av->top)) >=
                        (unsigned long)(av->trim_threshold))
                    sYSTRIm(av->top_pad, av);

#endif
            }

        }

        /*
          If the chunk was allocated via mmap, release via munmap()
          Note that if HAVE_MMAP is false but chunk_is_mmapped is
          true, then user must have overwritten memory. There's nothing
          we can do to catch this error unless DEBUG is set, in which case
          check_inuse_chunk (above) will have triggered error.
        */

        else {
#if HAVE_MMAP
            int ret;
            INTERNAL_SIZE_T offset = p->prev_size;
            av->n_mmaps--;
            av->mmapped_mem -= (size + offset);
            ret = munmap((char*)p - offset, size + offset);
            /* munmap returns non-zero on failure */
            assert(ret == 0);
#endif
        }
    }
}

/*
  ------------------------- malloc_consolidate -------------------------

  malloc_consolidate is a specialized version of free() that tears
  down chunks held in fastbins.  Free itself cannot be used for this
  purpose since, among other things, it might place chunks back onto
  fastbins.  So, instead, we need to use a minor variant of the same
  code.

  Also, because this routine needs to be called the first time through
  malloc anyway, it turns out to be the perfect place to trigger
  initialization code.
*/

#if __STD_C
static void malloc_consolidate(mstate av)
#else
static void malloc_consolidate(av) mstate av;
#endif
{
    mfastbinptr*    fb;                 /* current fastbin being consolidated */
    mfastbinptr*    maxfb;              /* last fastbin (for loop control) */
    mchunkptr       p;                  /* current chunk being consolidated */
    mchunkptr       nextp;              /* next chunk to consolidate */
    mchunkptr       unsorted_bin;       /* bin header */
    mchunkptr       first_unsorted;     /* chunk to link to */

    /* These have same use as in free() */
    mchunkptr       nextchunk;
    INTERNAL_SIZE_T size;
    INTERNAL_SIZE_T nextsize;
    INTERNAL_SIZE_T prevsize;
    int             nextinuse;
    mchunkptr       bck;
    mchunkptr       fwd;

    /*
      If max_fast is 0, we know that av hasn't
      yet been initialized, in which case do so below
    */

    if(av->max_fast != 0) {
        clear_fastchunks(av);

        unsorted_bin = unsorted_chunks(av);

        /*
          Remove each chunk from fast bin and consolidate it, placing it
          then in unsorted bin. Among other reasons for doing this,
          placing in unsorted bin avoids needing to calculate actual bins
          until malloc is sure that chunks aren't immediately going to be
          reused anyway.
        */

        maxfb = &(av->fastbins[fastbin_index(av->max_fast)]);
        fb = &(av->fastbins[0]);

        do {
            if((p = *fb) != 0) {
                *fb = 0;

                do {
                    check_inuse_chunk(p);
                    nextp = p->fd;

                    /* Slightly streamlined version of consolidation code in free() */
                    size = p->size & ~PREV_INUSE;
                    nextchunk = chunk_at_offset(p, size);
                    nextsize = chunksize(nextchunk);

                    if(!prev_inuse(p)) {
                        prevsize = p->prev_size;
                        size += prevsize;
                        p = chunk_at_offset(p, -((long) prevsize));
                        unlink(p, bck, fwd);
                    }

                    if(nextchunk != av->top) {
                        nextinuse = inuse_bit_at_offset(nextchunk, nextsize);
                        set_head(nextchunk, nextsize);

                        if(!nextinuse) {
                            size += nextsize;
                            unlink(nextchunk, bck, fwd);
                        }

                        first_unsorted = unsorted_bin->fd;
                        unsorted_bin->fd = p;
                        first_unsorted->bk = p;

                        set_head(p, size | PREV_INUSE);
                        p->bk = unsorted_bin;
                        p->fd = first_unsorted;
                        set_foot(p, size);
                    }

                    else {
                        size += nextsize;
                        set_head(p, size | PREV_INUSE);
                        av->top = p;
                    }

                }
                while((p = nextp) != 0);

            }
        }
        while(fb++ != maxfb);
    }
    else {
        malloc_init_state(av);
        check_malloc_state();
    }
}

/*
  ------------------------------ realloc ------------------------------
*/


#if __STD_C
Void_t* rEALLOc(Void_t* oldmem, size_t bytes)
#else
Void_t* rEALLOc(oldmem, bytes) Void_t* oldmem;
size_t bytes;
#endif
{
    mstate av = get_malloc_state();

    INTERNAL_SIZE_T  nb;              /* padded request size */

    mchunkptr        oldp;            /* chunk corresponding to oldmem */
    INTERNAL_SIZE_T  oldsize;         /* its size */

    mchunkptr        newp;            /* chunk to return */
    INTERNAL_SIZE_T  newsize;         /* its size */
    Void_t*          newmem;          /* corresponding user mem */

    mchunkptr        next;            /* next contiguous chunk after oldp */

    mchunkptr        remainder;       /* extra space at end of newp */
    unsigned long    remainder_size;  /* its size */

    mchunkptr        bck;             /* misc temp for linking */
    mchunkptr        fwd;             /* misc temp for linking */

    unsigned long    copysize;        /* bytes to copy */
    unsigned int     ncopies;         /* INTERNAL_SIZE_T words to copy */
    INTERNAL_SIZE_T* s;               /* copy source */
    INTERNAL_SIZE_T* d;               /* copy destination */


#ifdef REALLOC_ZERO_BYTES_FREES

    if(bytes == 0) {
        fREe(oldmem);
        return 0;
    }

#endif

    /* realloc of null is supposed to be same as malloc */
    if(oldmem == 0) return mALLOc(bytes);

    checked_request2size(bytes, nb);

    oldp    = mem2chunk(oldmem);
    oldsize = chunksize(oldp);

    check_inuse_chunk(oldp);

    if(!chunk_is_mmapped(oldp)) {

        if((unsigned long)(oldsize) >= (unsigned long)(nb)) {
            /* already big enough; split below */
            newp = oldp;
            newsize = oldsize;
        }

        else {
            next = chunk_at_offset(oldp, oldsize);

            /* Try to expand forward into top */
            if(next == av->top &&
                    (unsigned long)(newsize = oldsize + chunksize(next)) >=
                    (unsigned long)(nb + MINSIZE)) {
                set_head_size(oldp, nb);
                av->top = chunk_at_offset(oldp, nb);
                set_head(av->top, (newsize - nb) | PREV_INUSE);
                return chunk2mem(oldp);
            }

            /* Try to expand forward into next chunk;  split off remainder below */
            else if(next != av->top &&
                    !inuse(next) &&
                    (unsigned long)(newsize = oldsize + chunksize(next)) >=
                    (unsigned long)(nb)) {
                newp = oldp;
                unlink(next, bck, fwd);
            }

            /* allocate, copy, free */
            else {
                newmem = mALLOc(nb - MALLOC_ALIGN_MASK);

                if(newmem == 0)
                    return 0; /* propagate failure */

                newp = mem2chunk(newmem);
                newsize = chunksize(newp);

                /*
                  Avoid copy if newp is next chunk after oldp.
                */
                if(newp == next) {
                    newsize += oldsize;
                    newp = oldp;
                }
                else {
                    /*
                      Unroll copy of <= 36 bytes (72 if 8byte sizes)
                      We know that contents have an odd number of
                      INTERNAL_SIZE_T-sized words; minimally 3.
                    */

                    copysize = oldsize - SIZE_SZ;
                    s = (INTERNAL_SIZE_T*)(oldmem);
                    d = (INTERNAL_SIZE_T*)(newmem);
                    ncopies = copysize / sizeof(INTERNAL_SIZE_T);
                    assert(ncopies >= 3);

                    if(ncopies > 9)
                        MALLOC_COPY(d, s, copysize);

                    else {
                        *(d + 0) = *(s + 0);
                        *(d + 1) = *(s + 1);
                        *(d + 2) = *(s + 2);

                        if(ncopies > 4) {
                            *(d + 3) = *(s + 3);
                            *(d + 4) = *(s + 4);

                            if(ncopies > 6) {
                                *(d + 5) = *(s + 5);
                                *(d + 6) = *(s + 6);

                                if(ncopies > 8) {
                                    *(d + 7) = *(s + 7);
                                    *(d + 8) = *(s + 8);
                                }
                            }
                        }
                    }

                    fREe(oldmem);
                    check_inuse_chunk(newp);
                    return chunk2mem(newp);
                }
            }
        }

        /* If possible, free extra space in old or extended chunk */

        assert((unsigned long)(newsize) >= (unsigned long)(nb));

        remainder_size = newsize - nb;

        if(remainder_size < MINSIZE) {  /* not enough extra to split off */
            set_head_size(newp, newsize);
            set_inuse_bit_at_offset(newp, newsize);
        }
        else { /* split remainder */
            remainder = chunk_at_offset(newp, nb);
            set_head_size(newp, nb);
            set_head(remainder, remainder_size | PREV_INUSE);
            /* Mark remainder as inuse so free() won't complain */
            set_inuse_bit_at_offset(remainder, remainder_size);
            fREe(chunk2mem(remainder));
        }

        check_inuse_chunk(newp);
        return chunk2mem(newp);
    }

    /*
      Handle mmap cases
    */

    else {
#if HAVE_MMAP

#if HAVE_MREMAP
        INTERNAL_SIZE_T offset = oldp->prev_size;
        size_t pagemask = av->pagesize - 1;
        char *cp;
        unsigned long sum;

        /* Note the extra SIZE_SZ overhead */
        newsize = (nb + offset + SIZE_SZ + pagemask) & ~pagemask;

        /* don't need to remap if still within same page */
        if(oldsize == newsize - offset)
            return oldmem;

        cp = (char*)mremap((char*)oldp - offset, oldsize + offset, newsize, 1);

        if(cp != (char*)MORECORE_FAILURE) {

            newp = (mchunkptr)(cp + offset);
            set_head(newp, (newsize - offset) | IS_MMAPPED);

            assert(aligned_OK(chunk2mem(newp)));
            assert((newp->prev_size == offset));

            /* update statistics */
            sum = av->mmapped_mem += newsize - oldsize;

            if(sum > (unsigned long)(av->max_mmapped_mem))
                av->max_mmapped_mem = sum;

            sum += av->sbrked_mem;

            if(sum > (unsigned long)(av->max_total_mem))
                av->max_total_mem = sum;

            return chunk2mem(newp);
        }

#endif

        /* Note the extra SIZE_SZ overhead. */
        if((unsigned long)(oldsize) >= (unsigned long)(nb + SIZE_SZ))
            newmem = oldmem; /* do nothing */
        else {
            /* Must alloc, copy, free. */
            newmem = mALLOc(nb - MALLOC_ALIGN_MASK);

            if(newmem != 0) {
                MALLOC_COPY(newmem, oldmem, oldsize - 2 * SIZE_SZ);
                fREe(oldmem);
            }
        }

        return newmem;

#else
        /* If !HAVE_MMAP, but chunk_is_mmapped, user must have overwritten mem */
        check_malloc_state();
        MALLOC_FAILURE_ACTION;
        return 0;
#endif
    }
}

/*
  ------------------------------ memalign ------------------------------
*/

#if __STD_C
Void_t* mEMALIGn(size_t alignment, size_t bytes)
#else
Void_t* mEMALIGn(alignment, bytes) size_t alignment;
size_t bytes;
#endif
{
    INTERNAL_SIZE_T nb;             /* padded  request size */
    char*           m;              /* memory returned by malloc call */
    mchunkptr       p;              /* corresponding chunk */
    char*           brk;            /* alignment point within p */
    mchunkptr       newp;           /* chunk to return */
    INTERNAL_SIZE_T newsize;        /* its size */
    INTERNAL_SIZE_T leadsize;       /* leading space before alignment point */
    mchunkptr       remainder;      /* spare room at end to split off */
    unsigned long   remainder_size; /* its size */
    INTERNAL_SIZE_T size;

    /* If need less alignment than we give anyway, just relay to malloc */

    if(alignment <= MALLOC_ALIGNMENT) return mALLOc(bytes);

    /* Otherwise, ensure that it is at least a minimum chunk size */

    if(alignment <  MINSIZE) alignment = MINSIZE;

    /* Make sure alignment is power of 2 (in case MINSIZE is not).  */
    if((alignment & (alignment - 1)) != 0) {
        size_t a = MALLOC_ALIGNMENT * 2;

        while((unsigned long)a < (unsigned long)alignment) a <<= 1;

        alignment = a;
    }

    checked_request2size(bytes, nb);

    /*
      Strategy: find a spot within that chunk that meets the alignment
      request, and then possibly free the leading and trailing space.
    */


    /* Call malloc with worst case padding to hit alignment. */

    m  = (char*)(mALLOc(nb + alignment + MINSIZE));

    if(m == 0) return 0;  /* propagate failure */

    p = mem2chunk(m);

    if((((unsigned long)(m)) % alignment) != 0) {  /* misaligned */

        /*
          Find an aligned spot inside chunk.  Since we need to give back
          leading space in a chunk of at least MINSIZE, if the first
          calculation places us at a spot with less than MINSIZE leader,
          we can move to the next aligned spot -- we've allocated enough
          total room so that this is always possible.
        */

        brk = (char*)mem2chunk(((unsigned long)(m + alignment - 1)) &
                               -((signed long) alignment));

        if((unsigned long)(brk - (char*)(p)) < MINSIZE)
            brk += alignment;

        newp = (mchunkptr)brk;
        leadsize = brk - (char*)(p);
        newsize = chunksize(p) - leadsize;

        /* For mmapped chunks, just adjust offset */
        if(chunk_is_mmapped(p)) {
            newp->prev_size = p->prev_size + leadsize;
            set_head(newp, newsize | IS_MMAPPED);
            return chunk2mem(newp);
        }

        /* Otherwise, give back leader, use the rest */
        set_head(newp, newsize | PREV_INUSE);
        set_inuse_bit_at_offset(newp, newsize);
        set_head_size(p, leadsize);
        fREe(chunk2mem(p));
        p = newp;

        assert(newsize >= nb &&
               (((unsigned long)(chunk2mem(p))) % alignment) == 0);
    }

    /* Also give back spare room at the end */
    if(!chunk_is_mmapped(p)) {
        size = chunksize(p);

        if((unsigned long)(size) > (unsigned long)(nb + MINSIZE)) {
            remainder_size = size - nb;
            remainder = chunk_at_offset(p, nb);
            set_head(remainder, remainder_size | PREV_INUSE);
            set_head_size(p, nb);
            fREe(chunk2mem(remainder));
        }
    }

    check_inuse_chunk(p);
    return chunk2mem(p);
}

/*
  ------------------------------ calloc ------------------------------
*/

#if __STD_C
Void_t* cALLOc(size_t n_elements, size_t elem_size)
#else
Void_t* cALLOc(n_elements, elem_size) size_t n_elements;
size_t elem_size;
#endif
{
    mchunkptr p;
    unsigned long clearsize;
    unsigned long nclears;
    INTERNAL_SIZE_T* d;

    Void_t* mem = mALLOc(n_elements * elem_size);

    if(mem != 0) {
        p = mem2chunk(mem);

#if MMAP_CLEARS

        if(!chunk_is_mmapped(p))  /* don't need to clear mmapped space */
#endif
        {
            /*
              Unroll clear of <= 36 bytes (72 if 8byte sizes)
              We know that contents have an odd number of
              INTERNAL_SIZE_T-sized words; minimally 3.
            */

            d = (INTERNAL_SIZE_T*)mem;
            clearsize = chunksize(p) - SIZE_SZ;
            nclears = clearsize / sizeof(INTERNAL_SIZE_T);
            assert(nclears >= 3);

            if(nclears > 9)
                MALLOC_ZERO(d, clearsize);

            else {
                *(d + 0) = 0;
                *(d + 1) = 0;
                *(d + 2) = 0;

                if(nclears > 4) {
                    *(d + 3) = 0;
                    *(d + 4) = 0;

                    if(nclears > 6) {
                        *(d + 5) = 0;
                        *(d + 6) = 0;

                        if(nclears > 8) {
                            *(d + 7) = 0;
                            *(d + 8) = 0;
                        }
                    }
                }
            }
        }
    }

    return mem;
}

/*
  ------------------------------ cfree ------------------------------
*/

#if __STD_C
void cFREe(Void_t *mem)
#else
void cFREe(mem) Void_t *mem;
#endif
{
    fREe(mem);
}

/*
  ------------------------- independent_calloc -------------------------
*/

#if __STD_C
Void_t** iCALLOc(size_t n_elements, size_t elem_size, Void_t* chunks[])
#else
Void_t** iCALLOc(n_elements, elem_size, chunks) size_t n_elements;
size_t elem_size;
Void_t* chunks[];
#endif
{
    size_t sz = elem_size; /* serves as 1-element array */
    /* opts arg of 3 means all elements are same size, and should be cleared */
    return iALLOc(n_elements, &sz, 3, chunks);
}

/*
  ------------------------- independent_comalloc -------------------------
*/

#if __STD_C
Void_t** iCOMALLOc(size_t n_elements, size_t sizes[], Void_t* chunks[])
#else
Void_t** iCOMALLOc(n_elements, sizes, chunks) size_t n_elements;
size_t sizes[];
Void_t* chunks[];
#endif
{
    return iALLOc(n_elements, sizes, 0, chunks);
}


/*
  ------------------------------ ialloc ------------------------------
  ialloc provides common support for independent_X routines, handling all of
  the combinations that can result.

  The opts arg has:
    bit 0 set if all elements are same size (using sizes[0])
    bit 1 set if elements should be zeroed
*/


#if __STD_C
static Void_t** iALLOc(size_t n_elements,
                       size_t* sizes,
                       int opts,
                       Void_t* chunks[])
#else
static Void_t** iALLOc(n_elements, sizes, opts, chunks) size_t n_elements;
size_t* sizes;
int opts;
Void_t* chunks[];
#endif
{
    mstate av = get_malloc_state();
    INTERNAL_SIZE_T element_size;   /* chunksize of each element, if all same */
    INTERNAL_SIZE_T contents_size;  /* total size of elements */
    INTERNAL_SIZE_T array_size;     /* request size of pointer array */
    Void_t*         mem;            /* malloced aggregate space */
    mchunkptr       p;              /* corresponding chunk */
    INTERNAL_SIZE_T remainder_size; /* remaining bytes while splitting */
    Void_t**        marray;         /* either "chunks" or malloced ptr array */
    mchunkptr       array_chunk;    /* chunk for malloced ptr array */
    int             mmx;            /* to disable mmap */
    INTERNAL_SIZE_T size;
    size_t          i;

    /* Ensure initialization/consolidation */
    if(have_fastchunks(av)) malloc_consolidate(av);

    /* compute array length, if needed */
    if(chunks != 0) {
        if(n_elements == 0)
            return chunks; /* nothing to do */

        marray = chunks;
        array_size = 0;
    }
    else {
        /* if empty req, must still return chunk representing empty array */
        if(n_elements == 0)
            return (Void_t**) mALLOc(0);

        marray = 0;
        array_size = request2size(n_elements * (sizeof(Void_t*)));
    }

    /* compute total element size */
    if(opts & 0x1) {  /* all-same-size */
        element_size = request2size(*sizes);
        contents_size = n_elements * element_size;
    }
    else { /* add up all the sizes */
        element_size = 0;
        contents_size = 0;

        for(i = 0; i != n_elements; ++i)
            contents_size += request2size(sizes[i]);
    }

    /* subtract out alignment bytes from total to minimize overallocation */
    size = contents_size + array_size - MALLOC_ALIGN_MASK;

    /*
       Allocate the aggregate chunk.
       But first disable mmap so malloc won't use it, since
       we would not be able to later free/realloc space internal
       to a segregated mmap region.
    */
    mmx = av->n_mmaps_max;   /* disable mmap */
    av->n_mmaps_max = 0;
    mem = mALLOc(size);
    av->n_mmaps_max = mmx;   /* reset mmap */

    if(mem == 0)
        return 0;

    p = mem2chunk(mem);
    assert(!chunk_is_mmapped(p));
    remainder_size = chunksize(p);

    if(opts & 0x2) {        /* optionally clear the elements */
        MALLOC_ZERO(mem, remainder_size - SIZE_SZ - array_size);
    }

    /* If not provided, allocate the pointer array as final part of chunk */
    if(marray == 0) {
        array_chunk = chunk_at_offset(p, contents_size);
        marray = (Void_t**)(chunk2mem(array_chunk));
        set_head(array_chunk, (remainder_size - contents_size) | PREV_INUSE);
        remainder_size = contents_size;
    }

    /* split out elements */
    for(i = 0; ; ++i) {
        marray[i] = chunk2mem(p);

        if(i != n_elements - 1) {
            if(element_size != 0)
                size = element_size;
            else
                size = request2size(sizes[i]);

            remainder_size -= size;
            set_head(p, size | PREV_INUSE);
            p = chunk_at_offset(p, size);
        }
        else { /* the final element absorbs any overallocation slop */
            set_head(p, remainder_size | PREV_INUSE);
            break;
        }
    }

#if DEBUG

    if(marray != chunks) {
        /* final element must have exactly exhausted chunk */
        if(element_size != 0)
            assert(remainder_size == element_size);
        else
            assert(remainder_size == request2size(sizes[i]));

        check_inuse_chunk(mem2chunk(marray));
    }

    for(i = 0; i != n_elements; ++i)
        check_inuse_chunk(mem2chunk(marray[i]));

#endif

    return marray;
}


/*
  ------------------------------ valloc ------------------------------
*/

#if __STD_C
Void_t* vALLOc(size_t bytes)
#else
Void_t* vALLOc(bytes) size_t bytes;
#endif
{
    /* Ensure initialization/consolidation */
    mstate av = get_malloc_state();

    if(have_fastchunks(av)) malloc_consolidate(av);

    return mEMALIGn(av->pagesize, bytes);
}

/*
  ------------------------------ pvalloc ------------------------------
*/


#if __STD_C
Void_t* pVALLOc(size_t bytes)
#else
Void_t* pVALLOc(bytes) size_t bytes;
#endif
{
    mstate av = get_malloc_state();
    size_t pagesz;

    /* Ensure initialization/consolidation */
    if(have_fastchunks(av)) malloc_consolidate(av);

    pagesz = av->pagesize;
    return mEMALIGn(pagesz, (bytes + pagesz - 1) & ~(pagesz - 1));
}


/*
  ------------------------------ malloc_trim ------------------------------
*/

#if __STD_C
int mTRIm(size_t pad)
#else
int mTRIm(pad) size_t pad;
#endif
{
    mstate av = get_malloc_state();
    /* Ensure initialization/consolidation */
    malloc_consolidate(av);

#ifndef MORECORE_CANNOT_TRIM
    return sYSTRIm(pad, av);
#else
    return 0;
#endif
}


/*
  ------------------------- malloc_usable_size -------------------------
*/

#if __STD_C
size_t mUSABLe(Void_t* mem)
#else
size_t mUSABLe(mem) Void_t* mem;
#endif
{
    mchunkptr p;

    if(mem != 0) {
        p = mem2chunk(mem);

        if(chunk_is_mmapped(p))
            return chunksize(p) - 2 * SIZE_SZ;
        else if(inuse(p))
            return chunksize(p) - SIZE_SZ;
    }

    return 0;
}

/*
  ------------------------------ mallinfo ------------------------------
*/

struct mallinfo mALLINFo(void) {
    mstate av = get_malloc_state();
    struct mallinfo mi;
    unsigned int i;
    mbinptr b;
    mchunkptr p;
    INTERNAL_SIZE_T avail;
    INTERNAL_SIZE_T fastavail;
    int nblocks;
    int nfastblocks;

    /* Ensure initialization */
    if(av->top == 0)  malloc_consolidate(av);

    check_malloc_state();

    /* Account for top */
    avail = chunksize(av->top);
    nblocks = 1;  /* top always exists */

    /* traverse fastbins */
    nfastblocks = 0;
    fastavail = 0;

    for(i = 0; i < NFASTBINS; ++i) {
        for(p = av->fastbins[i]; p != 0; p = p->fd) {
            ++nfastblocks;
            fastavail += chunksize(p);
        }
    }

    avail += fastavail;

    /* traverse regular bins */
    for(i = 1; i < NBINS; ++i) {
        b = bin_at(av, i);

        for(p = last(b); p != b; p = p->bk) {
            ++nblocks;
            avail += chunksize(p);
        }
    }

    mi.smblks = nfastblocks;
    mi.ordblks = nblocks;
    mi.fordblks = avail;
    mi.uordblks = av->sbrked_mem - avail;
    mi.arena = av->sbrked_mem;
    mi.hblks = av->n_mmaps;
    mi.hblkhd = av->mmapped_mem;
    mi.fsmblks = fastavail;
    mi.keepcost = chunksize(av->top);
    mi.usmblks = av->max_total_mem;
    return mi;
}

/*
  ------------------------------ malloc_stats ------------------------------
*/

void mSTATs(void) {
    struct mallinfo mi = mALLINFo();

    fprintf(stderr, "max system bytes = %10lu\n",
            (unsigned long)(mi.usmblks));
    fprintf(stderr, "system bytes     = %10lu\n",
            (unsigned long)(mi.arena + mi.hblkhd));
    fprintf(stderr, "in use bytes     = %10lu\n",
            (unsigned long)(mi.uordblks + mi.hblkhd));
}


/*
  ------------------------------ mallopt ------------------------------
*/

#if __STD_C
int mALLOPt(int param_number, int value)
#else
int mALLOPt(param_number, value) int param_number;
int value;
#endif
{
    mstate av = get_malloc_state();
    /* Ensure initialization/consolidation */
    malloc_consolidate(av);

    switch(param_number) {
        case M_MXFAST:

            if(value >= 0 && value <= MAX_FAST_SIZE) {
                set_max_fast(av, value);
                return 1;
            }
            else
                return 0;

        case M_TRIM_THRESHOLD:
            av->trim_threshold = value;
            return 1;

        case M_TOP_PAD:
            av->top_pad = value;
            return 1;

        case M_MMAP_THRESHOLD:
            av->mmap_threshold = value;
            return 1;

        case M_MMAP_MAX:
#if !HAVE_MMAP

            if(value != 0)
                return 0;

#endif
            av->n_mmaps_max = value;
            return 1;

        default:
            return 0;
    }
}

/* Reset function */
void pvr_int_mem_reset(void) {
    /* This _should_ do it */
    memset(&av_, 0, sizeof(av_));
}


/*
  -------------------- Alternative MORECORE functions --------------------
*/


/*
  General Requirements for MORECORE.

  The MORECORE function must have the following properties:

  If MORECORE_CONTIGUOUS is false:

    * MORECORE must allocate in multiples of pagesize. It will
      only be called with arguments that are multiples of pagesize.

    * MORECORE(0) must return an address that is at least
      MALLOC_ALIGNMENT aligned. (Page-aligning always suffices.)

  else (i.e. If MORECORE_CONTIGUOUS is true):

    * Consecutive calls to MORECORE with positive arguments
      return increasing addresses, indicating that space has been
      contiguously extended.

    * MORECORE need not allocate in multiples of pagesize.
      Calls to MORECORE need not have args of multiples of pagesize.

    * MORECORE need not page-align.

  In either case:

    * MORECORE may allocate more memory than requested. (Or even less,
      but this will generally result in a malloc failure.)

    * MORECORE must not allocate memory when given argument zero, but
      instead return one past the end address of memory from previous
      nonzero call. This malloc does NOT call MORECORE(0)
      until at least one call with positive arguments is made, so
      the initial value returned is not important.

    * Even though consecutive calls to MORECORE need not return contiguous
      addresses, it must be OK for malloc'ed chunks to span multiple
      regions in those cases where they do happen to be contiguous.

    * MORECORE need not handle negative arguments -- it may instead
      just return MORECORE_FAILURE when given negative arguments.
      Negative arguments are always multiples of pagesize. MORECORE
      must not misinterpret negative args as large positive unsigned
      args. You can suppress all such calls from even occurring by defining
      MORECORE_CANNOT_TRIM,

  There is some variation across systems about the type of the
  argument to sbrk/MORECORE. If size_t is unsigned, then it cannot
  actually be size_t, because sbrk supports negative args, so it is
  normally the signed type of the same width as size_t (sometimes
  declared as "intptr_t", and sometimes "ptrdiff_t").  It doesn't much
  matter though. Internally, we use "long" as arguments, which should
  work across all reasonable possibilities.

  Additionally, if MORECORE ever returns failure for a positive
  request, and HAVE_MMAP is true, then mmap is used as a noncontiguous
  system allocator. This is a useful backup strategy for systems with
  holes in address spaces -- in this case sbrk cannot contiguously
  expand the heap, but mmap may be able to map noncontiguous space.

  If you'd like mmap to ALWAYS be used, you can define MORECORE to be
  a function that always returns MORECORE_FAILURE.

  If you are using this malloc with something other than sbrk (or its
  emulation) to supply memory regions, you probably want to set
  MORECORE_CONTIGUOUS as false.  As an example, here is a custom
  allocator kindly contributed for pre-OSX macOS.  It uses virtually
  but not necessarily physically contiguous non-paged memory (locked
  in, present and won't get swapped out).  You can use it by
  uncommenting this section, adding some #includes, and setting up the
  appropriate defines above:

      #define MORECORE osMoreCore
      #define MORECORE_CONTIGUOUS 0

  There is also a shutdown routine that should somehow be called for
  cleanup upon program exit.

  #define MAX_POOL_ENTRIES 100
  #define MINIMUM_MORECORE_SIZE  (64 * 1024)
  static int next_os_pool;
  void *our_os_pools[MAX_POOL_ENTRIES];

  void *osMoreCore(int size)
  {
    void *ptr = 0;
    static void *sbrk_top = 0;

    if(size > 0)
    {
      if(size < MINIMUM_MORECORE_SIZE)
         size = MINIMUM_MORECORE_SIZE;
      if(CurrentExecutionLevel() == kTaskLevel)
         ptr = PoolAllocateResident(size + RM_PAGE_SIZE, 0);
      if(ptr == 0)
      {
        return (void *) MORECORE_FAILURE;
      }
      // save ptrs so they can be freed during cleanup
      our_os_pools[next_os_pool] = ptr;
      next_os_pool++;
      ptr = (void *) ((((unsigned long) ptr) + RM_PAGE_MASK) & ~RM_PAGE_MASK);
      sbrk_top = (char *) ptr + size;
      return ptr;
    }
    else if(size < 0)
    {
      // we don't currently support shrink behavior
      return (void *) MORECORE_FAILURE;
    }
    else
    {
      return sbrk_top;
    }
  }

  // cleanup any allocated memory pools
  // called as last thing before shutting down driver

  void osCleanupMem(void)
  {
    void **ptr;

    for(ptr = our_os_pools; ptr < &our_os_pools[MAX_POOL_ENTRIES]; ptr++)
      if(*ptr)
      {
         PoolDeallocate(*ptr);
         *ptr = 0;
      }
  }

*/


/* ------------------------------------------------------------
History:

    V2.7.0 Sun Mar 11 14:14:06 2001  Doug Lea  (dl at gee)
      * Introduce independent_comalloc and independent_calloc.
        Thanks to Michael Pachos for motivation and help.
      * Make optional .h file available
      * Allow > 2GB requests on 32bit systems.
      * new WIN32 sbrk, mmap, munmap, lock code from <Walter@GeNeSys-e.de>.
        Thanks also to Andreas Mueller <a.mueller at paradatec.de>,
        and Anonymous.
      * Allow override of MALLOC_ALIGNMENT (Thanks to Ruud Waij for
        helping test this.)
      * memalign: check alignment arg
      * realloc: don't try to shift chunks backwards, since this
        leads to  more fragmentation in some programs and doesn't
        seem to help in any others.
      * Collect all cases in malloc requiring system memory into sYSMALLOc
      * Use mmap as backup to sbrk
      * Place all internal state in malloc_state
      * Introduce fastbins (although similar to 2.5.1)
      * Many minor tunings and cosmetic improvements
      * Introduce USE_PUBLIC_MALLOC_WRAPPERS, USE_MALLOC_LOCK
      * Introduce MALLOC_FAILURE_ACTION, MORECORE_CONTIGUOUS
        Thanks to Tony E. Bennett <tbennett@nvidia.com> and others.
      * Include errno.h to support default failure action.

    V2.6.6 Sun Dec  5 07:42:19 1999  Doug Lea  (dl at gee)
      * return null for negative arguments
      * Added Several WIN32 cleanups from Martin C. Fong <mcfong at yahoo.com>
         * Add 'LACKS_SYS_PARAM_H' for those systems without 'sys/param.h'
          (e.g. WIN32 platforms)
         * Cleanup header file inclusion for WIN32 platforms
         * Cleanup code to avoid Microsoft Visual C++ compiler complaints
         * Add 'USE_DL_PREFIX' to quickly allow co-existence with existing
           memory allocation routines
         * Set 'malloc_getpagesize' for WIN32 platforms (needs more work)
         * Use 'assert' rather than 'ASSERT' in WIN32 code to conform to
           usage of 'assert' in non-WIN32 code
         * Improve WIN32 'sbrk()' emulation's 'findRegion()' routine to
           avoid infinite loop
      * Always call 'fREe()' rather than 'free()'

    V2.6.5 Wed Jun 17 15:57:31 1998  Doug Lea  (dl at gee)
      * Fixed ordering problem with boundary-stamping

    V2.6.3 Sun May 19 08:17:58 1996  Doug Lea  (dl at gee)
      * Added pvalloc, as recommended by H.J. Liu
      * Added 64bit pointer support mainly from Wolfram Gloger
      * Added anonymously donated WIN32 sbrk emulation
      * Malloc, calloc, getpagesize: add optimizations from Raymond Nijssen
      * malloc_extend_top: fix mask error that caused wastage after
        foreign sbrks
      * Add linux mremap support code from HJ Liu

    V2.6.2 Tue Dec  5 06:52:55 1995  Doug Lea  (dl at gee)
      * Integrated most documentation with the code.
      * Add support for mmap, with help from
        Wolfram Gloger (Gloger@lrz.uni-muenchen.de).
      * Use last_remainder in more cases.
      * Pack bins using idea from  colin@nyx10.cs.du.edu
      * Use ordered bins instead of best-fit threshold
      * Eliminate block-local decls to simplify tracing and debugging.
      * Support another case of realloc via move into top
      * Fix error occurring when initial sbrk_base not word-aligned.
      * Rely on page size for units instead of SBRK_UNIT to
        avoid surprises about sbrk alignment conventions.
      * Add mallinfo, mallopt. Thanks to Raymond Nijssen
        (raymond@es.ele.tue.nl) for the suggestion.
      * Add `pad' argument to malloc_trim and top_pad mallopt parameter.
      * More precautions for cases where other routines call sbrk,
        courtesy of Wolfram Gloger (Gloger@lrz.uni-muenchen.de).
      * Added macros etc., allowing use in linux libc from
        H.J. Lu (hjl@gnu.ai.mit.edu)
      * Inverted this history list

    V2.6.1 Sat Dec  2 14:10:57 1995  Doug Lea  (dl at gee)
      * Re-tuned and fixed to behave more nicely with V2.6.0 changes.
      * Removed all preallocation code since under current scheme
        the work required to undo bad preallocations exceeds
        the work saved in good cases for most test programs.
      * No longer use return list or unconsolidated bins since
        no scheme using them consistently outperforms those that don't
        given above changes.
      * Use best fit for very large chunks to prevent some worst-cases.
      * Added some support for debugging

    V2.6.0 Sat Nov  4 07:05:23 1995  Doug Lea  (dl at gee)
      * Removed footers when chunks are in use. Thanks to
        Paul Wilson (wilson@cs.texas.edu) for the suggestion.

    V2.5.4 Wed Nov  1 07:54:51 1995  Doug Lea  (dl at gee)
      * Added malloc_trim, with help from Wolfram Gloger
        (wmglo@Dent.MED.Uni-Muenchen.DE).

    V2.5.3 Tue Apr 26 10:16:01 1994  Doug Lea  (dl at g)

    V2.5.2 Tue Apr  5 16:20:40 1994  Doug Lea  (dl at g)
      * realloc: try to expand in both directions
      * malloc: swap order of clean-bin strategy;
      * realloc: only conditionally expand backwards
      * Try not to scavenge used bins
      * Use bin counts as a guide to preallocation
      * Occasionally bin return list chunks in first scan
      * Add a few optimizations from colin@nyx10.cs.du.edu

    V2.5.1 Sat Aug 14 15:40:43 1993  Doug Lea  (dl at g)
      * faster bin computation & slightly different binning
      * merged all consolidations to one part of malloc proper
         (eliminating old malloc_find_space & malloc_clean_bin)
      * Scan 2 returns chunks (not just 1)
      * Propagate failure in realloc if malloc returns 0
      * Add stuff to allow compilation on non-ANSI compilers
          from kpv@research.att.com

    V2.5 Sat Aug  7 07:41:59 1993  Doug Lea  (dl at g.oswego.edu)
      * removed potential for odd address access in prev_chunk
      * removed dependency on getpagesize.h
      * misc cosmetics and a bit more internal documentation
      * anticosmetics: mangled names in macros to evade debugger strangeness
      * tested on sparc, hp-700, dec-mips, rs6000
          with gcc & native cc (hp, dec only) allowing
          Detlefs & Zorn comparison study (in SIGPLAN Notices.)

    Trial version Fri Aug 28 13:14:29 1992  Doug Lea  (dl at g.oswego.edu)
      * Based loosely on libg++-1.2X malloc. (It retains some of the overall
         structure of old version,  but most details differ.)

*/
