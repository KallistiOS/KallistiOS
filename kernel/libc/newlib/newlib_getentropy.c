/* KallistiOS ##version##

   newlib_getentropy.c
   Copyright (C) 2023 Luke Benstead

*/

#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include <arch/arch.h>

/* We provide getentropy() if using Newlib < 4.4.0 */
#if __NEWLIB__ < 4 || (__NEWLIB__ == 4 && __NEWLIB_MINOR__ < 4)
int getentropy(void *ptr, size_t len) {
#else
/* getentropy() is provided by Newlib >= 4.4.0,
   but we must provide _getentropy_r() */
int _getentropy_r(void *re, void *ptr, size_t len) {
    (void)re;
#endif
    const int block_size = 128;
    struct timeval tv;
    uint8_t *src = ((uint8_t *)_arch_mem_top);
    uint8_t *dst = ptr;
    size_t i;
    int j;
    uint8_t b;

    gettimeofday(&tv, NULL);

    /* We read backwards from the end of available memory
       and XOR in blocks into the key array, while XORing with the current time.
       If anyone has a better idea for generating entropy then send a patch :) */
    for(i = 0; i < len; ++i) {
        b = tv.tv_usec & 0xff;

        for(j = 0; j < block_size; ++j) {
            b ^= *--src;
        }

        *dst = b;
        ++dst;
    }

    return 0;
}
