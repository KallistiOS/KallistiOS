/* KallistiOS ##version##

   module_main.c
   Copyright (C)2003 Megan Potter

*/

#include <kos/library.h>
#include <kos/exports.h>
#include <kos/dbglog.h>
#include <kmg/kmg.h>

const char *lib_get_name(void) {
    return "imgloader.kmg";
}

const uint32_t lib_get_version(void) {
    return KMG_VERSION;
}

int lib_open(klibrary_t *lib) {
    dbglog(DBG_INFO, "Library \"%s\" opened.\n", lib_get_name());
    return 0;
}

int lib_close(klibrary_t *lib) {
    dbglog(DBG_INFO, "Library \"%s\" closed.\n", lib_get_name());
    return 0;
}