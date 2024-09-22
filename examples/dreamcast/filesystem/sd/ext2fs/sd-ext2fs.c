/* KallistiOS ##version##

   sd-ext2fs.c
   Copyright (C) 2012 Lawrence Sebald

   This example shows the basics of how to interact with the SD card adapter
   driver and to make it work with fs_ext2 (in libkosext2fs).

   About all this example program does is to mount the SD card (if one is found
   and it is detected to be ext2fs) on /sd of the VFS and print a directory
   listing of the root directory of the SD card.

   In addition, if you add -DENABLE_WRITE to the CFLAGS while compiling, this
   program will write a text file out on the root of the SD card.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include <dc/sd.h>
#include <kos/blockdev.h>
#include <ext2/fs_ext2.h>

/* Add -DENABLE_WRITE to the KOS_CFLAGS in the Makefile to enable writing a
   small file out on the SD card. */
#ifdef ENABLE_WRITE
#define MNT_MODE FS_EXT2_MOUNT_READWRITE
#else
#define MNT_MODE FS_EXT2_MOUNT_READONLY
#endif

int main(int argc, char *argv[]) {
    kos_blockdev_t sd_dev;
    uint8 partition_type;
    DIR *d;
    struct dirent *entry;
#ifdef ENABLE_WRITE
    FILE *fp;
#endif

    if(sd_init()) {
        printf("Could not initialize the SD card. Please make sure that you "
               "have an SD card adapter plugged in and an SD card inserted.\n");
        exit(EXIT_FAILURE);
    }

    /* Grab the block device for the first partition on the SD card. Note that
       you must have the SD card formatted with an MBR partitioning scheme. */
    if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)) {
        printf("Could not find the first partition on the SD card!\n");
        exit(EXIT_FAILURE);
    }

    /* Check to see if the MBR says that we have a Linux partition. */
    if(partition_type != 0x83) {
        printf("MBR indicates a non-ext2 filesystem. Will try to mount "
               "anyway\n");
    }

    /* Initialize fs_ext2 and attempt to mount the device. */
    if(fs_ext2_init()) {
        printf("Could not initialize fs_ext2!\n");
        exit(EXIT_FAILURE);
    }

    if(fs_ext2_mount("/sd", &sd_dev, MNT_MODE)) {
        printf("Could not mount SD card as ext2fs. Please make sure the card "
               "has been properly formatted.\n");
        exit(EXIT_FAILURE);
    }

    printf("Listing the contents of /sd:\n");

    /* Open the root of the SD card's filesystem and list the contents. */
    if(!(d = opendir("/sd"))) {
        printf("Could not open /sd: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while((entry = readdir(d))) {
        printf("%s\n", entry->d_name);
    }

    if(closedir(d)) {
        printf("Could not close directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

#ifdef ENABLE_WRITE
    /* Create a new file on the root of the SD card. */
    if(!(fp = fopen("/sd/hello.txt", "w"))) {
        printf("Could not create file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(fprintf(fp, "Hello World!\nTesting, testing, 1, 2, 3...") < 0) {
        printf("Failed to write to file: %s\n", strerror(errno));
    }
    else {
        printf("Wrote to file \"hello.txt\"\n");
    }

    fclose(fp);
#endif

    /* Clean up the filesystem and everything else */
    fs_ext2_unmount("/sd");
    fs_ext2_shutdown();
    sd_shutdown();

    return 0;
}
