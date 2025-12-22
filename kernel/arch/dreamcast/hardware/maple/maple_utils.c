/* KallistiOS ##version##

   maple_util.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2015 Lawrence Sebald
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dc/memory.h>
#include <dc/maple.h>
#include <kos/dbglog.h>

/* Enable / Disable the bus */
void maple_bus_enable(void) {
    maple_write(MAPLE_ENABLE, MAPLE_ENABLE_ENABLED);
}
void maple_bus_disable(void) {
    maple_write(MAPLE_ENABLE, MAPLE_ENABLE_DISABLED);
}

/* Start / Stop DMA */
void maple_dma_start(void) {
    maple_write(MAPLE_STATE, MAPLE_STATE_DMA);
}
void maple_dma_stop(void) {
    maple_write(MAPLE_STATE, MAPLE_STATE_IDLE);
}

int maple_dma_in_progress(void) {
    return maple_read(MAPLE_STATE) & MAPLE_STATE_DMA;
}

/* Set the DMA Address */
void maple_dma_addr(void *ptr) {
    maple_write(MAPLE_DMAADDR, ((uint32) ptr) & MEM_AREA_CACHE_MASK);
}

/* Return a "maple address" for a port,unit pair */
uint8 maple_addr(int port, int unit) {
    uint8 addr;

    assert(port < MAPLE_PORT_COUNT && unit < MAPLE_UNIT_COUNT);

    addr = port << 6;

    if(unit != 0)
        addr |= (1 << (unit - 1)) & 0x1f;
    else
        addr |= 0x20;

    return addr;
}

/* Decompose a "maple address" into a port,unit pair */
/* WARNING: Won't work on multi-cast addresses! */
void maple_raddr(uint8 addr, int * port, int * unit) {
    *port = (addr >> 6) & 3;

    if(addr & 0x20)
        *unit = 0;
    else if(addr & 0x10)
        *unit = 5;
    else if(addr & 0x08)
        *unit = 4;
    else if(addr & 0x04)
        *unit = 3;
    else if(addr & 0x02)
        *unit = 2;
    else if(addr & 0x01)
        *unit = 1;
    else {
        dbglog(DBG_ERROR, "maple_raddr: invalid address %02x\n", addr);
        *port = -1;
        *unit = -1;
        assert_msg(0, "invalid unit id");
    }
}

/* Strings for maple device capabilities */
static const char *maple_cap_names[] = {
    "LightGun",
    "Keyboard",
    "Argun",
    "Microphone",
    "Clock",
    "LCD",
    "MemoryCard",
    "Controller",
    NULL, NULL, NULL, NULL,
    "Camera",
    NULL,
    "Mouse",
    "JumpPack"
};

/* Print the capabilities of a given driver to dbglog; NOT THREAD SAFE */
static char caps_buffer[64];
const char * maple_pcaps(uint32 functions) {
    unsigned int i, o;

    for(o = 0, i = 0; i < 32; i++) {
        if(functions & (0x80000000 >> i)) {
            if(i > __array_size(maple_cap_names) || maple_cap_names[i] == NULL) {
                sprintf(caps_buffer + o, "UNKNOWN(%08x), ", (0x80000000 >> i));
                o += strlen(caps_buffer + o);
            }
            else {
                sprintf(caps_buffer + o, "%s, ", maple_cap_names[i]);
                o += strlen(caps_buffer + o);
            }
        }
    }

    if(o > 0) {
        o -= 2;
        caps_buffer[o] = '\0';
    }

    return caps_buffer;
}

static const char *maple_resp_names[] = {
    "EFILEERR",
    "EAGAIN",
    "EBADCMD",
    "EBADFUNC",
    "ENONE",
    NULL, NULL, NULL, NULL, NULL,
    "DEVINFO",
    "ALLINFO",
    "OK",
    "DATATRF"
};

/* Return a string representing the maple response code */
const char * maple_perror(int response) {
    response += 5;

    if(response < 0 || (size_t)response >= __array_size(maple_resp_names))
        return "UNKNOWN";
    else
        return maple_resp_names[response];
}

/* Determine if a given device is valid */
int maple_dev_valid(int p, int u) {
    return !!maple_enum_dev(p, u);
}

int maple_gun_enable(int port) {
    if(port >= 0 && port < 4) {
        maple_state.gun_port = port;
        return MAPLE_EOK;
    }

    return MAPLE_EFAIL;
}

void maple_gun_disable(void) {
    maple_state.gun_port = -1;
}

void maple_gun_read_pos(int *x, int *y) {
    *x = maple_state.gun_x;
    *y = maple_state.gun_y;
}

/* Debugging help */
void maple_sentinel_setup(void * buffer, int bufsize) {
    assert(__is_defined(MAPLE_DMA_DEBUG));
    assert(bufsize % 4 == 0);
    memset(buffer, 0xdeadbeef, bufsize);
}

void maple_sentinel_verify(const char * bufname, void * buffer, int bufsize) {
    int i;
    uint32 *b32;

    assert(__is_defined(MAPLE_DMA_DEBUG));
    assert(bufsize % 4 == 0);

    b32 = ((uint32 *)buffer) - 512 / 4;

    for(i = 0; i < 512 / 4; i++) {
        if(b32[i] != 0xdeadbeef) {
            dbglog(DBG_ERROR, "*** BUFFER CHECK FAILURE *** %s failed at pre-offset %d\n",
                   bufname, i);
            assert(0);
        }
    }

    b32 = ((uint32 *)buffer) + bufsize / 4;

    for(i = 0; i < 512 / 4; i++) {
        if(b32[i] != 0xdeadbeef) {
            dbglog(DBG_ERROR, "*** BUFFER CHECK FAILURE *** %s failed at post-offset %d\n",
                   bufname, i);
            assert(0);
        }
    }
}
