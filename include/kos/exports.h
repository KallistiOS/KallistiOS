/* KallistiOS ##version##

   kos/exports.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Ruslan Rostovtsev

*/

/** \file    kos/exports.h
    \brief   Kernel exported symbols support.
    \ingroup system_libraries

    This file contains support related to dynamic linking of the kernel of KOS.
    The kernel (at compile time) produces a list of exported symbols, which can
    be looked through using the functionality in this file.

    \author Megan Potter
    \author Ruslan Rostovtsev
*/

#ifndef __KOS_EXPORTS_H
#define __KOS_EXPORTS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <stdint.h>

/** \addtogroup system_libraries
    @{
*/

/** \brief  A single export symbol.

    This structure holds a single symbol that has been exported from the kernel.
    These will be patched into loaded ELF binaries at load time.

    \headerfile kos/exports.h
*/
typedef struct export_sym {
    const char *name;     /**< \brief The name of the symbol. */
    uintptr_t ptr;        /**< \brief A pointer to the symbol. */
} export_sym_t;

/** \cond */
/* These are the platform-independent exports */
extern export_sym_t kernel_symtab[];

/* And these are the arch-specific exports */
extern export_sym_t arch_symtab[];
/** \endcond */

#ifndef __EXPORTS_FILE
#include <kos/nmmgr.h>

/** \brief  A symbol table "handler" for nmmgr.
    \headerfile kos/exports.h
*/
typedef struct symtab_handler {
    struct nmmgr_handler nmmgr;   /**< \brief Name manager handler header */
    export_sym_t *table;          /**< \brief Location of the first entry */
} symtab_handler_t;
#endif

/** \brief  Setup initial kernel exports. */
void export_init(void);

/** \brief  Look up a symbol by name.
    \param  name            The symbol to look up
    \return                 The export structure, or NULL on failure
*/
export_sym_t *export_lookup(const char *name);

/** \brief  Look up a symbol by name and Name Manager path.
    \param  name            The symbol to look up
    \param  path            The Name Manager path to look up
    \return                 The export structure, or NULL on failure
*/
export_sym_t *export_lookup_path(const char *name, const char *path);

/** \brief  Look up a symbol by approx addr.
            It can be useful for unhandled exceptions messages.
    \param  addr            The symbol to look up
    \return                 The export structure, or NULL on failure
*/
export_sym_t *export_lookup_addr(uintptr_t addr);

/** @} */

__END_DECLS

#endif  /* __KOS_EXPORTS_H */
