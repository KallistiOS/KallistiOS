/* KallistiOS 2.0.0

   main.c
   (c)2001 Dan Potter
*/

#include <string.h>
#include <arch/arch.h>
#include <kos/fs.h>
#include <kos/fs_romdisk.h>

/* Ditto */
int main(int argc, char **argv);

/* Auto-init stuff: comment out here if you don't like this stuff
   to be running in your build, and also below in arch_main() */
int arch_auto_init() {

    fs_init();          /* VFS */
//  fs_ramdisk_init();      /* Ramdisk */
    fs_romdisk_init();      /* Romdisk */

    if(__kos_romdisk != NULL) {
        fs_romdisk_mount("/rd", __kos_romdisk, 0);
    }

    return 0;
}

/* This is the entry point inside the C program */
int arch_main() {

    if(mm_init() < 0)
        return 0;

    arch_auto_init();

    return main(0, NULL);
}



/* When you make a function called main() in a GCC program, it wants
   this stuff too. */
void _main() { }
void atexit() { }

/* GCC 3.0 also wants these */
void __gccmain() { }

