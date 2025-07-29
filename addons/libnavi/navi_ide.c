/* KallistiOS ##version##

   ide.c

   (c)1997,2002 Megan Potter

*/

#include <assert.h>
#include <stdio.h>
#include <navi/ide.h>
#include <dc/g2bus.h>
#include <kos/dbglog.h>
#include <kos/thread.h>

/*
   A *very* simple port-level ATA-IDE device driver. This was ported up
   from the _original_ KallistiOS =) (PC based)
 */

static uint16 dd[256];          /* hard disk parameters, read from controller */
static uint32 hd_cyls, hd_heads, hd_sects;          /* hard disk geometry */


static void ide_outp(int port, uint16 value, int size) {
    (void)size;
    uint32 addr;

    switch(port & 0xff0) {
        case 0x1f0:
            addr = 0xb4002000 + ((port & 7) << 2);
            break;
        case 0x3f0:
            addr = 0xb4001000 + ((port & 7) << 2);
            break;
        default:
            assert(0);
            return;
    }

    /* printf("ide_outp %02x -> %04x(%08x)\n", value, port, addr); */
    g2_write_16(addr, value);
}

uint16 ide_inp(int port, int size) {
    uint32 addr;
    uint16 value;

    switch(port & 0xff0) {
        case 0x1f0:
            addr = 0xb4002000 + ((port & 7) << 2);
            break;
        case 0x3f0:
            addr = 0xb4001000 + ((port & 7) << 2);
            break;
        default:
            assert(0);
            return 0;
    }

    value = g2_read_16(addr);

    if(!size)
        value &= 0xff;

    return value;
}

#define outp(x, y) ide_outp(x, y, 0)
#define outpw(x, y) ide_outp(x, y, 1)
#define inp(x) ide_inp(x, 0)
#define inpw(x) ide_inp(x, 1)


/* These are to synchronize us with the controller so we don't do something
   it's not ready for */
static void wait_controller(void) {
    int timeout = 1000;

    while((inp(0x1f7) & 0x80) && timeout) {
        thd_sleep(1);
        timeout--;
    }

    if(!timeout) {
        dbglog(DBG_ERROR, "IDE controller timed out waiting for ready.. status = %x/%x\n",
               inp(0x1f7), inp(0x1f1));
    }
}

static void wait_data(void) {
    int timeout = 1000;

    while(!(inp(0x1f7) & 0x08) && timeout) {
        thd_sleep(1);
        timeout--;
    }

    if(!timeout) {
        dbglog(DBG_ERROR, "IDE controller timed out waiting for data.. status = %x/%x\n",
               inp(0x1f7), inp(0x1f1));
    }
}

/* Reads a chunk of ascii out of the hd params table */
static char *get_ascii(uint16 *in_data, uint32 off_start, uint32 off_end) {
    static char ret_val [255];
    unsigned loop;
    int loop1;

    /* First, construct a string from the controller-retrieved data */
    for(loop = off_start, loop1 = 0; loop <= off_end; loop++) {
        ret_val [loop1++] = (char)(in_data [loop] / 256);   /* Get High byte */
        ret_val [loop1++] = (char)(in_data [loop] % 256);   /* Get Low byte */
    }

    /* Now, go back and eliminate the blank spaces */
    for(; (ret_val[loop1] < 'A' || ret_val[loop1] > 'z') && loop1 >= 0 ; loop1--)
        ret_val[loop1] = '\0';

    return ret_val;
}

/* Read n sectors from the hard disk using PIO mode */
static int ide_read_chs(uint32 cyl, uint32 head, uint32 sector, uint32 numsects, uint8 *bufptr) {
    int o;
    uint16  *bufptr16 = (uint16*)bufptr;

    /** printf("reading C/H/S/Cnt %d/%d/%d/%d\n",
        cyl, head, sector, numsects); */

    wait_controller();      /* wait for controller to be not busy */

    outp(0x1f2, numsects & 0xff);   /* number of sectors to read */
    outp(0x1f3, sector & 0xff); /* sector number to read */
    outp(0x1f4, cyl & 0xff);    /* cylinder (low part) */
    outp(0x1f5, cyl >> 8);      /* cylinder (high part) */
    outp(0x1f6, 0xa0 | (head & 0xf)); /* select device 0 and head */
    outp(0x1f7, 0x20);      /* command == read sector(s) w/retry */

    wait_data();            /* wait for data to be read */

    for(o = 0; o < 256; o++) {
        /*if(inp(0x1f7) & 1) {
            printf("as of %d, error code is %x/%x\n", o, inp(0x1f7), inp(0x1f1));
            return -1;
        } */
        bufptr16[o] = inpw(0x1f0);
    }

    if(inp(0x1f7) & 1) {
        printf("after read, status is %x/%x\n", inp(0x1f7), inp(0x1f1));
    }

    return 0;
}

