/* KallistiOS ##version##

   library-dependence.c
   Copyright (C) 2024 Ruslan Rostovtsev

   This example program simply show how library works.
*/

#include <stdint.h>
#include <kos/dbglog.h>
#include <kos/library.h>
#include <kos/exports.h>
#include <kos/version.h>

extern export_sym_t library_symtab[];
static symtab_handler_t library_hnd = {
    {
        "sym/library/dependence",
        0,
        0x00010000,
        0,
        NMMGR_TYPE_SYMTAB,
        NMMGR_LIST_INIT
    },
    library_symtab
};

/* Library functions */
const char *lib_get_name() {
    return library_hnd.nmmgr.pathname + 12;
}

uint32_t lib_get_version() {
    return KOS_VERSION_MAKE(1, 0, 0);
}

int lib_open(klibrary_t *lib) {
    dbglog(DBG_DEBUG, "Library \"%s\" opened.\n", lib_get_name());
    return nmmgr_handler_add(&library_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
    dbglog(DBG_DEBUG, "Library \"%s\" closed.\n", lib_get_name());
    return nmmgr_handler_remove(&library_hnd.nmmgr);
}

/* Exported functions */
int library_test_func(int arg) {
    dbglog(DBG_DEBUG, "Library \"%s\" test int: %d\n", lib_get_name(), arg);
    return 0;
}

void library_test_func2(const char *arg) {
    dbglog(DBG_DEBUG, "Library \"%s\" test char: %s\n", lib_get_name(), arg);
}
