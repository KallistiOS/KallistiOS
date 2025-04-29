/* KallistiOS ##version##

   hardware/g1ata.c
   Copyright (C) 2013, 2014, 2015 Lawrence Sebald
   Copyright (C) 2015, 2023, 2024, 2025 Ruslan Rostovtsev
*/

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <dc/g1ata.h>
#include <dc/asic.h>

#include <kos/dbglog.h>
#include <kos/sem.h>
#include <kos/mutex.h>
#include <kos/thread.h>

#include <arch/timer.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/memory.h>

/*
   This file implements support for accessing devices over the G1 bus by the
   AT Attachment (aka ATA, PATA, or IDE) protocol. See, the GD-ROM drive is
   actually just an ATA device that implements a different packet command set
   than the normal ATAPI set. Not only that, but Sega left everything in the
   hardware to actually support two devices on the bus at a time. Thus, you can
   put together a very simple passthrough adapter to get a normal 40-pin IDE
   port to work with and hook up a hard drive to. In theory, you could also hook
   up various other devices including DVD drives, CD Burners, and the whole nine
   yards, but for now this just supports hard drives (and Compact Flash cards).

   The setup here is relatively simple, because we only have one channel which
   can have a maximum of two devices attached to it at a time. Normally the
   primary device would be the GD-ROM drive itself, so we're only actually
   concerned with the secondary device (use the normal cdrom_* functions to
   access the GD-ROM drive -- there's not a particularly compelling reason to
   support its odd packet interface here).

   There are a few potentially useful outward facing functions here, but most of
   the time all you'll need here is the function to get a block device for a
   given partition. The individual block read/write functions are all public as
   well, in case you have a reason to want to use them directly. Just keep in
   mind that all block numbers in those are absolute (i.e, not offset by any
   partition boundaries or whatnot).

   If you want to learn more about ATA, look around the internet for the
   AT Attachment - 8 ATA/ATAPI Command Set document. That's where most of the
   fun stuff in here comes from. Register locations and such were derived from
   a couple of different sources, including Quzar's GDINFO program, my own SPI
   CD Player program (which I should eventually release), and the source code to
   the emulator NullDC. Also, various postings at OSDev were quite useful in
   working some of this out.

   Anyway, that's enough for this wall of text...
*/

/* An ATA device. For the moment, we only support one of these, which happens to
   be the slave device on the only ATA bus Sega gave us. */
static struct {
    uint32_t command_sets;
    uint32_t capabilities;
    uint64_t max_lba;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint16_t wdma_modes;
} device;

/* The type of the dev_data in the block device structure */
typedef struct ata_devdata {
    uint64_t block_count;
    uint64_t start_block;
    uint64_t end_block;
} ata_devdata_t;

/* ATA-related registers. Some of these serve very different purposes when read
   than they do when written (hence why some addresses are duplicated). */
#define G1_ATA_ALTSTATUS        0xA05F7018      /* Read */
#define G1_ATA_CTL              0xA05F7018      /* Write */
#define G1_ATA_DATA             0xA05F7080      /* Read/Write */
#define G1_ATA_ERROR            0xA05F7084      /* Read */
#define G1_ATA_FEATURES         0xA05F7084      /* Write */
#define G1_ATA_IRQ_REASON       0xA05F7088      /* Read */
#define G1_ATA_SECTOR_COUNT     0xA05F7088      /* Write */
#define G1_ATA_LBA_LOW          0xA05F708C      /* Read/Write */
#define G1_ATA_LBA_MID          0xA05F7090      /* Read/Write */
#define G1_ATA_LBA_HIGH         0xA05F7094      /* Read/Write */
#define G1_ATA_CHS_SECTOR       G1_ATA_LBA_LOW
#define G1_ATA_CHS_CYL_LOW      G1_ATA_LBA_MID
#define G1_ATA_CHS_CYL_HIGH     G1_ATA_LBA_HIGH
#define G1_ATA_DEVICE_SELECT    0xA05F7098      /* Read/Write */
#define G1_ATA_STATUS_REG       0xA05F709C      /* Read */
#define G1_ATA_COMMAND_REG      0xA05F709C      /* Write */

/* PIO-related registers. */
#define G1_ATA_PIO_RACCESS_WAIT 0xA05F7490      /* Write-only */
#define G1_ATA_PIO_WACCESS_WAIT 0xA05F7494      /* Write-only */

/* DMA-related registers. */
#define G1_ATA_DMA_RACCESS_WAIT 0xA05F74A0      /* Write-only */
#define G1_ATA_DMA_WACCESS_WAIT 0xA05F74A4      /* Write-only */
#define G1_ATA_DMA_ADDRESS      0xA05F7404      /* Read/Write */
#define G1_ATA_DMA_LENGTH       0xA05F7408      /* Read/Write */
#define G1_ATA_DMA_DIRECTION    0xA05F740C      /* Read/Write */
#define G1_ATA_DMA_ENABLE       0xA05F7414      /* Read/Write */
#define G1_ATA_DMA_STATUS       0xA05F7418      /* Read/Write */

/* Protection register code. */
#define G1_DMA_UNLOCK_CODE      0x8843
/* System memory protection unlock value. */
#define G1_DMA_UNLOCK_SYSMEM    (G1_DMA_UNLOCK_CODE << 16 | 0x407F)
/* All memory protection unlock value. */
#define G1_DMA_UNLOCK_ALLMEM    (G1_DMA_UNLOCK_CODE << 16 | 0x007F)