/* Write n sectors to the hard disk using PIO mode */
static int ide_write_chs(uint32 cyl, uint32 head, uint32 sector, uint32 numsects, uint8 *bufptr) {
    int o;
    uint16  *bufptr16 = (uint16*)bufptr;

    /** printf("writing C/H/S/Cnt %d/%d/%d/%d\n",
        cyl, head, sector, numsects); */

    wait_controller();      /* wait for controller to be not busy */

    outp(0x1f2, numsects & 0xff);   /* number of sectors to write */
    outp(0x1f3, sector & 0xff); /* sector number to write */
    outp(0x1f4, cyl & 0xff);    /* cylinder (low part) */
    outp(0x1f5, cyl >> 8);      /* cylinder (high part) */
    outp(0x1f6, 0xa0 | (head & 0xf)); /* select device 0 and head */
    outp(0x1f7, 0x30);      /* command == write sector(s) w/retry */

    wait_data();            /* wait for data to be ready */

    for(o = 0; o < 256; o++) {
        /* if(inp(0x1f7) & 1) {
            printf("as of %d, error code is %x/%x\n", o, inp(0x1f7), inp(0x1f1));
            return -1;
        } */
        outpw(0x1f0, bufptr16[o]);
    }

    if(inp(0x1f7) & 1) {
        printf("after write, status is %x/%x\n", inp(0x1f7), inp(0x1f1));
    }

    return 0;
}

/* Translate a linear sector address relative to the first of the partition
   to a CHS address suitable for feeding to the hard disk */
static void linear_to_chs(uint32 linear, uint32 * cyl, uint32 * head, uint32 * sector) {
    *sector = linear % hd_sects + 1;
    *head = (linear / hd_sects) % hd_heads;
    *cyl = linear / (hd_sects * hd_heads);
}

/* Read n sectors from the hard disk using PIO mode */
int ide_read(uint32 linear, uint32 numsects, void * bufptr) {
    unsigned i;
    uint32  cyl, head, sector;

    if(numsects > 1) {
        for(i = 0; i < numsects; i++) {
            if(ide_read(linear + i, 1, ((uint8 *)bufptr) + i * 512) < 0)
                return -1;
        }
    }
    else {
        linear_to_chs(linear, &cyl, &head, &sector);

        if(ide_read_chs(cyl, head, sector, numsects, (uint8*)bufptr) < 0)
            return -1;
    }

    return 0;
}

/* Write n sectors to the hard disk using PIO mode */
int ide_write(uint32 linear, uint32 numsects, void *bufptr) {
    unsigned i;
    uint32  cyl, head, sector;

    if(numsects > 1) {
        for(i = 0; i < numsects; i++) {
            if(ide_write(linear + i, 1, ((uint8 *)bufptr) + i * 512) < 0)
                return -1;
        }
    }
    else {
        linear_to_chs(linear, &cyl, &head, &sector);
        ide_write_chs(cyl, head, sector, numsects, (uint8*)bufptr);
    }

    return 0;
}

/* Get the available space */
uint32 ide_num_sectors(void) {
    return hd_cyls * hd_heads * hd_sects;
}

/* Initialize the device */
int ide_init(void) {
    int dd_off;

    dbglog(DBG_INFO, "ide_init: initializing\n");

    /* Reset */
    outp(0x3f6, 0x0e);
    thd_sleep(10);
    outp(0x3f6, 0x0a);
    thd_sleep(10);

    wait_controller();
    outp(0x1f6, 0xa0);  /* get info on first drive. 0xb0 == 2nd */
    outp(0x1f7, 0xec);  /* get drive info data */
    wait_data();

    for(dd_off = 0; dd_off < 256; dd_off++) {
        dd[dd_off] = inpw(0x1f0);
    }

    hd_cyls = dd[1];
    hd_heads = dd[3];
    hd_sects = dd[6];

    dbglog(DBG_INFO, "ide0: %s, %luMB, CHS (%u/%u/%u)\n",
           get_ascii(dd, 27, 46),
           (hd_cyls * hd_heads * hd_sects * 512L) / (1024L * 1024L),
           (unsigned)hd_cyls, (unsigned)hd_heads, (unsigned)hd_sects);

    return 0;
}

void ide_shutdown(void) {
}


