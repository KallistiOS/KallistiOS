/* KallistiOS ##version##

   init.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2015 Lawrence Sebald
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <kos/dbgio.h>
#include <kos/init.h>
#include <kos/platform.h>
#include <arch/arch.h>
#include <arch/irq.h>
#include <arch/memory.h>
#include <arch/rtc.h>
#include <arch/timer.h>
#include <arch/wdt.h>
#include <dc/perfctr.h>
#include <dc/ubc.h>
#include <dc/pvr.h>
#include <dc/vmufs.h>
#include <dc/syscalls.h>
#include <dc/dmac.h>

#include "initall_hdrs.h"

extern int _bss_start, end;

/* ctor/dtor stuff from libgcc. */
#if __GNUC__ == 4
#define _init init
#define _fini fini
#endif

extern void _init(void);
extern void _fini(void);
extern void __verify_newlib_patch();

void (*__kos_init_early_fn)(void) __attribute__((weak,section(".data"))) = NULL;

int main(int argc, char **argv);
uint32 _fs_dclsocket_get_ip(void);

/* We have to put this here so we can include plat-specific devices */
dbgio_handler_t * dbgio_handlers[] = {
    &dbgio_dcload,
    &dbgio_dcls,
    &dbgio_scif,
    &dbgio_null,
    &dbgio_fb
};
int dbgio_handler_cnt = sizeof(dbgio_handlers) / sizeof(dbgio_handler_t *);

void arch_init_net_dcload_ip(void) {
    union {
        uint32 ipl;
        uint8 ipb[4];
    } ip = { 0 };

    if(dcload_type == DCLOAD_TYPE_IP) {
        /* Grab the IP address from dcload before we disable dbgio... */
        ip.ipl = _fs_dclsocket_get_ip();
        dbglog(DBG_INFO, "dc-load says our IP is %d.%d.%d.%d\n", ip.ipb[3],
               ip.ipb[2], ip.ipb[1], ip.ipb[0]);
        dbgio_disable();
    }

    net_init(ip.ipl);     /* Enable networking (and drivers) */

    if(dcload_type == DCLOAD_TYPE_IP) {
        fs_dclsocket_init_console();

        if(!fs_dclsocket_init()) {
            dbgio_dev_select("fs_dclsocket");
            dbgio_enable();
            dbglog(DBG_INFO, "fs_dclsocket console support enabled\n");
        }
    }
}

void arch_init_net_no_dcload(void) {
    net_init(0);
}

KOS_INIT_FLAG_WEAK(arch_init_net_dcload_ip, true);
KOS_INIT_FLAG_WEAK(arch_init_net_no_dcload, false);

void arch_init_net(void) {
    KOS_INIT_FLAG_CALL(arch_init_net_dcload_ip);
    KOS_INIT_FLAG_CALL(arch_init_net_no_dcload);
}

void vmu_fs_init(void) {
    fs_vmu_init();
    vmufs_init();
}

void vmu_fs_shutdown(void) {
    fs_vmu_shutdown();
    vmufs_shutdown();
}

/* Mount the built-in romdisk to /rd. */
void fs_romdisk_mount_builtin(void) {
    fs_romdisk_mount("/rd", __kos_romdisk, 0);
}

void fs_romdisk_mount_builtin_legacy(void) {
    fs_romdisk_mount_builtin();
}

KOS_INIT_FLAG_WEAK(arch_init_net, false);
KOS_INIT_FLAG_WEAK(net_shutdown, false);
KOS_INIT_FLAG_WEAK(maple_wait_scan, true);
KOS_INIT_FLAG_WEAK(fs_romdisk_init, true);
KOS_INIT_FLAG_WEAK(fs_romdisk_shutdown, true);
KOS_INIT_FLAG_WEAK(fs_romdisk_mount_builtin, false);
KOS_INIT_FLAG_WEAK(fs_romdisk_mount_builtin_legacy, false);
KOS_INIT_FLAG_WEAK(vmu_fs_init, true);
KOS_INIT_FLAG_WEAK(vmu_fs_shutdown, true);
KOS_INIT_FLAG_WEAK(fs_iso9660_init, true);
KOS_INIT_FLAG_WEAK(fs_iso9660_shutdown, true);

void dcload_init(void) {
    if (*DCLOADMAGICADDR == DCLOADMAGICVALUE) {
        dbglog(DBG_INFO, "dc-load console support enabled\n");
        fs_dcload_init();
    }
}

KOS_INIT_FLAG_WEAK(dcload_init, true);
KOS_INIT_FLAG_WEAK(fs_dcload_init_console, true);
KOS_INIT_FLAG_WEAK(fs_dcload_shutdown, true);
KOS_INIT_FLAG_WEAK(fs_dclsocket_shutdown, true);