/* Bitmasks for the STATUS_REG/ALT_STATUS registers. */
#define G1_ATA_SR_ERR   0x01
#define G1_ATA_SR_IDX   0x02
#define G1_ATA_SR_CORR  0x04
#define G1_ATA_SR_DRQ   0x08
#define G1_ATA_SR_DSC   0x10
#define G1_ATA_SR_DF    0x20
#define G1_ATA_SR_DRDY  0x40
#define G1_ATA_SR_BSY   0x80

/* ATA Commands we might like to send. */
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_READ_SECTORS_EXT    0x24
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_WRITE_SECTORS_EXT   0x34
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_FLUSH_CACHE         0xE7
#define ATA_CMD_FLUSH_CACHE_EXT     0xEA
#define ATA_CMD_IDENTIFY            0xEC
#define ATA_CMD_SET_FEATURES        0xEF

/* Subcommands we might care about for the SET FEATURES command. */
#define ATA_FEATURE_TRANSFER_MODE   0x03

/* Transfer mode values. */
#define ATA_TRANSFER_PIO_DEFAULT    0x00
#define ATA_TRANSFER_PIO_NOIORDY    0x01
#define ATA_TRANSFER_PIO_FLOW(x)    0x08 | ((x) & 0x07)
#define ATA_TRANSFER_WDMA(x)        0x20 | ((x) & 0x07)
#define ATA_TRANSFER_UDMA(x)        0x40 | ((x) & 0x07)

#define ATA_MAX_SECTORS_LBA28       256
#define ATA_MAX_SECTORS_LBA48       65536

/* Access timing data. */
#define G1_ACCESS_WDMA_MODE2        0x00001001
#define G1_ACCESS_PIO_DEFAULT       0x00000222

/* DMA Settings. */
#define G1_DMA_TO_DEVICE            0
#define G1_DMA_TO_MEMORY            1

/* Macros to access the ATA registers */
#define OUT32(addr, data) *((volatile uint32_t *)(addr)) = data
#define OUT16(addr, data) *((volatile uint16_t *)(addr)) = data
#define OUT8(addr, data)  *((volatile uint8_t  *)(addr)) = data
#define IN32(addr)        *((volatile uint32_t *)(addr))
#define IN16(addr)        *((volatile uint16_t *)(addr))
#define IN8(addr)         *((volatile uint8_t  *)(addr))

static int initted = 0;
static int devices = 0;
static uint8_t dev_selected = 0x00;
static uint8_t orig_dev = 0x00;

/* Variables related to DMA. */
static int dma_in_progress = 0;
static int dma_blocking = 0;
static uint8_t dma_cmd = 0;
static size_t dma_nb_sectors = 0;
static uint64_t dma_sector = 0;
static semaphore_t dma_done = SEM_INITIALIZER(0);
static kthread_t *dma_thd = NULL;

/* From cdrom.c */
extern mutex_t _g1_ata_mutex;

#define g1_ata_wait_status(n) \
    do {} while((IN8(G1_ATA_ALTSTATUS) & (n)))

#define g1_ata_wait_nbsy() g1_ata_wait_status(G1_ATA_SR_BSY)

#define g1_ata_wait_bsydrq() g1_ata_wait_status(G1_ATA_SR_DRQ | G1_ATA_SR_BSY)

#define g1_ata_wait_drdy() \
    do {} while(!(IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_DRDY))

static inline int use_lba28(uint64_t sector, size_t count) {
    return ((sector + count) < 0x0FFFFFFF) && (count <= ATA_MAX_SECTORS_LBA28);
}

#define CAN_USE_LBA48() ((device.command_sets & (1 << 26)))

/* Is a G1 DMA in progress? */
int g1_dma_in_progress(void) {
    return IN32(G1_ATA_DMA_STATUS);
}

/* G1 mutex handling. */
inline int g1_ata_mutex_lock(void) {
    if(irq_inside_int())
        return mutex_trylock(&_g1_ata_mutex);
    else
        return mutex_lock(&_g1_ata_mutex);
}

inline int g1_ata_mutex_unlock(void) {
    /* Make sure to select the GD-ROM drive back. */
    g1_ata_select_device(G1_ATA_MASTER);
    return mutex_unlock(&_g1_ata_mutex);
}

static void g1_ata_set_sector_and_count(uint64_t sector, size_t count, int lba28) {
    if(!lba28) {
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)(count >> 8));
        OUT8(G1_ATA_LBA_LOW, (uint8_t)((sector >> 24) & 0xFF));
        OUT8(G1_ATA_LBA_MID, (uint8_t)((sector >> 32) & 0xFF));
        OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
    }

    /* Write out the number of sectors we want and the lower 24-bits of
       the LBA we're looking for. Note that putting 0 into the sector count
       register returns max count of sectors. */
    OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)count);
    OUT8(G1_ATA_LBA_LOW, (uint8_t)((sector >> 0) & 0xFF));
    OUT8(G1_ATA_LBA_MID, (uint8_t)((sector >> 8) & 0xFF));
    OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));
}

static void g1_dma_done(void) {
    /* Signal the calling thread to continue, if it is blocking. */
    if(dma_blocking) {
        sem_signal(&dma_done);
        thd_schedule(1, 0);
        dma_blocking = 0;
    }

    dma_in_progress = 0;

    /* Make sure to select the GD-ROM drive back. */
    g1_ata_select_device(G1_ATA_MASTER);
    mutex_unlock_as_thread(&_g1_ata_mutex, dma_thd);
}

