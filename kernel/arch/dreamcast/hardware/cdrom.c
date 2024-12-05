/* KallistiOS ##version##

   cdrom.c

   Copyright (C) 2000 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2014, 2024 Donald Haase
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Andy Barajas

 */
#include <assert.h>

#include <arch/timer.h>
#include <arch/memory.h>

#include <dc/cdrom.h>
#include <dc/g1ata.h>

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

/* The G1 ATA access mutex */
mutex_t _g1_ata_mutex = MUTEX_INITIALIZER;

/* Shortcut to cdrom_reinit_ex. Typically this is the only thing changed. */
cd_cmd_ret_t cdrom_set_sector_size(size_t size) {
    return cdrom_reinit_ex(CD_READ_DEFAULT, CD_TRACK_TYPE_DEFAULT, size);
}

/* Command execution sequence */
cd_cmd_ret_t cdrom_exec_cmd(cd_cmd_code_t cmd, void *param) {
    return cdrom_exec_cmd_timed(cmd, param, 0);
}

/* A helper function for the inner loop of issuing a command */
static inline gdc_cmd_hnd_t cdrom_req_cmd(cd_cmd_code_t cmd, void *param) {
    gdc_cmd_hnd_t hnd;
    uint32_t        i;

    assert(cmd > 0 && cmd < CMD_MAX);

    /* Submit the command */
    for(i = 0; i < CD_CMD_RETRY_MAX; ++i) {
        hnd = syscall_gdrom_send_command(cmd, param);
        if (hnd != 0) {
            break;
        }
        syscall_gdrom_exec_server();
        thd_pass();
    }

    return hnd;
}

cd_cmd_ret_t cdrom_exec_cmd_timed(cd_cmd_code_t cmd, void *param, uint32_t timeout) {
    cd_cmd_chk_status_t status = {0};
    gdc_cmd_hnd_t hnd;
    cd_cmd_chk_t n;
    uint64_t begin;

    mutex_lock_scoped(&_g1_ata_mutex);

    hnd = cdrom_req_cmd(cmd, param);

    if(hnd <= 0)
        return CD_ERR_SYS;

    /* Wait command to finish */
    if(timeout) {
        begin = timer_ms_gettime64();
    }
    do {
        syscall_gdrom_exec_server();
        n = syscall_gdrom_check_command(hnd, status);

        if(n != CD_CMD_PROCESSING && n != CD_CMD_BUSY) {
            break;
        }
        if(timeout) {
            if((timer_ms_gettime64() - begin) >= (unsigned)timeout) {
                syscall_gdrom_abort_command(hnd);
                syscall_gdrom_exec_server();
                dbglog(DBG_ERROR, "cdrom_exec_cmd_timed: Timeout exceeded\n");
                return CD_ERR_TIMEOUT;
            }
        }
        thd_pass();
    } while(1);

    if(n == CD_CMD_COMPLETED || n == CD_CMD_STREAMING)
        return CD_ERR_OK;
    else if(n == CD_CMD_NOT_FOUND)
        return CD_ERR_NO_ACTIVE;
    else {
        switch(status.err1) {
            case 2:
                return CD_ERR_NO_DISC;
            case 6:
                return CD_ERR_DISC_CHG;
            default:
                return CD_ERR_SYS;
        }
        if(status.err2 != 0)
            return CD_ERR_SYS;
    }
}

cd_cmd_ret_t cdrom_abort_cmd(uint32_t timeout, gdc_cmd_hnd_t hnd) {
    cd_cmd_chk_status_t status = {0};
    cd_cmd_chk_t rs;
    uint64_t begin;

    if(hnd <= 0) {
        return CD_ERR_NO_ACTIVE;
    }

    mutex_lock_scoped(&_g1_ata_mutex);
    syscall_gdrom_abort_command(hnd);

    if(timeout) {
        begin = timer_ms_gettime64();
    }
    do {
        syscall_gdrom_exec_server();
        rs = syscall_gdrom_check_command(hnd, status);

        if(rs == CD_CMD_NOT_FOUND || rs == CD_CMD_COMPLETED) {
            break;
        }
        if(timeout) {
            if((timer_ms_gettime64() - begin) >= (unsigned)timeout) {
                dbglog(DBG_ERROR, "cdrom_abort_cmd: Timeout exceeded, resetting.\n");
                syscall_gdrom_reset();
                syscall_gdrom_init();
                return CD_ERR_TIMEOUT;
            }
        }
        thd_pass();
    } while(1);

    return CD_ERR_OK;
}

