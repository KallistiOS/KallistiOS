/* KallistiOS ##version##

   malloc_debug.c
   (c)2001-2002 Megan Potter
*/

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <arch/arch.h>
#include <kos/thread.h>
#include <arch/spinlock.h>

static spinlock_t mutex = SPINLOCK_INITIALIZER;

#define BLOCK_MAGIC 0x1c518a74
#define PRE_MAGIC   0x6765adb8
#define POST_MAGIC  0x29d4ca6d

/* The memory pool will look something like this:
   memctl_t (padded to 32 bytes long)
   char[1024]   pre-buffer no-touch zone
   char[]   allocated buffer space
   char[1024]   post-buffer no-touch zone
 */
typedef struct memctl {
    uint32_t    magic;
    uint32_t    size;
    tid_t       thread;
    uint32_t    addr;
    int     inuse, damaged;
    uint32_t    *post;
    const char  *type;
    struct memctl   *next;
} memctl_t;

static memctl_t *first = NULL, *last = NULL;

#define get_memctl(p) ((memctl_t*)( ((uint32_t)(p)) - 1024))

void * sbrk(int amt);

void *calloc(size_t nmemb, size_t size) {
    uint32_t pr = arch_get_ret_addr();
    void * ptr;
    memctl_t * ctl;

    ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb * size);

    ctl = get_memctl(ptr);
    ctl->addr = pr;
    ctl->type = "calloc";

    return ptr;
}

void *malloc(size_t amt) {
    memctl_t *ctl;
    uint32_t *nt1, *nt2;
    uint32_t *space;
    uint32_t pr = arch_get_ret_addr();
    int i;

    spinlock_lock_scoped(&mutex);

    /* Get a control block space */
    ctl = sbrk(1024);
    memset(ctl, 0, 1024);
    ctl->magic = BLOCK_MAGIC;
    ctl->size = amt;
    // ctl->thread = thd_get_tid();
    ctl->thread = thd_current->tid;
    ctl->addr = pr;
    ctl->inuse = 1;
    ctl->damaged = 0;

    if(!first)
        first = ctl;

    if(last)
        last->next = ctl;

    /* Fill pre-buffer no-touch zone */
    nt1 = ((uint32_t *)ctl);

    for(i = sizeof(memctl_t) / 4; i < 1024 / 4; i++) {
        nt1[i] = PRE_MAGIC;
    }

    /* Allocate actual buffer space */
    if(amt & 31) {
        amt = (amt & ~31) + 32;
    }

    space = sbrk(amt);

    /* Allocate post-buffer no-touch zone */
    nt2 = sbrk(1024);

    for(i = 0; i < 1024 / 4; i++) {
        nt2[i] = POST_MAGIC;
    }

    ctl->post = nt2;
    ctl->next = NULL;
    ctl->type = "malloc";
    last = ctl;

    printf("Thread %d/%08lx allocated %ld bytes at %08lx; %08lx left\n",
           ctl->thread, ctl->addr, ctl->size, space, _arch_mem_top - (uint32_t)sbrk(0));

    assert(!(((uint32_t)space) & 7));

    return space;
}

void *memalign(size_t alignment, size_t amt) {
    uint32_t sb, pr = arch_get_ret_addr();
    memctl_t * ctl;
    void *rv;

    printf("memalign: real address for the following is %08lx\n   ", pr);

    sb = (uint32_t)sbrk(0);

    if(sb & (alignment - 1)) {
        sbrk(alignment - (sb & (alignment - 1)));
    }

    rv = malloc(amt);

    ctl = get_memctl(rv);
    ctl->addr = pr;
    ctl->type = "memalign";

    return rv;
}

void *realloc(void *ptr, size_t newsize) {
    memctl_t *ctl;
    uint32_t pr = arch_get_ret_addr();
    void *nb;

    printf("realloc: real address for the following is %08lx\n   ", pr);

    /* Don't realloc if we don't need to */
    ctl = get_memctl(ptr);

    if(newsize == ctl->size)
        return ptr;

    /* Free the old block */
    printf("   ");
    free(ptr);

    /* Allocate a new one */
    printf("   ");
    nb = malloc(newsize);

    /* Copy over the old data */
    memcpy(nb, ptr, ctl->size);

    ctl = get_memctl(nb);
    ctl->addr = pr;
    ctl->type = "realloc";

    return nb;
}

void free(void *block) {
    memctl_t *ctl;
    uint32_t *nt1, *nt2, pr = arch_get_ret_addr();
    int i;

    spinlock_lock_scoped(&mutex);

    printf("Thread %d/%08lx freeing block @ %08lx\n",
           thd_current->tid, pr, (uint32_t)block);

    if(((uint32_t)block) & 7 || (uint32_t)block < 0x8c010000 || (uint32_t)block >= _arch_mem_top) {
        printf("   ATTEMPT TO FREE INVALID ADDRESS!\n");
        spinlock_unlock(&mutex);
        return;
    }

    ctl = get_memctl(block);

    if(ctl->magic != BLOCK_MAGIC) {
        printf("  'magic' is not correct! %08lx\n", ctl->magic);
        spinlock_unlock(&mutex);
        return;
    }

    if(ctl->inuse == 0) {
        printf("  block already freed!\n");
        ctl->damaged = 1;
    }

    nt1 = (uint32_t *)(((uint32_t)block) - 1024);

    for(i = sizeof(memctl_t) / 4; i < 1024 / 4; i++) {
        if(nt1[i] != PRE_MAGIC) {
            printf("  pre-magic is wrong at index %d (%08lx)\n", i, nt1[i]);
            ctl->damaged = 1;
            break;
        }
    }

    nt2 = ctl->post;

    for(i = 0; i < 1024 / 4; i++) {
        if(nt2[i] != POST_MAGIC) {
            printf("  post-magic is wrong at index %d (%08lx)\n", i, nt2[i]);
            ctl->damaged = 1;
            //break;
        }
    }

    ctl->inuse = 0;
}

void malloc_stats(void) {
    memctl_t *ctl;
    int dmgcnt, leaked, leakedcnt;

    if(!first) {
        printf("NO MEMORY ALLOCATED\n");
        return;
    }

    printf("%ld TOTAL BYTES OF MEMORY ALLOCATED\n",
           (uint32_t)sbrk(0) - (uint32_t)first);

    ctl = first;
    printf("Problem blocks:\n");
    dmgcnt = 0;
    leaked = 0;
    leakedcnt = 0;

    while(ctl) {
        if(ctl->inuse) {
            printf("  INUSE %08lx: size %ld, thread %d, addr %08lx, type %s\n",
                   (uint32_t)ctl + 1024, ctl->size, ctl->thread, ctl->addr, ctl->type);
            leaked += ctl->size;
            leakedcnt++;
        }

        if(ctl->damaged) {
            printf("  DMGED %08lx: size %ld, thread %d, addr %08lx, type %s\n",
                   (uint32_t)ctl + 1024, ctl->size, ctl->thread, ctl->addr, ctl->type);
            dmgcnt++;
        }

        ctl = ctl->next;
    }

    printf("End of list; %d damaged blocks and %d leaked bytes in %d blocks\n", dmgcnt, leaked, leakedcnt);
}
