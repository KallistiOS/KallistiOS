/* KallistiOS ##version##

   ringbuffer.c
   (c)2025 jnmartin84
*/

#include <kos.h>
#include <assert.h>

/* Implement a memory-mapped ring buffer allowing a linear write to
   correctly wrap around. This also provides an example of using
   `mmu_page_map_static` to install fixed virtual-to-physical memory mappings
   in the SH4 TLB.

   Based on the idea presented in
   "Super Fast Circular Ring Buffer Using Virtual Memory trick":
   https://abhinavag.medium.com/a-fast-circular-ring-buffer-4d102ef4d4a3
*/

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

// a static 4kb buffer, 4kb aligned for TLB mapping with 4kb page size
static uint8_t __attribute__((aligned(4096))) ring_buffer_storage[4096];

// a nice, memorable but ultimately fake virtual memory address for our ring buffer
// this will be the starting address of our TLB mappings
static uint8_t *ring_buffer_pointer = (uint8_t *)0x12340000;

int main(int argc, char **argv)
{
    /* Before we proceed, set ring_buffer_storage to a sentinel value
       We use the value 127 here, to distinguish from the 0 or 255 we set later.
    */
    memset(ring_buffer_storage, 127, sizeof(ring_buffer_storage));
    // To convince you that every byte of the array is cleared to 127 at startup :-)
    for (int i = 0; i < 4096; i++)
    {
        assert(ring_buffer_storage[i] == 127);
    }

    /* Initialize basic MMU support; only using static TLB mappings */
    mmu_init_basic();

    /* mmu_page_map_static takes:
        a virtual address, aligned to the page size
        a physical address to back the virtual address, aligned to the page size
        the desired page size for the mapping
        mmu protection settings for the page
        cacheability
    */

    // the buffer itself can be covered by a single 4kb page
    // by using two virtual 4kb pages, you can wrap around to the front of the ring buffer
    // by writing past the end of it :-)

    /*
        virtual memory
        [v0 v1 v2 ... v4095][v4096 v4097 v4098 ... v8191]
         |  |  |      |        /     /     /          /
         |--|--|------|-------/     /     /          /
         |  |--|------|------------/     /          /
         |  |  |------|-----------------/          /
         |  |  |      |---------------------------/
         |  |  |      |
        [p0 p1 p2 ... p4095]
        physical memory
    */

    // physical(ring_buffer_storage) to ring_buffer_pointer
    mmu_page_map_static((uintptr_t)ring_buffer_pointer,
                        (uintptr_t)ring_buffer_storage & ~MEM_AREA_P1_BASE, PAGE_SIZE_4K, MMU_ALL_RDWR, MMU_CACHEABLE);

    // physical(ring_buffer_storage) to ring_buffer_pointer + 0x1000 (4 kb)
    mmu_page_map_static((uintptr_t)ring_buffer_pointer + 0x1000,
                        (uintptr_t)ring_buffer_storage & ~MEM_AREA_P1_BASE, PAGE_SIZE_4K, MMU_ALL_RDWR, MMU_CACHEABLE);

    // here we iterate over 6144 bytes, starting at the beginning of our ring buffer
    // although the backing store is 4096 bytes of physical memory,
    // we have mapped 8192 bytes of virtual memory to duplicate the physical memory linearly
    // the initial value of all of these bytes is 127
    // if the index is less than 4096,
    //  we set the byte to 0
    // if the index (byte i past the start of the ring buffer) is greater than or equal to 4096,
    //  we set the byte to 255
    for (int i = 0; i < 6144; i++)
    {
        if (i < 4096)
        {
            ring_buffer_pointer[i] = 0;
        }
        else
        {
            ring_buffer_pointer[i] = 255;
        }
    }

    // we will now ssert that the second half of the buffer contains zeros, as it was written to
    // from virtual memory indices 2048 through 4095
    for (int i = 2048; i < 4096; i++)
    {
        assert(ring_buffer_storage[i] == 0);
    }
    printf("Writing to elements 2048 through 4095 updated elements 2048 through 4095 :-)\n");

    // we then check the value of the first 2048 bytes in the actual ring buffer storage array
    // they should all be set to 255 at this time
    // if they are, that means we wrapped around a 4kb buffer by writing into it as if it was an 8kb buffer
    // magic :-)
    for (int i = 0; i < 2048; i++)
    {
        assert(ring_buffer_storage[i] == 255);
    }
    printf("Writing to elements 4096 through 6143 updated elements 0 through 2047 :-)\n");

    /* Shutdown MMU support */
    mmu_shutdown();

    return 0;
}
