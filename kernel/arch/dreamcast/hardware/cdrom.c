/* KallistiOS ##version##

   cdrom.c

   Copyright (C) 2000 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2014 Donald Haase

 */

#include <dc/cdrom.h>
#include <dc/g1ata.h>

#include <kos/thread.h>
#include <kos/mutex.h>

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
command queuing. Every call to gdc_req_cmd returns a 'request id' which
just needs to eventually be checked by cmd_stat. A non-blocking version
of all functions would simply require manual calls to check the status.
Doing this would probably allow data reading while cdda is playing without
hiccups (by severely reducing the number of gd commands being sent).
*/


/* GD-Rom BIOS calls... named mostly after Marcus' code. None have more
   than two parameters; R7 (fourth parameter) needs to describe
   which syscall we want. */

#define MAKE_SYSCALL(rs, p1, p2, idx) \
    uint32 *syscall_bc = (uint32*)0x8c0000bc; \
    int (*syscall)() = (int (*)())(*syscall_bc); \
    rs syscall((p1), (p2), 0, (idx));

/* Reset system functions */
static void gdc_init_system() {
    MAKE_SYSCALL(/**/, 0, 0, 3);
}

/* Submit a command to the system */
static int gdc_req_cmd(int cmd, void *param) {
    MAKE_SYSCALL(return, cmd, param, 0);
}

/* Check status on an executed command */
static int gdc_get_cmd_stat(int f, void *status) {
    MAKE_SYSCALL(return, f, status, 1);
}

/* Execute submitted commands */
static void gdc_exec_server() {
    MAKE_SYSCALL(/**/, 0, 0, 2);
}

/* Check drive status and get disc type */
static int gdc_get_drv_stat(void *param) {
    MAKE_SYSCALL(return, param, 0, 4);
}

/* Set disc access mode */
static int gdc_change_data_type(void *param) {
    MAKE_SYSCALL(return, param, 0, 10);
}

/* Reset the GD-ROM */
/* Stop gcc from complaining that we don't use it */
static void gdc_reset() __attribute__((unused));
static void gdc_reset() {
    MAKE_SYSCALL(/**/, 0, 0, 9);
}

/* Abort the current command */
static void gdc_abort_cmd(int cmd) {
    MAKE_SYSCALL(/**/, cmd, 0, 8);
}

/* The G1 ATA access mutex */
mutex_t _g1_ata_mutex = RECURSIVE_MUTEX_INITIALIZER;

/* Shortcut to cdrom_reinit_ex. Typically this is the only thing changed. */
int cdrom_set_sector_size(int size) {
    return cdrom_reinit_ex(-1, -1, size);
}

/* Command execution sequence */
/* XXX: It might make sense to have a version of this that takes a timeout. */
int cdrom_exec_cmd(int cmd, void *param) {
    int status[4] = {0};
    int f, n;
    
    mutex_lock(&_g1_ata_mutex);

    /* Make sure to select the GD-ROM drive. */
    g1_ata_select_device(G1_ATA_MASTER);

    /* Submit the command and wait for it to finish */
    f = gdc_req_cmd(cmd, param);

    do {
        gdc_exec_server();
        n = gdc_get_cmd_stat(f, status);

        if(n == PROCESSING)
            thd_pass();
    }
    while(n == PROCESSING);
    
    mutex_unlock(&_g1_ata_mutex);

    if(n == COMPLETED)
        return ERR_OK;
    else if(n == ABORTED)
        return ERR_ABORTED;
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
    }
}

/* Return the status of the drive as two integers (see constants) */
int cdrom_get_status(int *status, int *disc_type) {
    int     rv = ERR_OK;
    uint32  params[2];

    /* We might be called in an interrupt to check for ISO cache
       flushing, so make sure we're not interrupting something
       already in progress. */
    if(irq_inside_int()) {
        if(mutex_trylock(&_g1_ata_mutex))
            /* DH: Figure out a better return to signal error */
            return -1;
    }
    else {
        mutex_lock(&_g1_ata_mutex);
    }

    /* Make sure to select the GD-ROM drive. */
    g1_ata_select_device(G1_ATA_MASTER);

    rv = gdc_get_drv_stat(params);
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
    int rv = ERR_OK;
    uint32  params[4];

    mutex_lock(&_g1_ata_mutex);
    g1_ata_select_device(G1_ATA_MASTER);

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
            gdc_get_drv_stat(params);
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
    rv = gdc_change_data_type(params);
    mutex_unlock(&_g1_ata_mutex);
    return rv;
}

/* Re-init the drive, e.g., after a disc change, etc */
int cdrom_reinit() {
    /* By setting -1 to each parameter, they fall to the old defaults */
    return cdrom_reinit_ex(-1, -1, -1);
}

