/* KallistiOS ##version##

   cdrom.c

   Copyright (C) 2000 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2014 Donald Haase
   Copyright (C) 2023, 2024, 2025 Ruslan Rostovtsev
   Copyright (C) 2024 Andy Barajas

 */
#include <assert.h>

#include <arch/cache.h>
#include <arch/timer.h>
#include <arch/memory.h>

#include <dc/cdrom.h>
#include <dc/g1ata.h>
#include <dc/syscalls.h>

#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/dbglog.h>

/*

This module contains low-level primitives for accessing the CD-Rom (I
refer to it as a CD-Rom and not a GD-Rom, because this code will not
access the GD area, by design). Whenever a file is accessed and a new
disc is inserted, it reads the TOC for the disc in the drive and
gets everything situated. After that it will read raw sectors from
the data track on a standard DC bootable CDR (one audio track plus
one data track in xa1 format).

Most of the information/algorithms in this file are thanks to
Marcus Comstedt. Thanks to Maiwe for the verbose command names and
also for the CDDA playback routines.

Note that these functions may be affected by changing compiler options...
they require their parameters to be in certain registers, which is
normally the case with the default options. If in doubt, decompile the
output and look to make sure.

XXX: This could all be done in a non-blocking way by taking advantage of
command queuing. Every call to syscall_gdrom_send_command returns a 
'request id' which just needs to eventually be checked by cmd_stat. A 
non-blocking version of all functions would simply require manual calls 
to check the status. Doing this would probably allow data reading while 
cdda is playing without hiccups (by severely reducing the number of gd 
commands being sent).
*/

typedef int gdc_cmd_hnd_t;

/* The G1 ATA access mutex */
mutex_t _g1_ata_mutex = MUTEX_INITIALIZER;

static int cur_sector_size = 2048;

/* Shortcut to cdrom_reinit_ex. Typically this is the only thing changed. */
int cdrom_set_sector_size(int size) {
    return cdrom_reinit_ex(-1, -1, size);
}

/* Command execution sequence */
int cdrom_exec_cmd(int cmd, void *param) {
    return cdrom_exec_cmd_timed(cmd, param, 0);
}

int cdrom_exec_cmd_timed(int cmd, void *param, int timeout) {
    int32_t status[4] = {
        0, /* Error code 1 */
        0, /* Error code 2 */
        0, /* Transferred size */
        0  /* ATA status waiting */
    };
    gdc_cmd_hnd_t hnd;
    int n, rv = ERR_OK;
    uint64_t begin;

    assert(cmd > 0 && cmd < CMD_MAX);
    mutex_lock_scoped(&_g1_ata_mutex);

    /* Submit the command */
    for(n = 0; n < 10; ++n) {
        hnd = syscall_gdrom_send_command(cmd, param);
        if (hnd != 0) {
            break;
        }
        syscall_gdrom_exec_server();
        thd_pass();
    }

    if(hnd <= 0)
        return ERR_SYS;

    /* Wait command to finish */
    if(timeout) {
        begin = timer_ms_gettime64();
    }
    do {
        syscall_gdrom_exec_server();
        n = syscall_gdrom_check_command(hnd, status);

        if(n != PROCESSING && n != BUSY) {
            break;
        }
        if(timeout) {
            if((timer_ms_gettime64() - begin) >= (unsigned)timeout) {
                syscall_gdrom_abort_command(hnd);
                syscall_gdrom_exec_server();
                rv = ERR_TIMEOUT;
                dbglog(DBG_ERROR, "cdrom_exec_cmd_timed: Timeout exceeded\n");
                break;
            }
        }
        thd_pass();
    } while(1);

    if(rv != ERR_OK)
        return rv;
    else if(n == COMPLETED || n == STREAMING)
        return ERR_OK;
    else if(n == NO_ACTIVE)
        return ERR_NO_ACTIVE;
    else {
        switch(status[0]) {
            case 2:
                return ERR_NO_DISC;
            case 6:
                return ERR_DISC_CHG;
            default:
                return ERR_SYS;
        }
        if(status[1] != 0)
            return ERR_SYS;
    }
}

/* Return the status of the drive as two integers (see constants) */
int cdrom_get_status(int *status, int *disc_type) {
    int rv = ERR_OK;
    uint32_t params[2];

    /* We might be called in an interrupt to check for ISO cache
       flushing, so make sure we're not interrupting something
       already in progress. */
    if(mutex_lock_irqsafe(&_g1_ata_mutex))
        /* DH: Figure out a better return to signal error */
        return -1;

    do {
        rv = syscall_gdrom_check_drive(params);

        if(rv != BUSY) {
            break;
        }
        thd_pass();
    } while(1);

    mutex_unlock(&_g1_ata_mutex);

    if(rv >= 0) {
        if(status != NULL)
            *status = params[0];

        if(disc_type != NULL)
            *disc_type = params[1];
    }
    else {
        if(status != NULL)
            *status = -1;

        if(disc_type != NULL)
            *disc_type = -1;
    }

    return rv;
}

/* Helper function to account for long-standing typo */
int cdrom_change_dataype(int sector_part, int cdxa, int sector_size) {
    return cdrom_change_datatype(sector_part, cdxa, sector_size);
}

