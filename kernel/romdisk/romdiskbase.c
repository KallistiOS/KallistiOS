/* KallistiOS ##version##

   kernel/romdisk/romdiskbase.c
   Copyright (C) 2023 Paul Cercueil
*/

#include <kos/fs_romdisk.h>

extern unsigned char romdisk[];

void *__kos_romdisk = romdisk;

int fs_romdisk_mount_weak(void)
{
    return fs_romdisk_mount("/rd", __kos_romdisk, 0);
}