/* Auto-init stuff: override with a non-weak symbol if you don't want all of
   this to be linked into your code (and do the same with the
   arch_auto_shutdown function too). */
int  __weak arch_auto_init(void) {
    /* Initialize memory management */
    mm_init();

    /* Do this immediately so we can receive exceptions for init code
       and use ints for dbgio receive. */
    irq_init();         /* IRQs */
    irq_disable();      /* Turn on exceptions */

    ubc_init();

    /* Init dc-load console, if applicable */
    KOS_INIT_FLAG_CALL(fs_dcload_init_console);

    /* Init SCIF for debug stuff (maybe) */
    scif_init();

    /* Init debug IO */
    dbgio_init();

    /* Print a banner */
    if(__kos_init_flags & INIT_QUIET)
        dbgio_disable();
    else {
        /* PTYs not initialized yet */
        dbgio_write_str("\n--\n");
        dbgio_write_str(kos_get_banner());
    }

    timer_init();           /* Timers */
    hardware_sys_init();        /* DC low-level hardware init */

    syscall_sysinfo_init();

    /* Initialize our timer */
    perf_cntr_timer_enable();
    timer_ms_enable();
    rtc_init();

    thd_init();

    nmmgr_init();

    fs_init();          /* VFS */
    fs_dev_init();
    fs_null_init();
    fs_pty_init();          /* Pty */
    fs_ramdisk_init();      /* Ramdisk */
    KOS_INIT_FLAG_CALL(fs_romdisk_init);    /* Romdisk */

/* The arc4random_buf() function used for random & urandom is only
   available in newlib starting with version 2.4.0 */
#if defined(__NEWLIB__) && !(__NEWLIB__ < 2 && __NEWLIB_MINOR__ < 4)
    fs_rnd_init();          /* /dev/urandom etc. */
#else
#warning "/dev filesystem is not supported with Newlib < 2.4.0"
#endif

    hardware_periph_init();     /* DC peripheral init */

    if(!KOS_INIT_FLAG_CALL(fs_romdisk_mount_builtin))
        KOS_INIT_FLAG_CALL(fs_romdisk_mount_builtin_legacy);

    KOS_INIT_FLAG_CALL(dcload_init);

    if (!KOS_PLATFORM_IS_NAOMI)
        KOS_INIT_FLAG_CALL(fs_iso9660_init);

    KOS_INIT_FLAG_CALL(vmu_fs_init);

    /* Initialize library handling */
    library_init();

    /* Now comes the optional stuff */
    if(__kos_init_flags & INIT_IRQ) {
        irq_enable();       /* Turn on IRQs */
        KOS_INIT_FLAG_CALL(maple_wait_scan);  /* Wait for the maple scan to complete */
    }

    if (!KOS_PLATFORM_IS_NAOMI)
        KOS_INIT_FLAG_CALL(arch_init_net);

    return 0;
}

void  __weak arch_auto_shutdown(void) {
    KOS_INIT_FLAG_CALL(fs_dclsocket_shutdown);
    if (!KOS_PLATFORM_IS_NAOMI)
        KOS_INIT_FLAG_CALL(net_shutdown);

    snd_shutdown();
    hardware_shutdown();
    /* XXX: We should investigate shrinking this irq_disabled
       time. Until then, all these shut downs happen with
       irqs disabled which prevents things like safely joining
       threads or sending cleanup commands to hardware.
    */
    irq_disable();
    timer_shutdown();
    pvr_shutdown();
    library_shutdown();
    KOS_INIT_FLAG_CALL(fs_dcload_shutdown);
    KOS_INIT_FLAG_CALL(vmu_fs_shutdown);
    if (!KOS_PLATFORM_IS_NAOMI)
        KOS_INIT_FLAG_CALL(fs_iso9660_shutdown);
#if defined(__NEWLIB__) && !(__NEWLIB__ < 2 && __NEWLIB_MINOR__ < 4)
    fs_rnd_shutdown();
#endif
    fs_shutdown();
    fs_ramdisk_shutdown();
    KOS_INIT_FLAG_CALL(fs_romdisk_shutdown);
    fs_pty_shutdown();
    fs_null_shutdown();
    fs_dev_shutdown();
    thd_shutdown();
    rtc_shutdown();
}

/* Strongly defined in gcrt1.s and linked when compiling with -pg */
void __weak gprof_init(void) { }

