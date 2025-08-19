/* KallistiOS ##version##

   kernel/library.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Ruslan Rostovtsev
*/

#include <assert.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <arch/irq.h>
#include <kos/library.h>

/*

With KOS 1.3.x, we introduce the idea of a loadable library. It's a little
bit different from the standard *n*x concept of a library. Basically in
KOS, a library is a memory image plus a collection of exported names
associated with that image. Each library has a symbolic name and a version
that it can be referenced by, and all "shared libraries" are actually the
same exact library (there is no COW-style private storage per instance). So
it is somewhat similar to an AmigaOS style library or perhaps a Linux
kernel module.

Libraries may export names using the name manager, and may also require
imports through the name manager. If imports are required then other
libraries may have to have been loaded previously. (Note: We should maybe
eventually do dependencies like Linux...?)

To use a library, a client will call library_open with a symbolic name and
a filename. If the symbolic name is non-NULL and already exists, we'll
just increment its refcount and return a handle to it; otherwise we try to
load from the passed filename (if it is non-NULL). This design is used
because it's expected that some libraries (TCP/IP for example) can be used
from multiple other libraries without being reloaded or reinitialized. Some
modules (like instances of a shell) we may want to load each time.

Once a library has been opened, it may start threads and begin active
processing of its own, or it may simply add exports to the name manager
system (which can be found with nmmgr_lookup). Or simpler yet, it may add
itself to some existing system like the VFS or networking as a driver.

When a client is finished with the library, they library_close on it. This
decreases the reference count and may cause the library to be uninitialized
and unloaded if it is completely unused.

*/

/*****************************************************************************/

/* Library list */
struct kllist library_list;

/*****************************************************************************/
/* Debug */
#include <kos/dbgio.h>

int library_print_list(int (*pf)(const char *fmt, ...)) {
    klibrary_t *cur;

    pf("All libraries (may not be deterministic):\n");
    pf("libid\tflags\t\tbase\t\tname\n");

    LIST_FOREACH(cur, &library_list, list) {
        pf("%d\t", cur->libid);
        pf("%08lx\t", cur->flags);
        pf("%p\t", cur->image.data);
        pf("%s\n", cur->lib_get_name());
    }
    pf("--end of list--\n");

    return 0;
}

/*****************************************************************************/
/* Returns a fresh library ID for each new lib */

/* Highest library id (used when assigning next library id) */
static libid_t libid_highest;


/* Return the next available library id (assumes wraparound will not run
   into old libraries). */
static libid_t library_next_free(void) {
    int id;
    id = libid_highest++;
    return id;
}

/* Given a library ID, locates the library structure */
klibrary_t *library_by_libid(libid_t libid) {
    klibrary_t *np;

    LIST_FOREACH(np, &library_list, list) {
        if(np->libid == libid)
            return np;
    }

    return NULL;
}


/*****************************************************************************/
/* Library shell creation and deletion */

klibrary_t * library_create(int flags) {
    klibrary_t  * np;
    libid_t     libid;

    irq_disable_scoped();
    np = NULL;

    /* Get a new library id */
    libid = library_next_free();

    if(libid >= 0) {
        /* Create a new library structure */
        np = malloc(sizeof(klibrary_t));

        if(np != NULL) {
            /* Clear out potentially unused stuff */
            memset(np, 0, sizeof(klibrary_t));

            /* Populate the context */
            np->libid = libid;
            np->flags = flags;
            np->refcnt = 1;

            /* Insert it into the library list */
            LIST_INSERT_HEAD(&library_list, np, list);
        }
    }

    return np;
}

int library_destroy(klibrary_t *lib) {
    int oldirq = 0;

    oldirq = irq_disable();

    /* Free up the image */
    if(lib->image.data)
        elf_free(&lib->image);

    /* Remove it from the global list */
    LIST_REMOVE(lib, list);

    irq_restore(oldirq);

    /* Free the memory */
    free(lib);

    return 0;
}

/*****************************************************************************/
/* Library attribute functions -- all read-only */