/* Return the status of the drive as two integers (see constants) */
int cdrom_get_status(int *status, int *disc_type) {
    int rv = CD_ERR_OK;
    cd_check_drive_status_t params = {0};

    /* We might be called in an interrupt to check for ISO cache
       flushing, so make sure we're not interrupting something
       already in progress. */
    if(mutex_lock_irqsafe(&_g1_ata_mutex))
        /* DH: Figure out a better return to signal error */
        return -1;

    do {
        rv = syscall_gdrom_check_drive(params);
        /* DH: This may not be correct, it's unclear if check_drive 
        returns the same as check_command */
        if(rv != CD_CMD_BUSY) {
            break;
        }
        thd_pass();
    } while(1);

    mutex_unlock(&_g1_ata_mutex);

    if(rv >= 0) {
        if(status != NULL)
            *status = params.status;

        if(disc_type != NULL)
            *disc_type = params.disc_type;
    }
    else {
        if(status != NULL)
            *status = -1;

        if(disc_type != NULL)
            *disc_type = -1;
    }

    return rv;
}

/* Wrapper for the change datatype syscall */
int cdrom_change_datatype(cd_read_sec_part_t sector_part, cd_track_type_t track_type, int sector_size) {
    cd_check_drive_status_t check_status = {0};
    cd_sec_mode_params_t params;

    mutex_lock_scoped(&_g1_ata_mutex);

    /* Check if we are using default params */
    if(sector_size == 2352) {
        if(track_type == CD_TRACK_TYPE_DEFAULT)
            track_type = CD_TRACK_TYPE_ANY;

        if(sector_part == CD_READ_DEFAULT)
            sector_part = CD_READ_WHOLE_SECTOR;
    }
    else {
        if(track_type == CD_TRACK_TYPE_DEFAULT) {
            /* If not overriding track_type, check what the drive thinks we should 
               use */
            syscall_gdrom_check_drive(check_status);

            if(check_status.disc_type == CD_CDROM_XA)
                track_type = CD_TRACK_TYPE_MODE2_FORM1;
            else
                track_type = CD_TRACK_TYPE_MODE1;
        }

        if(sector_part == CD_READ_DEFAULT)
            sector_part = CD_READ_DATA_AREA;

        if(sector_size == -1)
            sector_size = 2048;
    }

    params.RW           = 0;            /* 0 = set, 1 = get */
    params.sector_part  = sector_part;
    params.track_type   = track_type;
    params.sector_size  = sector_size;

    return syscall_gdrom_sector_mode(params);
}

/* Re-init the drive, e.g., after a disc change, etc */
cd_cmd_ret_t cdrom_reinit(void) {
    /* By setting -1 to each parameter, they fall to the old defaults */
    return cdrom_reinit_ex(CD_READ_DEFAULT, CD_TRACK_TYPE_DEFAULT, -1);
}

/* Enhanced cdrom_reinit, takes the place of the old 'sector_size' function */
cd_cmd_ret_t cdrom_reinit_ex(cd_read_sec_part_t sector_part, cd_track_type_t track_type, int sector_size) {
    int r;

    do {
        r = cdrom_exec_cmd_timed(CMD_INIT, NULL, 10000);
    } while(r == CD_ERR_DISC_CHG);

    if(r == CD_ERR_NO_DISC || r == CD_ERR_SYS || r == CD_ERR_TIMEOUT) {
        return r;
    }

    return cdrom_change_datatype(sector_part, track_type, sector_size);
}

/* Read the table of contents */
cd_cmd_ret_t cdrom_read_toc(cd_toc_t *toc_buffer, cd_area_t area) {
    cd_cmd_toc_params_t params;

    params.area = area;
    params.buffer = toc_buffer;

    return cdrom_exec_cmd(CMD_GETTOC2, &params);
}

