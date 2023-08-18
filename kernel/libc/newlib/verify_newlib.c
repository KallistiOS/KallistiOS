/* KallistiOS ##version##

   verify_newlib.c
   Copyright (C)2004 Megan Potter

*/


// This is a symbol we add in the KOS-specific Newlib patch. This function
// will get linked in by crtbegin and ensure that the user is using a
// patched Newlib (otherwise a link error will occur).
extern int __newlib_kos_patch;
void __verify_newlib_patch(void) {
    (void)__newlib_kos_patch;
}
