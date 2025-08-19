/* KallistiOS ##version##

   library-dependent.c
   Copyright (C) 2024 Ruslan Rostovtsev

   This example program simply show how library works.
*/

#include <stdint.h>
#include <string.h>
#include <kos/dbglog.h>
#include <kos/library.h>
#include <kos/exports.h>
#include <kos/version.h>
#include <kos/md5.h>

#include "library-dependence.h"

const char *lib_get_name() {
    return "dependent";
}

uint32_t lib_get_version() {
    return KOS_VERSION_MAKE(1, 0, 0);
}

int lib_open(klibrary_t *lib) {
    uint8_t output[16];

    dbglog(DBG_INFO, "Library \"%s\" opened.\n", lib_get_name());

    // Test exports from dependence library
    library_test_func(333);
    library_test_func2("Hello from library dependent");

    // Test libkosutils from dependence library
    kos_md5((const uint8_t *)lib_get_name(), strlen(lib_get_name()), output);
    dbglog(DBG_INFO, "MD5 of \"%s\": %02X%02X%02X%02X...\n",
        lib_get_name(), output[0], output[1], output[2], output[3]);

    // Test host exported newlib
    if(strcmp(lib_get_name(), "dependent")) {
        return -1;
    }

    return 0;
}

int lib_close(klibrary_t *lib) {
    dbglog(DBG_INFO, "Library \"%s\" closed.\n", lib_get_name());
    return 0;
}