static void g1_dma_irq_hnd(uint32 code, void *data) {
    int can_lba48 = CAN_USE_LBA48();
    size_t nb_sectors;
    uint8_t status;

    /* XXXX: Probably should look at the code to make sure it isn't an error. */
    (void)code;
    (void)data;

    if(dma_in_progress && !can_lba48 && dma_nb_sectors > ATA_MAX_SECTORS_LBA28) {
        dma_sector += ATA_MAX_SECTORS_LBA28;
        dma_nb_sectors -= ATA_MAX_SECTORS_LBA28;
        nb_sectors = dma_nb_sectors <= ATA_MAX_SECTORS_LBA28 ? dma_nb_sectors : ATA_MAX_SECTORS_LBA28;

        /* Make sure to acknowledge the IRQ before continuing */
        status = IN8(G1_ATA_STATUS_REG);

        /* If there is an error, stop the DMA chain. */
        if(status & (G1_ATA_SR_ERR | G1_ATA_SR_DF)) {
            dbglog(DBG_ERROR, "g1_dma_irq_hnd: Error detected in DMA chain, aborting\n");
            g1_dma_done();
            return;
        }

        /* Set the DMA parameters for the next transfer. */
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
            ((dma_sector >> 24) & 0x0F));
        g1_ata_set_sector_and_count(dma_sector, nb_sectors, 1);
        OUT32(G1_ATA_DMA_ADDRESS, IN32(G1_ATA_DMA_ADDRESS) + ATA_MAX_SECTORS_LBA28 * 512);
        OUT32(G1_ATA_DMA_LENGTH, nb_sectors * 512);

        if(dma_cmd == ATA_CMD_WRITE_DMA)
            OUT8(G1_ATA_DMA_DIRECTION, G1_DMA_TO_DEVICE);
        else
            OUT8(G1_ATA_DMA_DIRECTION, G1_DMA_TO_MEMORY);

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, dma_cmd);

        /* Re-start the DMA transfer. */
        OUT32(G1_ATA_DMA_STATUS, 1);
    }
    else if(dma_in_progress) {
        g1_dma_done();
    }
}

/* Set the device select register to select a particular device. */
uint8_t g1_ata_select_device(uint8_t dev) {
    uint8_t old = IN8(G1_ATA_DEVICE_SELECT);

    /* Are we actually switching devices? */
    if(((dev ^ dev_selected) & 0x10)) {
        /* We might run into some trouble here if this is called in an IRQ
           handler, so treat that case specially... */
        if(irq_inside_int()) {
            /* If there's a DMA going, then punt. We don't want to sit around
               forever waiting... */
            if(g1_dma_in_progress())
                return 0x0F;

            if(IN8(G1_ATA_ALTSTATUS) & (G1_ATA_SR_DRQ | G1_ATA_SR_BSY))
                return 0x0F;
        }
        else {
            /* Wait for any in-progress DMA transfers to finish. */
            while(g1_dma_in_progress())
                thd_pass();

            /* According to section 7.10 of the ATA-5 spec, setting the device
               select register with either of BSY or DRQ asserted produces an
               indeterminite result. */
            g1_ata_wait_bsydrq();
        }
    }

    /* Write the register value out and return the old value. */
    OUT8(G1_ATA_DEVICE_SELECT, dev);
    dev_selected = dev;

    return old;
}

/* This one is an inline function since it needs to return something... */
static inline int g1_ata_wait_drq(void) {
    uint8_t val = IN8(G1_ATA_ALTSTATUS);

    while(!(val & G1_ATA_SR_DRQ) && !(val & (G1_ATA_SR_ERR | G1_ATA_SR_DF))) {
        val = IN8(G1_ATA_ALTSTATUS);
    }

    return (val & (G1_ATA_SR_ERR | G1_ATA_SR_DF)) ? -1 : 0;
}

static int dma_common(uint8_t cmd, size_t nsects, uint32_t addr, int dir,
                      int block) {
    uint8_t status;

    dma_cmd = cmd;

    /* Set the thread ID that initiated this DMA. */
    dma_thd = thd_current;

    if(irq_inside_int())
        dma_thd = (kthread_t *)0xFFFFFFFF;

    /* Set the DMA parameters up. */
    OUT32(G1_ATA_DMA_ADDRESS, addr);
    OUT32(G1_ATA_DMA_LENGTH, nsects * 512);
    OUT32(G1_ATA_DMA_DIRECTION, dir);

    /* Enable G1 DMA. */
    OUT32(G1_ATA_DMA_ENABLE, 1);

    /* Wait until the drive is ready to accept the command. */
    g1_ata_wait_nbsy();
    g1_ata_wait_drdy();

    /* Write out the command to the device. */
    OUT8(G1_ATA_COMMAND_REG, cmd);

    /* Start the DMA transfer. */
    OUT32(G1_ATA_DMA_STATUS, 1);

    if(block) {
        sem_wait(&dma_done);

        /* Ack the IRQ. */
        status = IN8(G1_ATA_STATUS_REG);

        /* Was there an error doing the transfer? */
        if(status & G1_ATA_SR_ERR) {
            errno = EIO;
            return -1;
        }

        /* Since we're blocking, make sure the drive is completely done. */
        g1_ata_wait_bsydrq();
    }

    return 0;
}