/* Wrapper for the change datatype syscall */
int cdrom_change_datatype(int sector_part, int cdxa, int sector_size) {
    uint32_t params[4];

    mutex_lock_scoped(&_g1_ata_mutex);

    /* Check if we are using default params */
    if(sector_size == 2352) {
        if(cdxa == -1)
            cdxa = 0;

        if(sector_part == -1)
            sector_part = CDROM_READ_WHOLE_SECTOR;
    }
    else {
        if(cdxa == -1) {
            /* If not overriding cdxa, check what the drive thinks we should 
               use */
            syscall_gdrom_check_drive(params);
            cdxa = (params[1] == 32 ? 2048 : 1024);
        }

        if(sector_part == -1)
            sector_part = CDROM_READ_DATA_AREA;

        if(sector_size == -1)
            sector_size = 2048;
    }

    params[0] = 0;              /* 0 = set, 1 = get */
    params[1] = sector_part;    /* Get Data or Full Sector */
    params[2] = cdxa;           /* CD-XA mode 1/2 */
    params[3] = sector_size;    /* sector size */

    cur_sector_size = sector_size;
    return syscall_gdrom_sector_mode(params);
}

/* Re-init the drive, e.g., after a disc change, etc */
int cdrom_reinit(void) {
    /* By setting -1 to each parameter, they fall to the old defaults */
    return cdrom_reinit_ex(-1, -1, -1);
}

/* Enhanced cdrom_reinit, takes the place of the old 'sector_size' function */
int cdrom_reinit_ex(int sector_part, int cdxa, int sector_size) {
    int r;

    do {
        r = cdrom_exec_cmd_timed(CMD_INIT, NULL, 10000);
    } while(r == ERR_DISC_CHG);

    if(r == ERR_NO_DISC || r == ERR_SYS || r == ERR_TIMEOUT) {
        return r;
    }

    r = cdrom_change_datatype(sector_part, cdxa, sector_size);

    return r;
}

/* Read the table of contents */
int cdrom_read_toc(CDROM_TOC *toc_buffer, int session) {
    struct {
        int session;
        void *buffer;
    } params;
    int rv;

    params.session = session;
    params.buffer = toc_buffer;

    rv = cdrom_exec_cmd(CMD_GETTOC2, &params);

    return rv;
}

/* Enhanced Sector reading: Choose mode to read in. */
int cdrom_read_sectors_ex(void *buffer, int sector, int cnt, int mode) {
    struct {
        int sec, num;
        void *buffer;
        int is_test;
    } params;
    int rv = ERR_OK;
    uintptr_t buf_addr = ((uintptr_t)buffer);

    params.sec = sector;    /* Starting sector */
    params.num = cnt;       /* Number of sectors */
    params.is_test = 0;     /* Enable test mode */

    /* The DMA mode blocks the thread it is called in by the way we execute
       gd syscalls. It does however allow for other threads to run. */
    /* XXX: DMA Mode may conflict with using a second G1ATA device. More 
       testing is needed from someone with such a device.
    */
    if(mode == CDROM_READ_DMA) {
        if(buf_addr & 0x1f) {
            dbglog(DBG_ERROR, "cdrom_read_sectors_ex: Unaligned memory for DMA (32-byte).\n");
            return ERR_SYS;
        }
        /* Use the physical memory address. */
        params.buffer = (void *)(buf_addr & MEM_AREA_CACHE_MASK);

        /* Invalidate the CPU cache only for cacheable memory areas.
           Otherwise, it is assumed that either this operation is unnecessary
           (another DMA is being used) or that the caller is responsible
           for managing the CPU data cache.
        */
        if((buf_addr & MEM_AREA_P2_BASE) != MEM_AREA_P2_BASE) {
            /* Invalidate the dcache over the range of the data. */
            dcache_inval_range(buf_addr, cnt * cur_sector_size);
        }
        rv = cdrom_exec_cmd(CMD_DMAREAD, &params);
    }
    else if(mode == CDROM_READ_PIO) {
        params.buffer = buffer;

        if(buf_addr & 0x01) {
            dbglog(DBG_ERROR, "cdrom_read_sectors_ex: Unaligned memory for PIO (2-byte).\n");
            return ERR_SYS;
        }
        rv = cdrom_exec_cmd(CMD_PIOREAD, &params);
    }

    return rv;
}

/* Basic old sector read */
int cdrom_read_sectors(void *buffer, int sector, int cnt) {
    return cdrom_read_sectors_ex(buffer, sector, cnt, CDROM_READ_PIO);
}


/* Read a piece of or all of the Q byte of the subcode of the last sector read.
   If you need the subcode from every sector, you cannot read more than one at 
   a time. */
/* XXX: Use some CD-Gs and other stuff to test if you get more than just the 
   Q byte */
int cdrom_get_subcode(void *buffer, int buflen, int which) {
    struct {
        int which;
        int buflen;
        void *buffer;
    } params;
    int rv;

    params.which = which;
    params.buflen = buflen;
    params.buffer = buffer;
    rv = cdrom_exec_cmd(CMD_GETSCD, &params);
    return rv;
}