/* Enhanced Sector reading: Choose mode to read in. */
cd_cmd_ret_t cdrom_read_sectors_ex(void *buffer, uint32_t sector, size_t cnt, cd_read_mode_t mode) {
    cd_read_params_t params;
    uintptr_t buf_addr = ((uintptr_t)buffer);

    params.start_sec = sector;  /* Starting sector */
    params.num_sec = cnt;       /* Number of sectors */
    params.buffer = buffer;     /* Output buffer */
    params.is_test = false;     /* Enable test mode */

    /* The DMA mode blocks the thread it is called in by the way we execute
       gd syscalls. It does however allow for other threads to run. */
    if(mode == CDROM_READ_DMA) {
        if(buf_addr & 0x1f) {
            dbglog(DBG_ERROR, "cdrom_read_sectors_ex: Unaligned memory for DMA (32-byte).\n");
            return CD_ERR_SYS;
        }
        return cdrom_exec_cmd(CMD_DMAREAD, &params);
    }
    else if(mode == CDROM_READ_PIO) {
        if(buf_addr & 0x01) {
            dbglog(DBG_ERROR, "cdrom_read_sectors_ex: Unaligned memory for PIO (2-byte).\n");
            return CD_ERR_SYS;
        }
        return cdrom_exec_cmd(CMD_PIOREAD, &params);
    }
    else
        return CD_ERR_SYS;
}

/* Basic old sector read */
cd_cmd_ret_t cdrom_read_sectors(void *buffer, uint32_t sector, size_t cnt) {
    return cdrom_read_sectors_ex(buffer, sector, cnt, CDROM_READ_PIO);
}


/* Read a piece of or all of the Q byte of the subcode of the last sector read.
   If you need the subcode from every sector, you cannot read more than one at 
   a time. */
/* XXX: Use some CD-Gs and other stuff to test if you get more than just the 
   Q byte */
cd_cmd_ret_t cdrom_get_subcode(void *buffer, size_t buflen, cd_sub_type_t which) {
    cd_cmd_getscd_params_t params;

    params.which = which;
    params.buflen = buflen;
    params.buffer = buffer;
    return cdrom_exec_cmd(CMD_GETSCD, &params);
}

/* Locate the LBA sector of the data track; use after reading TOC */
uint32_t cdrom_locate_data_track(cd_toc_t *toc) {
    uint32_t i, first, last;

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

/* Play CDDA tracks */
cd_cmd_ret_t cdrom_cdda_play(uint32 start, uint32 end, uint32 repeat, cd_cdda_mode_t mode) {
    cd_cmd_play_params_t params;

    /* Limit to 0-15 */
    if(repeat > 15)
        repeat = 15;

    params.start = start;
    params.end = end;
    params.repeat = repeat;

    if(mode == CDDA_TRACKS)
        return cdrom_exec_cmd(CMD_PLAY, &params);
    else if(mode == CDDA_SECTORS)
        return cdrom_exec_cmd(CMD_PLAY2, &params);
    else
        return CD_ERR_SYS;
}

/* Pause CDDA audio playback */
cd_cmd_ret_t cdrom_cdda_pause(void) {
    return cdrom_exec_cmd(CMD_PAUSE, NULL);
}

/* Resume CDDA audio playback */
cd_cmd_ret_t cdrom_cdda_resume(void) {
    return cdrom_exec_cmd(CMD_RELEASE, NULL);
}

/* Spin down the CD */
cd_cmd_ret_t cdrom_spin_down(void) {
    return cdrom_exec_cmd(CMD_STOP, NULL);
}

/* Initialize: assume no threading issues */
void cdrom_init(void) {
    uint32_t p;
    volatile uint32_t *react = (uint32_t *)(0x005f74e4 | MEM_AREA_P2_BASE);
    volatile uint32_t *bios = (uint32_t *)MEM_AREA_P2_BASE;

    mutex_lock(&_g1_ata_mutex);

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

    /* Reset system functions */
    syscall_gdrom_reset();
    syscall_gdrom_init();
    mutex_unlock(&_g1_ata_mutex);

    cdrom_reinit();
}

void cdrom_shutdown(void) {

    /* What would you want done here? */
}