int g1_ata_read_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                    void *buf) {
    int rv = 0;
    unsigned int i, j;
    size_t nsects;
    uint16_t word;
    uint8_t *ptr = (uint8_t *)buf;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENODEV;
        return -1;
    }

    /* Lock the mutex. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > ATA_MAX_SECTORS_LBA28 ? ATA_MAX_SECTORS_LBA28 : count;
        count -= nsects;

        g1_ata_select_device(G1_ATA_SLAVE | (h & 0x0F));

        /* Write out the number of sectors we want as well as the cylinder and
           sector. */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)nsects);
        OUT8(G1_ATA_CHS_SECTOR, s);
        OUT8(G1_ATA_CHS_CYL_LOW, (uint8_t)((c >> 0) & 0xFF));
        OUT8(G1_ATA_CHS_CYL_HIGH, (uint8_t)((c >> 8) & 0xFF));

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);

        /* Now, wait for the drive to give us back each sector. */
        for(i = 0; i < nsects; ++i, ++s) {
            /* Make sure to keep track of where we are, just in case something
               errors out (or we have to deal with a second pass). */
            if(s >= device.sectors) {
                if(++h == device.heads) {
                    h = 0;
                    ++c;
                }

                s = 1;
            }

            /* Wait for data */
            if(g1_ata_wait_drq()) {
                dbglog(DBG_KDEBUG, "g1_ata_read_chs: error reading CHS "
                       "%d, %d, %d of device: %02x\n", (int)c, (int)h, (int)s,
                       IN8(G1_ATA_ALTSTATUS));
                errno = EIO;
                rv = -1;
                goto out;
            }

            for(j = 0; j < 256; ++j) {
                word = IN16(G1_ATA_DATA);
                ptr[0] = word;
                ptr[1] = word >> 8;
                ptr += 2;
            }
        }
    }

    rv = 0;

out:
    g1_ata_mutex_unlock();

    return rv;
}

int g1_ata_write_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                     const void *buf) {
    int rv = 0;
    unsigned int i, j;
    size_t nsects;
    uint16_t word;
    uint8_t *ptr = (uint8_t *)buf;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Lock the mutex. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > ATA_MAX_SECTORS_LBA28 ? ATA_MAX_SECTORS_LBA28 : count;
        count -= nsects;

        g1_ata_select_device(G1_ATA_SLAVE | (h & 0x0F));

        /* Write out the number of sectors we want as well as the cylinder and
           sector. */
        OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)nsects);
        OUT8(G1_ATA_CHS_SECTOR, s);
        OUT8(G1_ATA_CHS_CYL_LOW, (uint8_t)((c >> 0) & 0xFF));
        OUT8(G1_ATA_CHS_CYL_HIGH, (uint8_t)((c >> 8) & 0xFF));

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS);

        /* Now, send the drive each sector. */
        for(i = 0; i < nsects; ++i, ++s) {
            /* Make sure to keep track of where we are, just in case something
               errors out (or we have to deal with a second pass). */
            if(s >= device.sectors) {
                if(++h >= device.heads) {
                    h = 0;
                    ++c;
                }

                s = 1;
            }

            /* Wait for the device to signal it is ready. */
            g1_ata_wait_nbsy();

            /* Send the data! */
            for(j = 0; j < 256; ++j) {
                word = ptr[0] | (ptr[1] << 8);
                OUT16(G1_ATA_DATA, word);
                ptr += 2;
            }
        }
    }

    /* Wait for the device to signal that it has finished writing the data. */
    g1_ata_wait_bsydrq();

    rv = 0;

    g1_ata_mutex_unlock();

    return rv;
}

int g1_ata_read_lba(uint64_t sector, size_t count, void *buf) {
    int rv = 0;
    unsigned int i, j;
    size_t nsects;
    uint16_t word;
    uint8_t *ptr = (uint8_t *)buf;
    int lba28, cmd;
    const size_t max_sectors = CAN_USE_LBA48() ? ATA_MAX_SECTORS_LBA48 : ATA_MAX_SECTORS_LBA28;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Make sure the disk supports LBA mode. */
    if(!device.max_lba) {
        errno = ENOTSUP;
        return -1;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Lock the mutex. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > max_sectors ? max_sectors : count;
        count -= nsects;

        /* Which mode are we using: LBA28 or LBA48? */
        lba28 = use_lba28(sector, nsects);
        if(lba28) {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                                 ((sector >> 24) & 0x0F));
            cmd = ATA_CMD_READ_SECTORS;
        }
        else {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
            cmd = ATA_CMD_READ_SECTORS_EXT;
        }

        /* Write out the number of sectors we want and the LBA. */
        g1_ata_set_sector_and_count(sector, nsects, lba28);

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, cmd);

        /* Now, wait for the drive to give us back each sector. */
        for(i = 0; i < nsects; ++i, ++sector) {
            /* Wait for data */
            if(g1_ata_wait_drq()) {
                dbglog(DBG_KDEBUG, "g1_ata_read_lba: error reading sector %d "
                       "of device: %02x\n", (int)sector, IN8(G1_ATA_ALTSTATUS));
                errno = EIO;
                rv = -1;
                goto out;
            }

            for(j = 0; j < 256; ++j) {
                word = IN16(G1_ATA_DATA);
                ptr[0] = word;
                ptr[1] = word >> 8;
                ptr += 2;
            }
        }
    }

    rv = 0;

out:
    g1_ata_mutex_unlock();

    return rv;
}