/* Locate the LBA sector of the data track; use after reading TOC */
uint32 cdrom_locate_data_track(CDROM_TOC *toc) {
    int i, first, last;

    first = TOC_TRACK(toc->first);
    last = TOC_TRACK(toc->last);

    if(first < 1 || last > 99 || first > last)
        return 0;

    /* Find the last track which as a CTRL of 4 */
    for(i = last; i >= first; i--) {
        if(TOC_CTRL(toc->entry[i - 1]) == 4)
            return TOC_LBA(toc->entry[i - 1]);
    }

    return 0;
}

/* Play CDDA tracks
   start  -- track to play from
   end    -- track to play to
   repeat -- number of times to repeat (0-15, 15=infinite)
   mode   -- CDDA_TRACKS or CDDA_SECTORS
 */
int cdrom_cdda_play(uint32 start, uint32 end, uint32 repeat, int mode) {
    struct {
        int start;
        int end;
        int repeat;
    } params;
    int rv = ERR_OK;

    /* Limit to 0-15 */
    if(repeat > 15)
        repeat = 15;

    params.start = start;
    params.end = end;
    params.repeat = repeat;

    if(mode == CDDA_TRACKS)
        rv = cdrom_exec_cmd(CMD_PLAY, &params);
    else if(mode == CDDA_SECTORS)
        rv = cdrom_exec_cmd(CMD_PLAY2, &params);

    return rv;
}

/* Pause CDDA audio playback */
int cdrom_cdda_pause(void) {
    int rv;
    rv = cdrom_exec_cmd(CMD_PAUSE, NULL);
    return rv;
}

/* Resume CDDA audio playback */
int cdrom_cdda_resume(void) {
    int rv;
    rv = cdrom_exec_cmd(CMD_RELEASE, NULL);
    return rv;
}

/* Spin down the CD */
int cdrom_spin_down(void) {
    int rv;
    rv = cdrom_exec_cmd(CMD_STOP, NULL);
    return rv;
}

/*
    Unlocks G1 ATA DMA access to all memory on the root bus, not just system memory.
    Patches syscall region where the DMA protection register is set,
    ensuring it allows broader memory access, and updates the register accordingly.
 */
static void unlock_dma_memory(void) {
    uint32_t i, patched = 0;
    size_t flush_size;
    volatile uint32_t *prot_reg = (uint32_t *)(G1_ATA_DMA_PROTECTION | MEM_AREA_P2_BASE);
    uintptr_t patch_addr[2] = {0x0c001c20, 0x0c0023fc};

    for(i = 0; i < sizeof(patch_addr) / sizeof(uintptr_t); ++i) {
        if(*(uint32_t *)(patch_addr[i] | MEM_AREA_P2_BASE) == (uint32_t)G1_ATA_DMA_UNLOCK_SYSMEM) {
            *(uint32_t *)(patch_addr[i] | MEM_AREA_P2_BASE) = G1_ATA_DMA_UNLOCK_ALLMEM;
            ++patched;
        }
    }
    if(patched) {
        flush_size = (patch_addr[1] - patch_addr[0]) + CPU_CACHE_BLOCK_SIZE;
        flush_size &= ~(CPU_CACHE_BLOCK_SIZE - 1);
        icache_flush_range(patch_addr[0] | MEM_AREA_P1_BASE, flush_size);
    }
    *prot_reg = G1_ATA_DMA_UNLOCK_ALLMEM;
}

/* Initialize: assume no threading issues */
void cdrom_init(void) {
    uint32_t p;
    volatile uint32_t *react = (uint32_t *)(G1_ATA_BUS_PROTECTION | MEM_AREA_P2_BASE);
    volatile uint32_t *state = (uint32_t *)(G1_ATA_BUS_PROTECTION_STATUS | MEM_AREA_P2_BASE);
    volatile uint32_t *bios = (uint32_t *)MEM_AREA_P2_BASE;

    mutex_lock(&_g1_ata_mutex);

    /*
        First, check the protection status to determine if it's necessary 
        to pass check the entire BIOS again.
    */
    if (*state != G1_ATA_BUS_PROTECTION_STATUS_PASSED) {
        /* Reactivate drive: send the BIOS size and then read each
        word across the bus so the controller can verify it.
        If first bytes are 0xe6ff instead of usual 0xe3ff, then
        hardware is fitted with custom BIOS using magic bootstrap
        which can and must pass controller verification with only
        the first 1024 bytes */
        if((*(uint16_t *)MEM_AREA_P2_BASE) == 0xe6ff) {
            *react = 0x3ff;
            for(p = 0; p < 0x400 / sizeof(bios[0]); p++) {
                (void)bios[p];
            }
        } else {
            *react = 0x1fffff;
            for(p = 0; p < 0x200000 / sizeof(bios[0]); p++) {
                (void)bios[p];
            }
        }
    }

    syscall_gdrom_init();

    unlock_dma_memory();
    mutex_unlock(&_g1_ata_mutex);

    cdrom_reinit();
}

void cdrom_shutdown(void) {

    /* What would you want done here? */
}