libid_t library_get_libid(klibrary_t *lib) {
    if(lib == NULL) {
        errno = EINVAL;
        return -1;
    }
    else
        return lib->libid;
}

int library_get_refcnt(klibrary_t *lib) {
    if(lib == NULL) {
        errno = EINVAL;
        return -1;
    }
    else
        return lib->refcnt;
}

const char *library_get_name(klibrary_t *lib) {
    if(lib == NULL || lib->lib_get_name == NULL) {
        errno = EINVAL;
        return NULL;
    }
    else
        return lib->lib_get_name();
}

uint32_t library_get_version(klibrary_t *lib) {
    if(lib == NULL || lib->lib_get_version == NULL) {
        errno = EINVAL;
        return 0;
    }
    else
        return lib->lib_get_version();
}

/*****************************************************************************/
klibrary_t *library_lookup(const char *name) {
    klibrary_t *lib;

    irq_disable_scoped();

    LIST_FOREACH(lib, &library_list, list) {
        if(!strcasecmp(lib->lib_get_name(), name))
            break;
    }

    if(!lib)
        errno = ENOENT;

    return lib;
}

klibrary_t *library_lookup_fn(const char *fn) {
    klibrary_t *lib;

    irq_disable_scoped();

    LIST_FOREACH(lib, &library_list, list) {
        if(!strncmp(lib->image.fn, fn, NAME_MAX))
            break;
    }

    if(!lib)
        errno = ENOENT;

    return lib;
}

klibrary_t *library_open(const char *name, const char *fn) {
    klibrary_t *lib = NULL;

    // If they passed us a valid name, try that first.
    if(name) {
        lib = library_lookup(name);
    }
    else {
        lib = library_lookup_fn(fn);
    }

    if(lib) {
        ++lib->refcnt;
        return lib;
    }

    // Ok, we need to load. Make sure we have a valid filename.
    if(!fn) {
        errno = ENOENT;
        return NULL;
    }

    lib = library_create(LIBRARY_DEFAULTS);

    if(!lib) {
        errno = ENOMEM;
        return NULL;
    }

    // Use the ELF functions to load the memory image and extract the
    // entry points we need.
    if(elf_load(fn, lib, &lib->image) < 0) {
        library_destroy(lib);
        return NULL;
    }

    // Pull out the image pointers
    lib->lib_get_name = (const char * (*)())lib->image.lib_get_name;
    lib->lib_get_version = (uint32_t(*)())lib->image.lib_get_version;
    lib->lib_open = (int (*)(klibrary_t *))lib->image.lib_open;
    lib->lib_close = (int (*)(klibrary_t *))lib->image.lib_close;

    // Make sure they are all valid.
    if(!lib->lib_get_name || !lib->lib_get_version ||
            !lib->lib_open || !lib->lib_close) {
        errno = EINVAL;
        library_destroy(lib);
        return NULL;
    }

    // Call down and "open" the library itself.
    if(lib->lib_open(lib) < 0) {
        library_destroy(lib);
        return NULL;
    }

    // It's all good.
    return lib;
}

int library_close(klibrary_t *lib) {
    // Make sure it's valid.
    if(lib == NULL || lib->lib_close == NULL) {
        errno = EINVAL;
        return -1;
    }

    // Check for reference
    if(--lib->refcnt > 0) {
        return 0;
    }

    // Call down and "close" the lib.
    if(lib->lib_close(lib) < 0)
        return -1;

    // Unload this lib.
    library_destroy(lib);

    // It's all good.
    return 0;
}


/*****************************************************************************/
/* Init/shutdown */

/* Init */
void library_init(void) {
    /* Initialize handle counters */
    libid_highest = 1;

    /* Initialize the library list */
    LIST_INIT(&library_list);
}

/* Shutdown */
void library_shutdown(void) {
    klibrary_t *n1, *n2;

    /* Kill remaining libraries */
    n1 = LIST_FIRST(&library_list);

    while(n1 != NULL) {
        n2 = LIST_NEXT(n1, list);
        free(n1);
        n1 = n2;
    }

    LIST_INIT(&library_list);
}