int g1_ata_read_lba_dma(uint64_t sector, size_t count, void *buf,
                        int block) {
    int lba28, old, can_lba48 = CAN_USE_LBA48();
    uintptr_t addr;
    uint8_t cmd;

    /* Make sure we're actually being asked to do work... */
    if(!count)
        return 0;

    if(!buf) {
        errno = EFAULT;
        return -1;
    }

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Make sure the disk supports LBA mode. */
    if(!device.max_lba) {
        errno = ENOTSUP;
        return -1;
    }

    /* Make sure the disk supports Multi-Word DMA mode 2. */
    if(!device.wdma_modes) {
        errno = EPERM;
        return -1;
    }

    /* Chaining isn't done yet, so make sure we don't need to. */
    if(count > ATA_MAX_SECTORS_LBA48) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Check the alignment of the address. */
    addr = (uintptr_t)buf;

    if(addr & 0x1F) {
        dbglog(DBG_ERROR, "g1_ata_read_lba_dma: Unaligned output address\n");
        errno = EFAULT;
        return -1;
    }

    /* Invalidate the CPU cache only for cacheable memory areas.
       Otherwise, it is assumed that either this operation is unnecessary
       (another DMA is being used) or that the caller is responsible
       for managing the CPU data cache.
    */
    if((addr & MEM_AREA_P2_BASE) != MEM_AREA_P2_BASE) {
        /* Invalidate the dcache over the range of the data. */
        dcache_inval_range(addr, count * 512);
    }

    /* Use the physical memory address. */
    addr &= MEM_AREA_CACHE_MASK;

    /* Lock the mutex. It will be unlocked later in the IRQ handler. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Disable IRQs temporarily... */
    old = irq_disable();

    /* Make sure there is no DMA in progress already. */
    if(dma_in_progress || g1_dma_in_progress()) {
        irq_restore(old);
        g1_ata_mutex_unlock();
        dbglog(DBG_KDEBUG, "g1_ata_read_lba_dma: DMA in progress\n");
        errno = EIO;
        return -1;
    }

    /* Set the settings for this transfer and re-enable IRQs. */
    dma_blocking = block;
    dma_in_progress = 1;
    dma_nb_sectors = count;
    dma_sector = sector;
    irq_restore(old);

    if(!can_lba48 && count > ATA_MAX_SECTORS_LBA28)
        count = ATA_MAX_SECTORS_LBA28;

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    /* Which mode are we using: LBA28 or LBA48? */
    lba28 = !can_lba48 || use_lba28(sector, count);
    if(lba28) {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                             ((sector >> 24) & 0x0F));
        cmd = ATA_CMD_READ_DMA;
    }
    else {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
        cmd = ATA_CMD_READ_DMA_EXT;
    }

    /* Write out the number of sectors we want and the LBA. */
    g1_ata_set_sector_and_count(sector, count, lba28);

    /* Do the rest of the work... */
    return dma_common(cmd, count, addr, G1_DMA_TO_MEMORY, block);
}

int g1_ata_write_lba(uint64_t sector, size_t count, const void *buf) {
    unsigned int i, j;
    size_t nsects;
    uint16_t word;
    uint8_t *ptr = (uint8_t *)buf;
    int cmd, lba28;
    const size_t max_sectors = CAN_USE_LBA48() ? ATA_MAX_SECTORS_LBA48 : ATA_MAX_SECTORS_LBA28;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Make sure the disk supports LBA mode. */
    if(!device.max_lba) {
        errno = ENOTSUP;
        return -1;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Lock the mutex. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    while(count) {
        nsects = count > max_sectors ? max_sectors : count;
        count -= nsects;

        /* Which mode are we using: LBA28 or LBA48? */
        lba28 = use_lba28(sector, nsects);
        if(lba28) {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                                 ((sector >> 24) & 0x0F));
            cmd = ATA_CMD_WRITE_SECTORS;
        }
        else {
            g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
            cmd = ATA_CMD_WRITE_SECTORS_EXT;
        }

        /* Write out the number of sectors we want and the LBA. */
        g1_ata_set_sector_and_count(sector, nsects, lba28);

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, cmd);

        /* Now, send the drive each sector. */
        for(i = 0; i < nsects; ++i, ++sector) {
            /* Wait for the device to signal it is ready. */
            g1_ata_wait_nbsy();

            /* Send the data! */
            for(j = 0; j < 256; ++j) {
                word = ptr[0] | (ptr[1] << 8);
                OUT16(G1_ATA_DATA, word);
                ptr += 2;
            }
        }
    }

    /* Wait for the device to signal that it has finished writing the data. */
    g1_ata_wait_bsydrq();

    g1_ata_mutex_unlock();

    return 0;
}

int g1_ata_write_lba_dma(uint64_t sector, size_t count, const void *buf,
                         int block) {
    int cmd, lba28, old, can_lba48 = CAN_USE_LBA48();
    uintptr_t addr;

    /* Make sure we're actually being asked to do work... */
    if(!count)
        return 0;

    if(!buf) {
        errno = EFAULT;
        return -1;
    }

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Make sure the disk supports LBA mode. */
    if(!device.max_lba) {
        errno = ENOTSUP;
        return -1;
    }

    /* Make sure the disk supports Multi-Word DMA mode 2. */
    if(!device.wdma_modes) {
        errno = EPERM;
        return -1;
    }

    /* Make sure we don't exceed maximum sector count */
    if(count > ATA_MAX_SECTORS_LBA48) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Check the alignment of the address. */
    addr = (uintptr_t)buf;

    if(addr & 0x1F) {
        dbglog(DBG_ERROR, "g1_ata_write_lba_dma: Unaligned input address\n");
        errno = EFAULT;
        return -1;
    }

    /* Flush the CPU cache only for cacheable memory areas.
       Otherwise, it is assumed that either this operation is unnecessary
       (another DMA is being used) or that the caller is responsible
       for managing the CPU data cache.
    */
    if((addr & MEM_AREA_P2_BASE) != MEM_AREA_P2_BASE) {
        /* Flush the dcache over the range of the data. */
        dcache_flush_range(addr, count * 512);
    }

    /* Use the physical memory address. */
    addr &= MEM_AREA_CACHE_MASK;

    /* Lock the mutex. It will be unlocked in the IRQ handler later. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Disable IRQs temporarily... */
    old = irq_disable();

    /* Make sure there is no DMA in progress already. */
    if(dma_in_progress || g1_dma_in_progress()) {
        irq_restore(old);
        g1_ata_mutex_unlock();
        dbglog(DBG_KDEBUG, "g1_ata_write_lba_dma: DMA in progress\n");
        errno = EIO;
        return -1;
    }

    /* Set the settings for this transfer and re-enable IRQs. */
    dma_blocking = block;
    dma_in_progress = 1;
    dma_nb_sectors = count;
    dma_sector = sector;
    irq_restore(old);

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_bsydrq();

    /* Which mode are we using: LBA28 or LBA48? */
    lba28 = !can_lba48 || use_lba28(sector, count);
    if(lba28) {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE |
                             ((sector >> 24) & 0x0F));
        cmd = ATA_CMD_WRITE_DMA;
        if(count > ATA_MAX_SECTORS_LBA28) {
            count = ATA_MAX_SECTORS_LBA28;
        }
    }
    else {
        g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
        cmd = ATA_CMD_WRITE_DMA_EXT;
    }

    /* Write out the number of sectors we want and the LBA. */
    g1_ata_set_sector_and_count(sector, count, lba28);

    /* Do the rest of the work... */
    return dma_common(cmd, count, addr, G1_DMA_TO_DEVICE, block);
}

