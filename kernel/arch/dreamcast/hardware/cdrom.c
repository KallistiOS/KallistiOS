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
#include <kos/timer.h>
#include <dc/memory.h>
#include <arch/irq.h>

#include <dc/asic.h>
#include <dc/cdrom.h>
#include <dc/g1ata.h>
#include <dc/syscalls.h>
#include <dc/vblank.h>

#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/sem.h>
#include <kos/dbglog.h>

/*

This module contains low-level primitives for accessing the CD-Rom (I
refer to it as a CD-Rom and not a GD-Rom, because this code will not
access the GD area, by design). Whenever a file is accessed and a new
disc is inserted, it reads the TOC for the disc in the drive and
gets everything situated. After that it will read raw sectors from
the data track on a standard DC bootable CDR (one audio track plus
one data track in xa1 format).

Initial information/algorithms in this file are thanks to
Marcus Comstedt. Thanks to Maiwe for the verbose command names and
also for the CDDA playback routines.

*/

struct cmd_req_data {
    int cmd;
    void *data;
};

struct cmd_transfer_data {
    gdc_cmd_hnd_t hnd;
    size_t size;
};

/* The G1 ATA access semaphore */
semaphore_t _g1_ata_sem = SEM_INITIALIZER(1);

/* Command handling */
static gdc_cmd_hnd_t cmd_hnd = 0;
static int cmd_response = NO_ACTIVE;
static cd_cmd_chk_status_t cmd_status = { 0 };

/* DMA and IRQ handling */
static bool dma_in_progress = false;
static bool dma_blocking = false;
static bool dma_auto_unlock = false;
static semaphore_t dma_done = SEM_INITIALIZER(0);
static asic_evt_handler_entry_t old_dma_irq = {NULL, NULL};
static int vblank_hnd = -1;

/* Streaming */
static bool stream_enabled = false;
static bool stream_dma = false;
static cdrom_stream_callback_t stream_cb = NULL;
static void *stream_cb_param = NULL;

/* Initialization */
static bool inited = false;
static int cur_sector_size = 2048;

/* Shortcut to cdrom_reinit_ex. Typically this is the only thing changed. */
int cdrom_set_sector_size(int size) {
    return cdrom_reinit_ex(-1, -1, size);
}

static int cdrom_poll(void *d, uint32_t timeout, int (*cb)(void *)) {
    uint64_t start_time;
    int ret;

    if(timeout)
        start_time = timer_ms_gettime64();

    do {
        ret = (*cb)(d);
        if(ret)
            return ret;

        if(!irq_inside_int())
            thd_pass();
    } while(!timeout || (timer_ms_gettime64() - start_time) < timeout);

    return ERR_TIMEOUT;
}

static gdc_cmd_hnd_t cdrom_submit_cmd(void *d) {
    struct cmd_req_data *req = d;
    gdc_cmd_hnd_t ret;

    ret = syscall_gdrom_send_command(req->cmd, req->data);

    syscall_gdrom_exec_server();

    return ret;
}

static inline gdc_cmd_hnd_t cdrom_req_cmd(cd_cmd_code_t cmd, void *param) {
    struct cmd_req_data req = { cmd, param };

    assert(cmd > 0 && cmd < CD_CMD_MAX);

    /* Submit the command, retry if needed for 10ms */
    return (gdc_cmd_hnd_t)cdrom_poll(&req, 10, (int (*)(void *))cdrom_submit_cmd);
}

static int cdrom_check_ready(void *d) {
    syscall_gdrom_exec_server();

    cmd_response = syscall_gdrom_check_command(*(int *)d, &cmd_status);
    if(cmd_response < 0)
        return ERR_SYS;

    return cmd_response != BUSY;
}

static int cdrom_check_cmd_done(void *d) {
    syscall_gdrom_exec_server();

    cmd_response = syscall_gdrom_check_command(*(int *)d, &cmd_status);
    if(cmd_response < 0)
        return ERR_SYS;

    return cmd_response != BUSY && cmd_response != PROCESSING;
}

