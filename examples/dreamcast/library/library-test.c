/* KallistiOS ##version##

   library-test.c
   Copyright (C) 2024 Ruslan Rostovtsev

   This example program simply show how library works.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <arch/arch.h>

#include <kos/init.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/library.h>
#include <kos/exports.h>

#include "library-dependence.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_EXPORT);

extern export_sym_t libtest_symtab[];
static symtab_handler_t st_libtest = {
    {
        "sym/library/test",
        0,
        0x00010000,
        0,
        NMMGR_TYPE_SYMTAB,
        NMMGR_LIST_INIT
    },
    libtest_symtab
};

static void __attribute__((__noreturn__)) wait_exit(void) {
    maple_device_t *dev;
    cont_state_t *state;

    dbglog(DBG_INFO, "Press any button to exit.\n");

    for(;;) {
        dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(dev) {
            state = (cont_state_t *)maple_dev_status(dev);
            if(state) {
                if(state->buttons) {
                    arch_exit();
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {

    klibrary_t *lib_dependence;
    klibrary_t *lib_dependent;
    export_sym_t *sym;
    uint32_t ver;
    library_test_func_t library_test_func;
    library_test_func2_t library_test_func2;

    // dbgio_dev_select("fb");
    dbglog(DBG_INFO, "Initializing exports.\n");

    if(nmmgr_handler_add(&st_libtest.nmmgr) < 0) {
        dbglog(DBG_ERROR, "Failed.");
        wait_exit();                             
        return -1;
    }

    dbglog(DBG_INFO, "Loading /rd/library-dependence.klf\n");
    lib_dependence = library_open("dependence", "/rd/library-dependence.klf");

    if (lib_dependence == NULL) {
        dbglog(DBG_ERROR, "Loading failed.\n");
        wait_exit();
        return -1;
    }

    ver = library_get_version(lib_dependence);

    dbglog(DBG_INFO, "Successfully loaded: %s v%ld.%ld.%ld\n",
        library_get_name(lib_dependence),
        (ver >> 16) & 0xff, (ver >> 8) & 0xff, ver & 0xff);

    dbglog(DBG_INFO, "Loading /rd/library-dependence.klf\n");
    lib_dependent = library_open("dependent", "/rd/library-dependent.klf");

    if (lib_dependence == NULL) {
        dbglog(DBG_ERROR, "Loading failed.\n");
        wait_exit();
        return -1;
    }

    ver = library_get_version(lib_dependent);

    dbglog(DBG_INFO, "Successfully loaded: %s v%ld.%ld.%ld\n",
        library_get_name(lib_dependent),
        (ver >> 16) & 0xff, (ver >> 8) & 0xff, ver & 0xff);

    dbglog(DBG_INFO, "Testing exports runtime on host\n");

    sym = export_lookup("library_test_func");
    
    if (sym && sym->ptr != (uint32_t)-1) {
        library_test_func = (library_test_func_t)sym->ptr;
        library_test_func(444);
    }
    else {
        dbglog(DBG_ERROR, "Lookup symbol failed: library_test_func");
    }

    sym = export_lookup("library_test_func2");

    if (sym && sym->ptr != (uint32_t)-1) {
        library_test_func2 = (library_test_func2_t)sym->ptr;
        library_test_func2("Hello from library test");
    }
    else {
        dbglog(DBG_ERROR, "Lookup symbol failed: library_test_func");
    }

    library_close(lib_dependent);
    library_close(lib_dependence);
    nmmgr_handler_remove(&st_libtest.nmmgr);

    wait_exit();
    return 0;
}