int g1_ata_flush(void) {
    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Lock the mutex. */
    if(g1_ata_mutex_lock())
        return -1;

    /* Select the slave device. */
    g1_ata_select_device(G1_ATA_SLAVE | G1_ATA_LBA_MODE);
    timer_spin_sleep(1);

    /* Flush the disk's write cache to make sure everything gets written out. */
    if(CAN_USE_LBA48())
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_FLUSH_CACHE_EXT);
    else
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_FLUSH_CACHE);

    timer_spin_sleep(1);
    g1_ata_wait_bsydrq();
    g1_ata_mutex_unlock();

    return 0;
}

int g1_ata_lba_mode(void) {
    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }
    if(!(device.capabilities & (1 << 9))) {
        return 0; // CHS
    } else if(!(device.command_sets & (1 << 26))) {
        return 28; // LBA28
    }
    return 48; // LBA48
}

static int g1_ata_set_transfer_mode(uint8_t mode) {
    uint8_t status;

    /* Fill in the registers as is required. */
    OUT8(G1_ATA_FEATURES, ATA_FEATURE_TRANSFER_MODE);
    OUT8(G1_ATA_SECTOR_COUNT, mode);
    OUT8(G1_ATA_CHS_SECTOR, 0);
    OUT8(G1_ATA_CHS_CYL_LOW, 0);
    OUT8(G1_ATA_CHS_CYL_HIGH, 0);

    /* Send the SET FEATURES command. */
    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_SET_FEATURES);
    timer_spin_sleep(1);

    /* Wait for command completion. */
    g1_ata_wait_nbsy();

    /* See if the command completed. */
    status = IN8(G1_ATA_STATUS_REG);

    if((status & G1_ATA_SR_ERR) || (status & G1_ATA_SR_DF)) {
        dbglog(DBG_KDEBUG, "Error setting transfer mode %02x\n", mode);
        return -1;
    }

    return 0;
}

static int g1_ata_scan(void) {
    uint8_t dsel = IN8(G1_ATA_DEVICE_SELECT), st;
    int rv, i;
    uint16_t data[256];

    /* Lock the mutex. */
    if(g1_ata_mutex_lock())
        return -1;

    /* For now, just check if there's a slave device. We don't care about the
       primary device, since it should always be the GD-ROM drive. */
    OUT8(G1_ATA_DEVICE_SELECT, 0xF0);
    timer_spin_sleep(1);

    OUT8(G1_ATA_SECTOR_COUNT, 0);
    OUT8(G1_ATA_LBA_LOW, 0);
    OUT8(G1_ATA_LBA_MID, 0);
    OUT8(G1_ATA_LBA_HIGH, 0);

    /* Send the IDENTIFY command. */
    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_IDENTIFY);
    timer_spin_sleep(1);
    st = IN8(G1_ATA_STATUS_REG);

    /* Check if there's anything on the bus. */
    if(!st || st == 0xFF) {
        rv = 0;
        goto out;
    }

    /* Wait for the device to finish. */
    g1_ata_wait_nbsy();

    /* Wait for data. */
    if(g1_ata_wait_drq()) {
        dbglog(DBG_KDEBUG, "g1_ata_scan: error while identifying device\n"
               "             possibly ATAPI? %02x %02x\n",
               IN8(G1_ATA_LBA_MID), IN8(G1_ATA_LBA_HIGH));
        rv = 0;
        goto out;
    }

    /* Read out the data from the device. There will always be 256 words of
       data, according to the spec. */
    for(i = 0; i < 256; ++i)
        data[i] = IN16(G1_ATA_DATA);

    /* Read off some information we might need. */
    device.command_sets = (uint32_t)(data[82]) | ((uint32_t)(data[83]) << 16);
    device.capabilities = (uint32_t)(data[49]) | ((uint32_t)(data[50]) << 16);
    device.wdma_modes = data[63];

    /* See if we support LBA mode or not... */
    if(!(device.capabilities & (1 << 9))) {
        /* Nope. We have to use CHS addressing... >_< */
        device.max_lba = 0;
        device.cylinders = data[1];
        device.heads = data[3];
        device.sectors = data[6];
        dbglog(DBG_KDEBUG, "g1_ata_scan: found device with CHS: %d %d %d\n",
               device.cylinders, device.heads, device.sectors);
    }
    /* Do we support LBA48? */
    else if(!(device.command_sets & (1 << 26))) {
        /* Nope, use LBA28 */
        device.max_lba = (uint64_t)(data[60]) | ((uint64_t)(data[61]) << 16);
        device.cylinders = device.heads = device.sectors = 0;
        dbglog(DBG_KDEBUG, "g1_ata_scan: found device with LBA28: %llu\n",
               device.max_lba);
    }
    else {
        /* Yep, we support LBA48 */
        device.max_lba = (uint64_t)(data[100]) | ((uint64_t)(data[101]) << 16) |
                         ((uint64_t)(data[102]) << 32) |
                         ((uint64_t)(data[103]) << 48);
        device.cylinders = device.heads = device.sectors = 0;
        dbglog(DBG_KDEBUG, "g1_ata_scan: found device with LBA48: %llu\n",
               device.max_lba);
    }

    rv = 1;

    /* Set our transfer modes. */
    if(!g1_ata_set_transfer_mode(ATA_TRANSFER_PIO_DEFAULT)) {
        OUT32(G1_ATA_PIO_RACCESS_WAIT, G1_ACCESS_PIO_DEFAULT);
        OUT32(G1_ATA_PIO_WACCESS_WAIT, G1_ACCESS_PIO_DEFAULT);
    }

    /* Do we support Multiword DMA mode 2? If so, enable it. Otherwise, we won't
       even bother doing DMA at all. */
    if(device.wdma_modes & 0x0004) {
        if(!g1_ata_set_transfer_mode(ATA_TRANSFER_WDMA(2))) {
            OUT32(G1_ATA_DMA_RACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
            OUT32(G1_ATA_DMA_WACCESS_WAIT, G1_ACCESS_WDMA_MODE2);
            OUT32(G1_ATA_DMA_PROTECTION | MEM_AREA_P2_BASE, G1_DMA_UNLOCK_ALLMEM);
        }
        else {
            device.wdma_modes = 0;
        }
    }
    else {
        device.wdma_modes = 0;
    }

out:
    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    g1_ata_mutex_unlock();

    return rv;
}