/* Enhanced cdrom_reinit, takes the place of the old 'sector_size' function */
int cdrom_reinit_ex(int sector_part, int cdxa, int sector_size) {
    int r = -1;
    int timeout;

    mutex_lock(&_g1_ata_mutex);

    /* Try a few times; it might be busy. If it's still busy
       after this loop then it's probably really dead. */
    timeout = 10 * 1000 / 20; /* 10 second timeout */

    /* Make sure to select the GD-ROM drive. */
    g1_ata_select_device(G1_ATA_MASTER);

    while(timeout > 0) {
        r = cdrom_exec_cmd(CMD_INIT, NULL);

        if(r == 0) break;

        if(r == ERR_NO_DISC) {
            mutex_unlock(&_g1_ata_mutex);
            return r;
        }
        else if(r == ERR_SYS) {
            mutex_unlock(&_g1_ata_mutex);
            return r;
        }

        /* Still trying.. sleep a bit and check again */
        thd_sleep(20);
        timeout--;
    }

    if(timeout <= 0) {
        /* Send an abort since we're giving up waiting for the init */
        gdc_abort_cmd(CMD_INIT);
        mutex_unlock(&_g1_ata_mutex);
        return r;
    }

    r = cdrom_change_datatype(sector_part, cdxa, sector_size);
    mutex_unlock(&_g1_ata_mutex);
    
    return r;
}

/* Read the table of contents */
int cdrom_read_toc(CDROM_TOC *toc_buffer, int session) {
    struct {
        int session;
        void    *buffer;
    } params;
    int rv;

    params.session = session;
    params.buffer = toc_buffer;

    mutex_lock(&_g1_ata_mutex);

    rv = cdrom_exec_cmd(CMD_GETTOC2, &params);
    mutex_unlock(&_g1_ata_mutex);

    return rv;
}

/* Enhanced Sector reading: Choose mode to read in. */
int cdrom_read_sectors_ex(void *buffer, int sector, int cnt, int mode) {
    struct {
        int sec, num;
        void    *buffer;
        int dunno;
    } params;
    int rv = ERR_OK;

    params.sec = sector;    /* Starting sector */
    params.num = cnt;       /* Number of sectors */
    params.buffer = buffer; /* Output buffer */
    params.dunno = 0;       /* ? */

    mutex_lock(&_g1_ata_mutex);

    /* The DMA mode blocks the thread it is called in by the way we execute
       gd syscalls. It does however allow for other threads to run. */
    /* XXX: DMA Mode may conflict with using a second G1ATA device. More 
       testing is needed from someone with such a device.
    */
    if(mode == CDROM_READ_DMA)
        rv = cdrom_exec_cmd(CMD_DMAREAD, &params);
    else if (mode == CDROM_READ_PIO)
        rv = cdrom_exec_cmd(CMD_PIOREAD, &params);

    mutex_unlock(&_g1_ata_mutex);
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
        void    *buffer;
    } params;
    int rv;

    params.which = which;
    params.buflen = buflen;
    params.buffer = buffer;
    mutex_lock(&_g1_ata_mutex);
    rv = cdrom_exec_cmd(CMD_GETSCD, &params);
    mutex_unlock(&_g1_ata_mutex);
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

    mutex_lock(&_g1_ata_mutex);

    if(mode == CDDA_TRACKS)
        rv = cdrom_exec_cmd(CMD_PLAY, &params);
    else if(mode == CDDA_SECTORS)
        rv = cdrom_exec_cmd(CMD_PLAY2, &params);

    mutex_unlock(&_g1_ata_mutex);

    return rv;
}

/* Pause CDDA audio playback */
int cdrom_cdda_pause() {
    int rv;

    mutex_lock(&_g1_ata_mutex);
    rv = cdrom_exec_cmd(CMD_PAUSE, NULL);
    mutex_unlock(&_g1_ata_mutex);

    return rv;
}

/* Resume CDDA audio playback */
int cdrom_cdda_resume() {
    int rv;

    mutex_lock(&_g1_ata_mutex);
    rv = cdrom_exec_cmd(CMD_RELEASE, NULL);
    mutex_unlock(&_g1_ata_mutex);

    return rv;
}

/* Spin down the CD */
int cdrom_spin_down() {
    int rv;

    mutex_lock(&_g1_ata_mutex);
    rv = cdrom_exec_cmd(CMD_STOP, NULL);
    mutex_unlock(&_g1_ata_mutex);

    return rv;
}

/* Initialize: assume no threading issues */
int cdrom_init() {
    uint32 p;
    volatile uint32 *react = (uint32 *)0xa05f74e4,
                     *bios = (uint32 *)0xa0000000;

    /* Reactivate drive: send the BIOS size and then read each
       word across the bus so the controller can verify it.
       If first bytes are 0xe6ff instead of usual 0xe3ff, then
       hardware is fitted with custom BIOS using magic bootstrap
       which can and must pass controller verification with only
       the first 1024 bytes */
    if((*(uint16 *)0xa0000000) == 0xe6ff) {
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

    mutex_lock(&_g1_ata_mutex);
    /* Make sure to select the GD-ROM drive. */
    g1_ata_select_device(G1_ATA_MASTER);

    /* Reset system functions */
    gdc_init_system();
    mutex_unlock(&_g1_ata_mutex);

    /* Do an initial initialization */
    cdrom_reinit();

    return 0;
}

void cdrom_shutdown() {

    /* What would you want done here? */
}
