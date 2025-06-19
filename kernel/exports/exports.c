/* KallistiOS ##version##

   exports.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Ruslan Rostovtsev

*/

/*

Just a quick interface to actually make use of all those nifty kernel
export tables. This just does a linear search through the two tables
to look for a symbol, for now. This should be optimized later with a
binary search or something, but loading a new program is generally expected
to be a somewhat slow process anyway.

*/

#include <string.h>
#include <kos/nmmgr.h>
#include <kos/exports.h>

static symtab_handler_t st_kern = {
    {
        "sym/kernel/kernel",
        0,
        0x00010000,
        0,
        NMMGR_TYPE_SYMTAB,
        NMMGR_LIST_INIT
    },
    kernel_symtab
};

static symtab_handler_t st_arch = {
    {
        "sym/kernel/arch",
        0,
        0x00010000,
        0,
        NMMGR_TYPE_SYMTAB,
        NMMGR_LIST_INIT
    },
    arch_symtab
};

void export_init(void) {
    /* Add our two export tables */
    nmmgr_handler_add(&st_kern.nmmgr);
    nmmgr_handler_add(&st_arch.nmmgr);
}

export_sym_t *export_lookup(const char *name) {
    nmmgr_handler_t *nmmgr;
    nmmgr_list_t *nmmgrs;
    int i;
    symtab_handler_t *sth;

    /* Get the name manager list */
    nmmgrs = nmmgr_get_list();

    /* Go through and look at each symtab entry */
    LIST_FOREACH(nmmgr, nmmgrs, list_ent) {
        /* Not a symtab -> ignore */
        if(nmmgr->type != NMMGR_TYPE_SYMTAB)
            continue;

        sth = (symtab_handler_t *)nmmgr;

        /* First look through the kernel table */
        for(i = 0; /* */; i++) {
            if(sth->table[i].name == NULL)
                break;

            if(!strcmp(name, sth->table[i].name))
                return sth->table + i;
        }
    }

    return NULL;
}

export_sym_t *export_lookup_path(const char *name, const char *path) {
    nmmgr_handler_t *nmmgr;
    symtab_handler_t *sth;
    int i;

    /* Get the name manager list */
    nmmgr = nmmgr_lookup(path);

    if(nmmgr == NULL) {
        return NULL;
    }
    sth = (symtab_handler_t *)nmmgr;

    for(i = 0; sth->table[i].name; i++) {
        if(!strcmp(name, sth->table[i].name))
            return sth->table + i;
    }

    return NULL;
}

export_sym_t *export_lookup_addr(uintptr_t addr) {
    nmmgr_handler_t *nmmgr;
    nmmgr_list_t *nmmgrs;
    int	i;
    symtab_handler_t *sth;

    uintptr_t dist = ~0;
    export_sym_t *best = NULL;

    /* Get the name manager list */
    nmmgrs = nmmgr_get_list();

    /* Go through and look at each symtab entry */
    LIST_FOREACH(nmmgr, nmmgrs, list_ent) {
        /* Not a symtab -> ignore */
        if(nmmgr->type != NMMGR_TYPE_SYMTAB)
            continue;

        sth = (symtab_handler_t *)nmmgr;

        /* First look through the kernel table */
        for(i = 0; sth->table[i].name; i++) {
            if(addr - sth->table[i].ptr < dist) {
                dist = addr - sth->table[i].ptr;
                best = sth->table + i;
            }
        }
    }

    return best;
}
