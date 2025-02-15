/* KallistiOS ##version##

   navi_flash.c

   (c)2002 Megan Potter

*/

#include <assert.h>
#include <stdio.h>
#include <navi/flash.h>
#include <kos/thread.h>

/*

  A simple MTD driver for the STMicro M29W800B (the flash chip I'm using). Should
  also work for M29W160B if you can find it, or the AMD chips.

  We use the function prefix "nvflash" here instead of just "flash" since
  conceivably, KOS might eventually have a driver for the normal DC flash.

 */

/*

  For quick reference, here are the M29W800B erase regions:

  KB    Range
  --    -----
  64    F0000h-FFFFFh
  64    E0000h-EFFFFh
  64    D0000h-DFFFFh
  64    C0000h-CFFFFh
  64    B0000h-BFFFFh
  64    A0000h-AFFFFh
  64    90000h-9FFFFh
  64    80000h-8FFFFh
  64    70000h-7FFFFh
  64    60000h-6FFFFh
  64    50000h-5FFFFh
  64    40000h-4FFFFh
  64    30000h-3FFFFh
  64    20000h-2FFFFh
  64    10000h-1FFFFh
  32    08000h-0FFFFh
  8     06000h-07FFFh
  8     04000h-05FFFh
  16    00000h-03FFFh

*/

/* These defines are from Linux's AMD MTD driver */
#define ADDR_MANUFACTURER               0x0000
#define ADDR_DEVICE_ID                  0x0001
#define ADDR_SECTOR_LOCK                0x0002
#define ADDR_HANDSHAKE                  0x0003
#define ADDR_UNLOCK_1                   0x0AAA
#define ADDR_UNLOCK_2                   0x0555

#define CMD_UNLOCK_DATA_1               0x00AA
#define CMD_UNLOCK_DATA_2               0x0055
#define CMD_MANUFACTURER_UNLOCK_DATA    0x0090
#define CMD_UNLOCK_BYPASS_MODE          0x0020
#define CMD_PROGRAM_UNLOCK_DATA         0x00A0
#define CMD_RESET_DATA                  0x00F0
#define CMD_SECTOR_ERASE_UNLOCK_DATA    0x0080
#define CMD_SECTOR_ERASE_UNLOCK_DATA_2  0x0030
#define CMD_ERASE_ALL           0x0010

#define CMD_UNLOCK_SECTOR               0x0060
#define STMICRO             0x0020
#define D6_MASK                         0x40

static vuint8   * const flashport = (vuint8 *)0xa0000000;

/* We'll do this before sending a command */
static void send_unlock(void) {
    flashport[ADDR_UNLOCK_1] = CMD_UNLOCK_DATA_1;
    flashport[ADDR_UNLOCK_2] = CMD_UNLOCK_DATA_2;
}

/* Send a command (including unlock) */
static void send_cmd(uint32 cmd) {
    send_unlock();
    flashport[ADDR_UNLOCK_1] = cmd;
}

/* Read a flash value */
static uint32 nvflash_read(uint32 addr) {
    return flashport[addr];
}

/* Determine if the flash memory is busy writing */
static int nvflash_busy(uint32 addr) {
    return (nvflash_read(addr) & D6_MASK) != (nvflash_read(addr) & D6_MASK);
}

/* Wait until the flash is ready, with timeout */
static int nvflash_wait_ready(uint32 addr, int timeout) {
    int wait = 0;

    if(timeout < 0) {
        timeout = -timeout;
        wait = 1;
    }

    while(timeout-- && nvflash_busy(addr)) {
        if(wait)
            thd_sleep(1);
    }

    if(timeout <= 0) {
        printf("nvflash_wait_ready: writing to flash timed out\n");
        return -1;
    }

    return 0;
}

/* Write a single flash value, return -1 if we fail or timeout */
static int nvflash_write(uint32 addr, uint8 value) {
    send_cmd(CMD_PROGRAM_UNLOCK_DATA);
    flashport[addr] = value;

    if(nvflash_wait_ready(addr, 10000) < 0) {
        printf("nvflash_write: failed writing to flash at %08lx (%02x vs %02x)\n",
               addr, flashport[addr], value);
        return 0;
    }

    if(flashport[addr] != value) {
        printf("nvflash_write: warning! failed writing to flash at %08lx (%02x vs %02x)\n",
               addr, flashport[addr], value);
        return 0;
    }

    return 0;
}

/* Write a block of data */
int nvflash_write_block(uint32 addr, void * data, uint32 len) {
    uint8   * db = (uint8 *)data;
    unsigned i;

    for(i = 0; i < len; i++) {
        if(!(i % 0x10000)) {
            printf("nvflash_write_block: writing block at %08x\n", (unsigned)(i + addr));
        }

        if(nvflash_write(addr + i, db[i]) < 0) {
            printf("nvflash_write_block: aborting block write at %u\n", (unsigned)(i + addr));
            return -1;
        }
    }

    return 0;
}

/* Erase a block of flash */
int nvflash_erase_block(uint32 addr) {
    send_cmd(CMD_SECTOR_ERASE_UNLOCK_DATA);
    send_unlock();
    flashport[addr] = CMD_SECTOR_ERASE_UNLOCK_DATA_2;

    if(nvflash_wait_ready(addr, -10000) < 0) {
        printf("nvflash_erase_block: failed erasing flash block at %08lx\n", addr);
        return -1;
    }

    if(flashport[addr] != 0xff) {
        printf("nvflash_erase_block: failed erasing flash block at %08lx\n", addr);
        return -1;
    }

    return 0;
}

/* Erase the whole flash chip */
int nvflash_erase_all(void) {
    send_cmd(CMD_SECTOR_ERASE_UNLOCK_DATA);
    send_cmd(CMD_ERASE_ALL);

    if(nvflash_wait_ready(0, -20000) < 0) {
        printf("nvflash_erase_all: failed erasing full chip\n");
        return -1;
    }

    if(flashport[0] != 0xff) {
        printf("nvflash_erase_block: failed erasing full chip\n");
        return -1;
    }

    return 0;
}

/* Return 0 if we successfully detect a compatible device */
int nvflash_detect(void) {
    uint16      mfr_id, dev_id;

    if(nvflash_read(0) == 0xff && nvflash_read(2) == 0x28) {
        printf("flash_detect: normal DC BIOS detected\n");
        return -1;
    }

    /* Reset and ask for manufacturer code */
    send_cmd(CMD_RESET_DATA);
    send_cmd(CMD_MANUFACTURER_UNLOCK_DATA);
    mfr_id = nvflash_read(0);
    dev_id = nvflash_read(2);
    send_cmd(CMD_RESET_DATA);

    if(mfr_id == STMICRO) {
        printf("flash_detect: STMicro flash detected, device id = %02x\n", dev_id);
        return 0;
    }
    else {
        printf("flash_detect: unknown manufacturer/device pair %02x/%02x\n", mfr_id, dev_id);
        printf(" .... we'll assume you know what you're doing!\n");
        return 0;
    }
}