/* Block device interface. */
static int atab_init(kos_blockdev_t *d) {
    (void)d;

    if(!initted) {
        errno = ENXIO;
        return -1;
    }

    return 0;
}

static int atab_shutdown(kos_blockdev_t *d) {
    free(d->dev_data);
    return 0;
}

static int atab_read_blocks(kos_blockdev_t *d, uint64_t block, size_t count,
                            void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    return g1_ata_read_lba(block + data->start_block, count, (uint16_t *)buf);
}

static int atab_read_blocks_dma(kos_blockdev_t *d, uint64_t block, size_t count,
                                void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    return g1_ata_read_lba_dma(block + data->start_block, count, buf, 1);
}

static int atab_write_blocks(kos_blockdev_t *d, uint64_t block, size_t count,
                             const void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    return g1_ata_write_lba(block + data->start_block, count, buf);
}

static int atab_write_blocks_dma(kos_blockdev_t *d, uint64_t block,
                                 size_t count, const void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    return g1_ata_write_lba_dma(block + data->start_block, count, buf, 1);
}

static int atab_read_blocks_chs(kos_blockdev_t *d, uint64_t block, size_t count,
                                void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;
    uint8_t h, s;
    uint16_t c;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Convert the LBA address to CHS */
    block += data->start_block;
    c = (uint16_t)(block / (device.sectors * device.heads));
    h = (uint8_t)((block / device.sectors) % device.heads);
    s = (uint8_t)((block % device.sectors) + 1);

    return g1_ata_read_chs(c, h, s, count, buf);
}

static int atab_write_blocks_chs(kos_blockdev_t *d, uint64_t block,
                                 size_t count, const void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;
    uint8_t h, s;
    uint16_t c;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Convert the LBA address to CHS */
    block += data->start_block;
    c = (uint16_t)(block / (device.sectors * device.heads));
    h = (uint8_t)((block / device.sectors) % device.heads);
    s = (uint8_t)((block % device.sectors) + 1);

    return g1_ata_write_chs(c, h, s, count, buf);
}

static uint64_t atab_count_blocks(kos_blockdev_t *d) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    return (uint32_t)data->block_count;
}

static int atab_flush(kos_blockdev_t *d) {
    (void)d;
    return g1_ata_flush();
}

static kos_blockdev_t ata_blockdev = {
    NULL,                   /* dev_data */
    9,                      /* l_block_size (block size of 512 bytes) */
    &atab_init,             /* init */
    &atab_shutdown,         /* shutdown */
    &atab_read_blocks,      /* read_blocks */
    &atab_write_blocks,     /* write_blocks */
    &atab_count_blocks,     /* count_blocks */
    &atab_flush             /* flush */
};

static kos_blockdev_t ata_blockdev_dma = {
    NULL,                   /* dev_data */
    9,                      /* l_block_size (block size of 512 bytes) */
    &atab_init,             /* init */
    &atab_shutdown,         /* shutdown */
    &atab_read_blocks_dma,  /* read_blocks */
    &atab_write_blocks_dma, /* write_blocks */
    &atab_count_blocks,     /* count_blocks */
    &atab_flush             /* flush */
};

static kos_blockdev_t ata_blockdev_chs = {
    NULL,                   /* dev_data */
    9,                      /* l_block_size (block size of 512 bytes) */
    &atab_init,             /* init */
    &atab_shutdown,         /* shutdown */
    &atab_read_blocks_chs,  /* read_blocks */
    &atab_write_blocks_chs, /* write_blocks */
    &atab_count_blocks,     /* count_blocks */
    &atab_flush             /* flush */
};

