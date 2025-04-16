/* KallistiOS ##version##

   aram.c
   Copyright (C) 2024 Paul Cercueil <paul@crapouillou.net>
*/

#include <stdint.h>
#include <dc/g2bus.h>
#include <dc/aram.h>

static void aram_copy(char *dst, const char *src, size_t size)
{
    uint32_t cnt = 0;

    g2_lock_scoped();

    if (!(((uintptr_t)dst | (uintptr_t)src) & 0x3)) {
        for (; size >= 4; size -= 4) {
            /* Fifo wait if necessary */
            if (!(cnt % 8))
                g2_fifo_wait();

            *(uint32_t *)dst = *(const uint32_t *)src;
            dst += 4;
            src += 4;
            cnt++;
        }
    }

    /* Reset FIFO counter */
    cnt = 0;

    for (; size; size--) {
        /* Fifo wait if necessary */
        if (!(cnt % 8))
            g2_fifo_wait();

        *dst++ = *src++;
        cnt++;
    }
}

void aram_read(void *dst, aram_addr_t addr, size_t size)
{
    const char *src = (const char *)aram_addr_to_host(addr);

    aram_copy((char *)dst, src, size);
}

void aram_write(aram_addr_t addr, const void *src, size_t size)
{
    char *dst = (char *)aram_addr_to_host(addr);

    aram_copy(dst, (const char *)src, size);
}

static inline uint32_t has_eof(uint32_t dword)
{
    uint32_t ret = 0;

    __asm__ __inline__("cmp/str %[ret], %[dword]\n\t"
                       "movt %[ret]\n\t"
                       : [ret]"+r"(ret) : [dword]"r"(dword));

    return ret;
}

char *aram_read_string(aram_addr_t addr, uint32_t *dst, size_t size)
{
    const uint32_t *src;
    uint32_t value, cnt = 0;
    char *ret = (char *)dst;

    if (addr & 0x3) {
        ret += 4 - (addr & 0x3);
        addr &= ~0x3;
    }

    src = (const uint32_t *)aram_addr_to_host(addr);

    g2_lock_scoped();

    for (; size >= 4; size -= 4) {
        /* Fifo wait if necessary */
        if (!(cnt % 8))
            g2_fifo_wait();

        value = *src++;
        *dst++ = value;

        if (has_eof(value))
            break;

        cnt++;
    }

    return ret;
}
