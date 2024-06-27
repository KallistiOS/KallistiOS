/* KallistiOS ##version##

   kernel/romdisk/romdiskbase.c
   Copyright (C) 2023 Paul Cercueil
*/

#include <kos/fs_romdisk.h>

extern const unsigned char romdisk_data[];

const void *__kos_romdisk = romdisk_data;

extern void fs_romdisk_mount_builtin(void);

void (*fs_romdisk_mount_builtin_weak)(void) = fs_romdisk_mount_builtin;