static int cdrom_check_drive_ready(cd_check_drive_status_t *d) {
    return (syscall_gdrom_check_drive(d) != BUSY);
}

static int cdrom_check_abort_done(void *d) {
    syscall_gdrom_exec_server();

    cmd_response = syscall_gdrom_check_command(*(gdc_cmd_hnd_t *)d, &cmd_status);
    if(cmd_response < 0)
        return ERR_SYS;

    return cmd_response == NO_ACTIVE || cmd_response == COMPLETED;
}

static int cdrom_check_abort_streaming(void *d) {
    syscall_gdrom_exec_server();

    cmd_response = syscall_gdrom_check_command(*(gdc_cmd_hnd_t *)d, &cmd_status);
    if(cmd_response < 0)
        return ERR_SYS;

    return cmd_response == NO_ACTIVE || cmd_response == COMPLETED
        || cmd_response == STREAMING;
}

static int cdrom_check_transfer(void *d) {
    struct cmd_transfer_data *data = d;

    syscall_gdrom_exec_server();

    cmd_response = syscall_gdrom_check_command(data->hnd, &cmd_status);
    if(cmd_response < 0)
        return ERR_SYS;

    if(cmd_response == NO_ACTIVE || cmd_response == COMPLETED)
        return ERR_NO_ACTIVE;

    return cdrom_stream_progress(&data->size) == 0;
}

/* Command execution sequence */
int cdrom_exec_cmd(cd_cmd_code_t cmd, void *param) {
    return cdrom_exec_cmd_timed(cmd, param, 0);
}

int cdrom_exec_cmd_timed(cd_cmd_code_t cmd, void *param, uint32_t timeout) {

    sem_wait_scoped(&_g1_ata_sem);
    cmd_hnd = cdrom_req_cmd(cmd, param);

    if(cmd_hnd <= 0) {
        return ERR_SYS;
    }

    /* Start the process of executing the command. */
    if(cdrom_poll(&cmd_hnd, timeout, cdrom_check_cmd_done) == ERR_TIMEOUT) {
        cdrom_abort_cmd(1000, true);
        return ERR_TIMEOUT;
    }

    if(cmd_response != STREAMING) {
        cmd_hnd = 0;
    }

    if(cmd_response == COMPLETED || cmd_response == STREAMING) {
        return ERR_OK;
    }
    else if(cmd_response == NO_ACTIVE) {
        return ERR_NO_ACTIVE;
    }
    else if(cmd_status.err1 == 2) {
        return ERR_NO_DISC;
    }
    else if(cmd_status.err1 == 6) {
        return ERR_DISC_CHG;
    }

    return ERR_SYS;
}

int cdrom_abort_cmd(uint32_t timeout, bool abort_dma) {
    int rv = ERR_OK;
    irq_mask_t old = irq_disable();

    if(cmd_hnd <= 0) {
        irq_restore(old);
        return ERR_NO_ACTIVE;
    }

    if(abort_dma && dma_in_progress) {
        dma_in_progress = false;
        dma_blocking = false;
        dma_auto_unlock = false;
        /* G1 ATA mutex already locked */
    }
    else {
        sem_wait(&_g1_ata_sem);
    }

    irq_restore(old);
    syscall_gdrom_abort_command(cmd_hnd);

    if(cdrom_poll(&cmd_hnd, timeout, cdrom_check_abort_done) == ERR_TIMEOUT) {
        dbglog(DBG_ERROR, "cdrom_abort_cmd: Timeout exceeded, resetting.\n");
        rv = ERR_TIMEOUT;
        syscall_gdrom_reset();
        syscall_gdrom_init();
    }

    cmd_hnd = 0;
    stream_enabled = false;

    if(stream_cb) {
        cdrom_stream_set_callback(0, NULL);
    }

    sem_signal(&_g1_ata_sem);
    return rv;
}

