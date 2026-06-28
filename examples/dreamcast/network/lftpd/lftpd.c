/* KallistiOS ##version##

   lftpd.c
   Copyright (C) 2026 BruceLeet
   Copyright (C) 2026 Paul Cercueil
   Copyright (C) 2026 Andy Barajas
*/

/*
   FTP server example for the Dreamcast, built on the lftpd kos-port. It brings
   up KOS networking, mounts any attached IDE/SD storage as FAT, and serves the
   VFS root over FTP so files can be moved to and from the Dreamcast with a
   standard FTP client. Press START on the controller to exit.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <kos.h>
#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/g1ata.h>
#include <dc/sd.h>
#include <fat/fs_fat.h>

#include <lftpd/lftpd.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

#define FTP_PORT      21
#define DHCP_WAIT_MS  5000

/* fs_fat_mount() keeps the pointer to these (it does not copy them), so they
   must outlive the mount -- hence file scope, not stack locals. */
static kos_blockdev_t ide_bd;
static kos_blockdev_t sd_bd;

/* Press START to exit; flush any cached FAT writes first. */
static void clean_exit(void) {
    fs_fat_sync("/sd");
    fs_fat_sync("/ide");
    exit(0);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    uint32_t y_pos = 20;

    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)clean_exit);

    vid_clear(23, 86, 155);
    bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "KOS FTP Server (lftpd)");
    y_pos += 24;

    printf("===========================\n");
    printf("KOS FTP Server (lftpd)\n");
    printf("===========================\n\n");

    fs_fat_init();

    /* Mount IDE */
    printf("Initializing ATA...\n");
    if(g1_ata_init() < 0) {
        printf("  g1_ata_init failed\n");
    }
    else {
        uint8_t pt;

        if(g1_ata_blockdev_for_partition(0, 1, &ide_bd, &pt) < 0) {
            printf("  ATA partition not found\n");
        }
        else if(fs_fat_mount("/ide", &ide_bd, FS_FAT_MOUNT_READWRITE) < 0) {
            printf("  IDE mount failed\n");
        }
        else {
            printf("  Mounted /ide (type 0x%02x)\n\n", pt);
            bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "IDE MOUNTED");
            y_pos += 24;
        }
    }

    /* Mount SD */
    printf("Initializing SD...\n");
    if(sd_init() < 0) {
        printf("  sd_init failed\n");
    }
    else {
        uint8_t pt;

        if(sd_blockdev_for_partition(0, &sd_bd, &pt) < 0) {
            printf("  SD partition not found\n");
        }
        else if(fs_fat_mount("/sd", &sd_bd, FS_FAT_MOUNT_READWRITE) < 0) {
            printf("  SD mount failed\n");
        }
        else {
            printf("  Mounted /sd (type 0x%02x)\n\n", pt);
            bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "SD MOUNTED");
            y_pos += 24;
        }
    }

    /* Wait for DHCP / network to come up */
    printf("Waiting for network (%d ms)...\n", DHCP_WAIT_MS);
    thd_sleep(DHCP_WAIT_MS);

    uint8_t *ip = net_default_dev->ip_addr;
    printf("FTP: %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], FTP_PORT);
    bfont_draw_str_fmt(vram_s + y_pos * 640 + 20, 640, true,
                       "FTP: %d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], FTP_PORT);
    y_pos += 24;
    bfont_draw_str(vram_s + y_pos * 640 + 20, 640, true, "Press START to exit");

    /* Serve the VFS root over FTP -- blocks forever. */
    lftpd_t lftpd;
    lftpd_start("/", FTP_PORT, &lftpd);

    return 0;
}