int g1_ata_blockdev_for_partition(int partition, int dma, kos_blockdev_t *rv,
                                  uint8_t *partition_type) {
    uint8_t buf[512];
    int pval;
    ata_devdata_t *ddata;

    if(!initted) {
        errno = ENXIO;
        return -1;
    }

    if(!rv || !partition_type) {
        errno = EFAULT;
        return -1;
    }

    /* Make sure the partition asked for is sane */
    if(partition < 0 || partition > 3) {
        dbglog(DBG_DEBUG, "Invalid partition number given: %d\n", partition);
        errno = EINVAL;
        return -1;
    }

    /* Read the MBR from the disk */
    if(device.max_lba) {
        if(g1_ata_read_lba(0, 1, (uint16_t *)buf) < 0)
            return -1;
    }
    else {
        if(g1_ata_read_chs(0, 0, 1, 1, (uint16_t *)buf) < 0)
            return -1;
    }

    /* Make sure the ATA disk uses MBR partitions.
       TODO: Support GPT partitioning at some point. */
    if(buf[0x01FE] != 0x55 || buf[0x1FF] != 0xAA) {
        dbglog(DBG_DEBUG, "ATA device doesn't appear to have a MBR %02x %02x\n",
               buf[0x01fe], buf[0x01ff]);
        errno = ENOENT;
        return -1;
    }

    /* Figure out where the partition record we're concerned with is, and make
       sure that the partition actually exists. */
    pval = 16 * partition + 0x01BE;

    if(buf[pval + 4] == 0) {
        dbglog(DBG_DEBUG, "Partition %d appears to be empty\n", partition);
        errno = ENOENT;
        return -1;
    }

    /* Allocate the device data */
    if(!(ddata = (ata_devdata_t *)malloc(sizeof(ata_devdata_t)))) {
        errno = ENOMEM;
        return -1;
    }

    /* Copy in the template block device and fill it in */
    if(device.max_lba) {
        if(dma && device.wdma_modes)
            memcpy(rv, &ata_blockdev_dma, sizeof(kos_blockdev_t));
        else
            memcpy(rv, &ata_blockdev, sizeof(kos_blockdev_t));
    }
    else {
        memcpy(rv, &ata_blockdev_chs, sizeof(kos_blockdev_t));
    }

    /* It doesn't matter whether we're using CHS or LBA... We only bother to
       parse out the LBA information from the MBR. Should be valid either
       way (whereas the CHS stuff may very well be invalid). */
    ddata->block_count = buf[pval + 0x0C] | (buf[pval + 0x0D] << 8) |
                         (buf[pval + 0x0E] << 16) | (buf[pval + 0x0F] << 24);
    ddata->start_block = buf[pval + 0x08] | (buf[pval + 0x09] << 8) |
                         (buf[pval + 0x0A] << 16) | (buf[pval + 0x0B] << 24);
    ddata->end_block = ddata->start_block + ddata->block_count - 1;
    rv->dev_data = ddata;
    *partition_type = buf[pval + 4];

    return 0;
}

int g1_ata_blockdev_for_device(int dma, kos_blockdev_t *rv) {
    ata_devdata_t *ddata;

    if(!initted) {
        errno = ENXIO;
        return -1;
    }

    if(!rv) {
        errno = EFAULT;
        return -1;
    }

    /* Allocate the device data */
    if(!(ddata = (ata_devdata_t *)malloc(sizeof(ata_devdata_t)))) {
        errno = ENOMEM;
        return -1;
    }

    /* Copy in the template block device and fill it in */
    if(device.max_lba) {
        if(dma && device.wdma_modes)
            memcpy(rv, &ata_blockdev_dma, sizeof(kos_blockdev_t));
        else
            memcpy(rv, &ata_blockdev, sizeof(kos_blockdev_t));
    }
    else {
        memcpy(rv, &ata_blockdev_chs, sizeof(kos_blockdev_t));
    }

    if(device.max_lba) {
        ddata->end_block = device.max_lba;
    }
    else {
        ddata->end_block = (device.cylinders * device.heads * device.sectors);
    }
    ddata->start_block = 0;
    ddata->block_count = ddata->end_block + 1;
    rv->dev_data = ddata;

    return 0;
}

int g1_ata_init(void) {
    if(initted)
        return 0;

    orig_dev = dev_selected = IN8(G1_ATA_DEVICE_SELECT);

    /* Scan for devices. */
    if((devices = g1_ata_scan()) < 0) {
        devices = 0;
        return -1;
    }

    /* Hook all the DMA related events. */
    asic_evt_set_handler(ASIC_EVT_GD_DMA, g1_dma_irq_hnd, NULL);
    asic_evt_enable(ASIC_EVT_GD_DMA, ASIC_IRQB);
    asic_evt_set_handler(ASIC_EVT_GD_DMA_OVERRUN, g1_dma_irq_hnd, NULL);
    asic_evt_enable(ASIC_EVT_GD_DMA_OVERRUN, ASIC_IRQB);
    asic_evt_set_handler(ASIC_EVT_GD_DMA_ILLADDR, g1_dma_irq_hnd, NULL);
    asic_evt_enable(ASIC_EVT_GD_DMA_ILLADDR, ASIC_IRQB);

    initted = 1;

    return 0;
}

void g1_ata_shutdown(void) {
    /* Make sure to flush any cached data out. */
    if(devices)
        g1_ata_flush();

    /* Reselect whatever was selected at boot (probably the GD-ROM). */
    OUT8(G1_ATA_DEVICE_SELECT, orig_dev);

    devices = 0;
    initted = 0;

    memset(&device, 0, sizeof(device));

    /* Unhook the events and disable the IRQs. */
    asic_evt_disable(ASIC_EVT_GD_DMA, ASIC_IRQB);
    asic_evt_remove_handler(ASIC_EVT_GD_DMA);
    asic_evt_disable(ASIC_EVT_GD_DMA_OVERRUN, ASIC_IRQB);
    asic_evt_remove_handler(ASIC_EVT_GD_DMA_OVERRUN);
    asic_evt_disable(ASIC_EVT_GD_DMA_ILLADDR, ASIC_IRQB);
    asic_evt_remove_handler(ASIC_EVT_GD_DMA_ILLADDR);
}