/* Return the status of the drive as two integers (see constants) */
int cdrom_get_status(int *status, int *disc_type) {
    uint32_t params[2] = {0};
    int rv;

    /* We might be called in an interrupt to check for ISO cache
       flushing, so make sure we're not interrupting something
       already in progress. */
    if(sem_wait_irqsafe(&_g1_ata_sem))
        /* DH: Figure out a better return to signal error */
        return -1;

    rv = cdrom_poll(params, 0, (int (*)(void *))cdrom_check_drive_ready);

    sem_signal(&_g1_ata_sem);

    if(rv >= 0) {
        rv = ERR_OK;

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

/* Wrapper for the change datatype syscall */
int cdrom_change_datatype(cd_read_sec_part_t sector_part, int cdxa, int sector_size) {
    cd_check_drive_status_t status;
    uint32_t params[4];

    sem_wait_scoped(&_g1_ata_sem);

    /* Check if we are using default params */
    if(sector_size == 2352) {
        if(cdxa == -1)
            cdxa = 0;

        if(sector_part == CDROM_READ_DEFAULT)
            sector_part = CDROM_READ_WHOLE_SECTOR;
    }
    else {
        if(cdxa == -1) {
            /* If not overriding cdxa, check what the drive thinks we should 
               use */
            syscall_gdrom_check_drive(&status);
            cdxa = (status.disc_type == CD_CDROM_XA ? 2048 : 1024);
        }

        if(sector_part == CDROM_READ_DEFAULT)
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
    return cdrom_reinit_ex(CDROM_READ_DEFAULT, -1, -1);
}

/* Enhanced cdrom_reinit, takes the place of the old 'sector_size' function */
int cdrom_reinit_ex(cd_read_sec_part_t sector_part, int cdxa, int sector_size) {
    int r;

    do {
        r = cdrom_exec_cmd_timed(CD_CMD_INIT, NULL, 10000);
    } while(r == ERR_DISC_CHG);

    if(r == ERR_NO_DISC || r == ERR_SYS || r == ERR_TIMEOUT) {
        return r;
    }

    return cdrom_change_datatype(sector_part, cdxa, sector_size);
}

/* Read the table of contents */
int cdrom_read_toc(cd_toc_t *toc_buffer, bool high_density) {
    cd_cmd_toc_params_t params;

    params.area = high_density ? CD_AREA_HIGH : CD_AREA_LOW;
    params.buffer = toc_buffer;

    return cdrom_exec_cmd(CD_CMD_GETTOC2, &params);
}

static int cdrom_read_sectors_dma_irq(cd_read_params_t *params) {

    sem_wait_scoped(&_g1_ata_sem);
    cmd_hnd = cdrom_req_cmd(CD_CMD_DMAREAD, params);

    if(cmd_hnd <= 0) {
        return ERR_SYS;
    }
    dma_in_progress = true;
    dma_blocking = true;

    /* Start the process of executing the command. */
    cdrom_poll(&cmd_hnd, 0, cdrom_check_ready);

    if(cmd_response == PROCESSING) {
        /* Wait DMA is finished or command failed. */
        sem_wait(&dma_done);

        /* Just to make sure the command is finished properly.
           Usually we are already done here. */
        cdrom_poll(&cmd_hnd, 0, cdrom_check_cmd_done);
    }
    else {
        /* The command can complete or fails immediately,
           in this case we just countdown the semaphore if needed.
        */
        if(sem_count(&dma_done) > 0) {
            sem_wait(&dma_done);
        }
    }

    cmd_hnd = 0;

    if(cmd_response == COMPLETED || cmd_response == NO_ACTIVE) {
        return ERR_OK;
    }
    else if(cmd_status.err1 == 2) {
        return ERR_NO_DISC;
    }
    else if(cmd_status.err1 == 6) {
        return ERR_DISC_CHG;
    }

    return ERR_SYS;
}

/* Enhanced Sector reading: Choose mode to read in. */
int cdrom_read_sectors_ex(void *buffer, uint32_t sector, size_t cnt, bool dma) {
    cd_read_params_t params;
    uintptr_t buf_addr = ((uintptr_t)buffer);

    params.start_sec = sector;  /* Starting sector */
    params.num_sec = cnt;       /* Number of sectors */
    params.is_test = 0;         /* Enable test mode */

    if(dma) {
        if(!__builtin_is_aligned(buf_addr, 32)) {
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
        return cdrom_read_sectors_dma_irq(&params);
    }
    else {
        params.buffer = buffer;

        if(!__builtin_is_aligned(buf_addr, 2)) {
            dbglog(DBG_ERROR, "cdrom_read_sectors_ex: Unaligned memory for PIO (2-byte).\n");
            return ERR_SYS;
        }
        return cdrom_exec_cmd(CD_CMD_PIOREAD, &params);
    }

    return ERR_OK;
}

/* Basic old sector read */
int cdrom_read_sectors(void *buffer, uint32_t sector, size_t cnt) {
    return cdrom_read_sectors_ex(buffer, sector, cnt, false);
}

int cdrom_stream_start(int sector, int cnt, bool dma) {
    struct {
        int sec;
        int num;
    } params;
    int rv = ERR_SYS;

    params.sec = sector;
    params.num = cnt;

    if(stream_enabled) {
        cdrom_stream_stop(false);
    }
    stream_dma = dma;

    if(stream_dma) {
        rv = cdrom_exec_cmd_timed(CD_CMD_DMAREAD_STREAM, &params, 0);
    }
    else {
        rv = cdrom_exec_cmd_timed(CD_CMD_PIOREAD_STREAM, &params, 0);
    }

    if(rv != ERR_OK) {
        stream_enabled = false;
    }
    return rv;
}

int cdrom_stream_stop(bool abort_dma) {
    if(cmd_hnd <= 0) {
        return ERR_OK;
    }
    if(abort_dma && dma_in_progress) {
        return cdrom_abort_cmd(1000, true);
    }
    sem_wait(&_g1_ata_sem);

    cdrom_poll(&cmd_hnd, 0, cdrom_check_abort_streaming);

    if(cmd_response == STREAMING) {
        sem_signal(&_g1_ata_sem);
        return cdrom_abort_cmd(1000, false);
    }

    cmd_hnd = 0;
    stream_enabled = false;
    sem_signal(&_g1_ata_sem);

    if(stream_cb) {
        cdrom_stream_set_callback(0, NULL);
    }
    return ERR_OK;
}

int cdrom_stream_request(void *buffer, size_t size, bool block) {
    int rs;
    uintptr_t buf_addr = ((uintptr_t)buffer);
    cd_transfer_params_t params;
    struct cmd_transfer_data data;

    if(cmd_hnd <= 0) {
        return ERR_NO_ACTIVE;
    }
    if(dma_in_progress) {
        dbglog(DBG_ERROR, "cdrom_stream_request: Previous DMA request is in progress.\n");
        return ERR_SYS;
    }

    if(stream_dma) {
        if(!__builtin_is_aligned(buf_addr, 32)) {
            dbglog(DBG_ERROR, "cdrom_stream_request: Unaligned memory for DMA (32-byte).\n");
            return ERR_SYS;
        }
        /* Use the physical memory address. */
        params.addr = (void *)(buf_addr & MEM_AREA_CACHE_MASK);

        /* Invalidate the CPU cache only for cacheable memory areas.
           Otherwise, it is assumed that either this operation is unnecessary
           (another DMA is being used) or that the caller is responsible
           for managing the CPU data cache.
        */
        if((buf_addr & MEM_AREA_P2_BASE) != MEM_AREA_P2_BASE) {
            /* Invalidate the dcache over the range of the data. */
            dcache_inval_range(buf_addr, size);
        }
    }
    else {
        params.addr = buffer;

        if(!__builtin_is_aligned(buf_addr, 2)) {
            dbglog(DBG_ERROR, "cdrom_stream_request: Unaligned memory for PIO (2-byte).\n");
            return ERR_SYS;
        }
    }

    params.size = size;
    sem_wait_scoped(&_g1_ata_sem);

    if(stream_dma) {
        dma_in_progress = true;
        dma_blocking = block;
        dma_auto_unlock = !block;

        rs = syscall_gdrom_dma_transfer(cmd_hnd, &params);

        if(rs < 0) {
            dma_in_progress = false;
            dma_blocking = false;
            dma_auto_unlock = false;
            return ERR_SYS;
        }
        if(!block) {
            return ERR_OK;
        }
        sem_wait(&dma_done);
    }
    else {
        rs = syscall_gdrom_pio_transfer(cmd_hnd, &params);
        if(rs < 0)
            return ERR_SYS;
    }

    data = (struct cmd_transfer_data){ cmd_hnd, 0 };

    if(cdrom_poll(&data, 0, cdrom_check_transfer) == ERR_NO_ACTIVE) {
        cmd_hnd = 0;
    }
    else if(!stream_dma) {
        /* Syscalls doesn't call it on last reading in PIO mode.
           Looks like a bug, fixing it. */
        if(data.size == 0 && stream_cb)
            stream_cb(stream_cb_param);
    }

    return ERR_OK;
}

int cdrom_stream_progress(size_t *size) {
    int rv = 0;
    size_t check_size = 0;

    if(cmd_hnd <= 0) {
        if(size) {
            *size = check_size;
        }
        return rv;
    }

    if(stream_dma) {
        rv = syscall_gdrom_dma_check(cmd_hnd, &check_size);
    }
    else {
        rv = syscall_gdrom_pio_check(cmd_hnd, &check_size);
    }

    if(size) {
        *size = check_size;
    }
    return rv;
}

void cdrom_stream_set_callback(cdrom_stream_callback_t callback, void *param) {
    stream_cb = callback;
    stream_cb_param = param;

    if(!stream_dma) {
        syscall_gdrom_pio_callback((uintptr_t)stream_cb, stream_cb_param);
    }
}

/* Read a piece of or all of the Q byte of the subcode of the last sector read.
   If you need the subcode from every sector, you cannot read more than one at 
   a time. */
/* XXX: Use some CD-Gs and other stuff to test if you get more than just the 
   Q byte */
int cdrom_get_subcode(void *buffer, size_t buflen, cd_sub_type_t which) {
    cd_cmd_getscd_params_t params = { .which = which, .buflen = buflen, .buffer = buffer };
    return cdrom_exec_cmd(CD_CMD_GETSCD, &params);
}

/* Locate the LBA sector of the data track; use after reading TOC */
uint32_t cdrom_locate_data_track(cd_toc_t *toc) {
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
int cdrom_cdda_play(uint32_t start, uint32_t end, uint32_t repeat, int mode) {
    cd_cmd_play_params_t params;

    /* Limit to 0-15 */
    if(repeat > 15)
        repeat = 15;

    params.start = start;
    params.end = end;
    params.repeat = repeat;

    if(mode == CDDA_TRACKS)
        return cdrom_exec_cmd(CD_CMD_PLAY_TRACKS, &params);
    else if(mode == CDDA_SECTORS)
        return cdrom_exec_cmd(CD_CMD_PLAY_SECTORS, &params);
    else
        return ERR_OK;
}

/* Pause CDDA audio playback */
int cdrom_cdda_pause(void) {
    return cdrom_exec_cmd(CD_CMD_PAUSE, NULL);
}

/* Resume CDDA audio playback */
int cdrom_cdda_resume(void) {
    return cdrom_exec_cmd(CD_CMD_RELEASE, NULL);
}

/* Spin down the CD */
int cdrom_spin_down(void) {
    return cdrom_exec_cmd(CD_CMD_STOP, NULL);
}

static void cdrom_vblank(uint32_t evt, void *data) {
    (void)evt;
    (void)data;

    if(dma_in_progress) {
        syscall_gdrom_exec_server();
        cmd_response = syscall_gdrom_check_command(cmd_hnd, &cmd_status);

        if(cmd_response != PROCESSING && cmd_response != BUSY && cmd_response != STREAMING) {
            dma_in_progress = false;

            if(dma_blocking) {
                dma_blocking = false;
                sem_signal(&dma_done);
                thd_schedule(true);
            }
        }
    }
}

static void g1_dma_irq_hnd(uint32_t code, void *data) {
    (void)data;

    if(dma_in_progress) {
        dma_in_progress = false;

        syscall_gdrom_exec_server();
        cmd_response = syscall_gdrom_check_command(cmd_hnd, &cmd_status);

        if(dma_blocking) {
            dma_blocking = false;
            sem_signal(&dma_done);
            thd_schedule(true);
        }
        else if(dma_auto_unlock) {
            sem_signal(&_g1_ata_sem);
            dma_auto_unlock = false;
        }
        if(stream_enabled) {
            syscall_gdrom_dma_callback((uintptr_t)stream_cb, stream_cb_param);
        }
    }
    else if(old_dma_irq.hdl) {
        old_dma_irq.hdl(code, old_dma_irq.data);
    }
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
        flush_size = (patch_addr[1] - patch_addr[0]) + CACHE_L1_ICACHE_LINESIZE;
        flush_size &= ~(CACHE_L1_ICACHE_LINESIZE - 1);
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

    if(inited) {
        return;
    }

    sem_wait(&_g1_ata_sem);

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
    sem_signal(&_g1_ata_sem);

    /* Hook all the DMA related events. */
    old_dma_irq = asic_evt_set_handler(ASIC_EVT_GD_DMA, g1_dma_irq_hnd, NULL);
    asic_evt_set_handler(ASIC_EVT_GD_DMA_OVERRUN, g1_dma_irq_hnd, NULL);
    asic_evt_set_handler(ASIC_EVT_GD_DMA_ILLADDR, g1_dma_irq_hnd, NULL);

    if(old_dma_irq.hdl == NULL) {
        asic_evt_enable(ASIC_EVT_GD_DMA, ASIC_IRQB);
        asic_evt_enable(ASIC_EVT_GD_DMA_OVERRUN, ASIC_IRQB);
        asic_evt_enable(ASIC_EVT_GD_DMA_ILLADDR, ASIC_IRQB);
    }

    vblank_hnd = vblank_handler_add(cdrom_vblank, NULL);
    inited = true;

    cdrom_reinit();
}

void cdrom_shutdown(void) {

    if(!inited) {
        return;
    }

    vblank_handler_remove(vblank_hnd);

    /* Unhook the events and disable the IRQs. */
    if(old_dma_irq.hdl) {
        /* G1-ATA driver uses the same handler for 3 events. */
        asic_evt_set_handler(ASIC_EVT_GD_DMA,
            old_dma_irq.hdl, old_dma_irq.data);
        asic_evt_set_handler(ASIC_EVT_GD_DMA_OVERRUN,
            old_dma_irq.hdl, old_dma_irq.data);
        asic_evt_set_handler(ASIC_EVT_GD_DMA_ILLADDR,
            old_dma_irq.hdl, old_dma_irq.data);

        old_dma_irq.hdl = NULL;
    }
    else {
        asic_evt_disable(ASIC_EVT_GD_DMA, ASIC_IRQB);
        asic_evt_remove_handler(ASIC_EVT_GD_DMA);
        asic_evt_disable(ASIC_EVT_GD_DMA_OVERRUN, ASIC_IRQB);
        asic_evt_remove_handler(ASIC_EVT_GD_DMA_OVERRUN);
        asic_evt_disable(ASIC_EVT_GD_DMA_ILLADDR, ASIC_IRQB);
        asic_evt_remove_handler(ASIC_EVT_GD_DMA_ILLADDR);
    }
    inited = false;
}