/* This is the entry point inside the C program */
void arch_main(void) {
    uint8 *bss_start = (uint8 *)(&_bss_start);
    uint8 *bss_end = (uint8 *)(&end);
    int rv;

    if (KOS_PLATFORM_IS_NAOMI) {
        /* Ugh. I'm really not sure why we have to set up these DMA registers this
           way on boot, but failing to do so breaks maple... */
        DMAC_SAR2 = 0;
        DMAC_CHCR2 = 0x1201;
        DMAC_DMAOR = 0x8201;
    }

    /* Ensure the WDT is not enabled from a previous session */
    wdt_disable();

    /* Ensure that UBC is not enabled from a previous session */
    ubc_shutdown();

    /* Handle optional callback provided by KOS_INIT_EARLY() */
    if(__kos_init_early_fn)
        __kos_init_early_fn();

    /* Clear out the BSS area */
    memset(bss_start, 0, bss_end - bss_start);

    /* Do auto-init stuff */
    arch_auto_init();

    __verify_newlib_patch();

    dbglog(DBG_INFO, "\n");

    /* Run ctors */
    _init();

    /* Setup gprof */
    gprof_init();

    /* Call the user's main function */
    rv = main(0, NULL);

    /* Call kernel exit */
    exit(rv);
}

/* Set the exit path (default is RETURN) */
int arch_exit_path = ARCH_EXIT_RETURN;
void arch_set_exit_path(int path) {
    assert(path >= ARCH_EXIT_RETURN && path <= ARCH_EXIT_REBOOT);
    arch_exit_path = path;
}

/* Does the actual shutdown stuff for a proper shutdown */
void arch_shutdown(void) {
    /* Run dtors */
    _fini();

    dbglog(DBG_CRITICAL, "arch: shutting down kernel\n");

    /* Disable the WDT, if active */
    wdt_disable();

    /* Turn off UBC breakpoints, if any */
    ubc_shutdown();

    /* Do auto-shutdown... or use the "light weight" version underneath */
#if 1
    arch_auto_shutdown();
#else
    /* Ensure that interrupts are disabled */
    irq_disable();

    /* Make sure that PVR and Maple are shut down */
    pvr_shutdown();
    maple_shutdown();

    /* Shut down any other hardware things */
    hardware_shutdown();
#endif

    if(__kos_init_flags & INIT_MALLOCSTATS) {
        malloc_stats();
    }

    /* Shut down IRQs */
    irq_shutdown();
}

/* Generic kernel exit point */
void arch_exit(void) {
    /* arch_exit always returns EXIT_SUCCESS (0)
       if return codes are desired then a call to
       newlib's exit() should be used in its place */
    exit(EXIT_SUCCESS);
}

/* Return point from newlib's _exit() (configurable) */
void arch_exit_handler(int ret_code) {
    dbglog(DBG_INFO, "\narch: exit return code %d\n", ret_code);

    /* Shut down */
    arch_shutdown();

    switch(arch_exit_path) {
        default:
            dbglog(DBG_CRITICAL, "arch: arch_exit_path has invalid value!\n");
            __fallthrough;
        case ARCH_EXIT_RETURN:
            arch_return(ret_code);
            break;
        case ARCH_EXIT_MENU:
            arch_menu();
            break;
        case ARCH_EXIT_REBOOT:
            arch_reboot();
            break;
    }
}

/* Called to shut down the system and return to the debug handler (if any) */
void arch_return(int ret_code) {
    /* Jump back to the boot loader */
    arch_real_exit(ret_code);
}

/* Called to jump back to the BIOS menu; assumes a normal shutdown is possible */
void arch_menu(void) {
    dbglog(DBG_CRITICAL, "arch: exiting the system to the BIOS menu\n");
    syscall_system_bios_menu();
}

/* Called to shut down non-gracefully; assume the system is in peril
   and don't try to call the dtors */
__used __noreturn
void arch_abort(void) {
    /* Disable the WDT, if active */
    wdt_disable();

    /* Turn off UBC breakpoints, if any */
    ubc_shutdown();

    dbglog(DBG_CRITICAL, "arch: aborting the system\n");

    /* PVR disable-by-fire */
    PVR_SET(PVR_RESET, PVR_RESET_ALL);
    PVR_SET(PVR_RESET, PVR_RESET_NONE);

    /* Maple disable-by-fire */
    maple_dma_stop();

    /* Sound disable (nothing weird done in here) */
    spu_disable();

    /* Turn off any IRQs */
    irq_disable();

    arch_real_exit(EXIT_FAILURE);
}

/* Called to reboot the system; assume the system is in peril and don't
   try to call the dtors */
void arch_reboot(void) {
    typedef void (*reboot_func)() __noreturn;
    reboot_func rb;

    dbglog(DBG_CRITICAL, "arch: rebooting the system\n");

    /* Ensure that interrupts are disabled */
    irq_disable();

    /* Reboot */
    rb = (reboot_func)(MEM_AREA_P2_BASE | 0x00000000);
    rb();
}
