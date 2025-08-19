/* KallistiOS ##version##

   dreameye-sd.c
   Copyright (C) 2013 Lawrence Sebald

   This example simply dumps all the images on the first connected Dreameye to
   the SD card. It creates a new directory and saves the images in it.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include <dc/sd.h>
#include <kos/blockdev.h>
#include <ext2/fs_ext2.h>
#include <dc/maple.h>
#include <dc/maple/dreameye.h>
#include <kos/dbgio.h>

int main(int argc, char *argv[]) {
    maple_device_t *dreameye;
    dreameye_state_t *state;
    uint8_t *buf = NULL;
    int size, err;
    FILE *fp;
    int img_count, i;
    char fn[64];
    kos_blockdev_t sd_dev;
    uint8_t partition_type;

    /* We're not using these, obviously... */
    (void)argc;
    (void)argv;

    /* Comment this out if you'd rather that debug output went to dcload. Of
       course, you'll need to be using dcload-ip, but you should have already
       known that. ;-) */
    dbgio_dev_select("fb");

    printf("KallistiOS Dreameye Image Dump program\n");
    printf("Attempting to find a connected Dreameye device...\n");

    dreameye = maple_enum_type(0, MAPLE_FUNC_CAMERA);

    if(!dreameye) {
        printf("Couldn't find any attached devices, bailing out.\n");
        exit(EXIT_FAILURE);
    }

    state = (dreameye_state_t *)maple_dev_status(dreameye);

    printf("Attempting to grab the number of saved images...\n");
    dreameye_get_image_count(dreameye, 1);

    printf("Image Count is %s -- (%d)\n",
           state->image_count_valid ? "valid" : "invalid", state->image_count);
    img_count = state->image_count;

    /* Initialize the low-level SD card stuff. */
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

    if(fs_ext2_mount("/sd", &sd_dev, FS_EXT2_MOUNT_READWRITE)) {
        printf("Could not mount SD card as ext2fs. Please make sure the card "
               "has been properly formatted.\n");
        exit(EXIT_FAILURE);
    }

    /* Try to make a "dreameye" directory on the root of the card and move to
       the new directory. */
    if(mkdir("/sd/dreameye", 0777)) {
        printf("Cannot create a dreameye directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(chdir("/sd/dreameye")) {
        printf("Cannot set current working directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < img_count; ++i) {
        printf("Reading image %d...\n", i + 1);
        err = dreameye_get_image(dreameye, i + 2, &buf, &size);

        if(err != MAPLE_EOK) {
            printf("Error was: %d\n", err);
            free(buf);
            continue;
        }

        printf("Image received successfully, size %d bytes\n", size);
        sprintf(fn, "image%02d.jpg", i + 1);

        if(!(fp = fopen(fn, "wb"))) {
            printf("Cannot open /sd/dreameye/%s: %s\n", fn, strerror(errno));
            free(buf);
            continue;
        }

        if(fwrite(buf, 1, size, fp) != (size_t)size) {
            printf("Cannot write image to file: %s\n", strerror(errno));
            free(buf);
            fclose(fp);
            continue;
        }

        fclose(fp);
        free(buf);
    }

    /* Clean up the filesystem and everything else */
    fs_ext2_unmount("/sd");
    fs_ext2_shutdown();
    sd_shutdown();

    printf("Complete!\n");
    return 0;
}
