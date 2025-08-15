/* KallistiOS ##version##

   include/kos/library.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Ruslan Rostovtsev

*/

/** \file    kos/library.h
    \brief   Dynamically loadable library support.
    \ingroup system_libraries

    This file contains definitions for accessing loadable libraries at runtime.
    Each library has a name and a version number that it can be referenced by.
    One must be careful with these dynamic libraries as there is no private
    storage per instance, and all memory space is shared.

    Libraries can both export and import symbols. Imported symbols may require
    other libraries to be loaded earlier. Libraries are reference counted so
    that they can be opened multiple times without actually loading them
    multiple times, and so that a close will act as expected in situations like
    this.

    \author Megan Potter
    \author Ruslan Rostovtsev
*/

#ifndef __KOS_LIBRARY_H
#define __KOS_LIBRARY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/thread.h>
#include <kos/elf.h>
#include <kos/fs.h>

/** \defgroup system_libraries  Libraries
    \brief                      API for managing dynamically loaded libraries
    \ingroup                    system

    @{
*/

/** \cond */
/* Pre-define list/queue types */
struct klibrary;
TAILQ_HEAD(klqueue, klibrary);
LIST_HEAD(kllist, klibrary);
/** \endcond */

/* Thread IDs are ok for us */
typedef tid_t libid_t;                  /**< \brief Library ID type. */

/** \brief  Loaded library structure.

    This structure represents a single loaded library. Each library is
    essentially a loaded binary of code and a set of exported entry points that
    are standardized.

    Each loaded library should export at least the functions described in this
    structure:
    \li     const char *lib_get_name()
    \li     uint32_t %lib_get_version()
    \li     int lib_open(struct klibrary *lib)
    \li     int lib_close(struct klibrary *lib)

    You should not modify any members of this structure yourself (except if you
    are implementing a library).

    \headerfile kos/library.h
*/
typedef struct klibrary {
    /** \brief  Library list handle.

        Contrary to what doxygen might think, this is not a function.
    */
    LIST_ENTRY(klibrary) list;

    /** \brief  Library ID (assigned at runtime). */
    libid_t libid;

    /** \brief  Library flags. */
    uint32_t  flags;

    /** \brief  ELF image for this library.

        This can be used to look up additional entry points in the library.
    */
    elf_prog_t image;

    /** \brief  Library reference count.

        This value is incremented every time the library is opened, and
        decremented each time it is closed. Once the library's reference count
        hits 0, a close will actually destroy the library.
    */
    int refcnt;

    /* Standard library entry points. Every loaded library must provide
       at least these things. */

    /** \brief  Retrieve the library's symbolic name.

        This function must be implemented by all loadable libraries to fetch the
        library's symbolic name. This function must work before calling
        lib_open() on the library.

        \return             The library's symbolic name
    */
    const char *(*lib_get_name)(void);

    /** \brief  Retrieve the library's version.

        This function must be implemented by all loadble libraries to fetch the
        library's version number. This function must work before calling
        lib_open() on the library.

        \return             The library's version number
    */
    uint32_t(*lib_get_version)(void);

    /** \brief  Open a library.

        This function must be implemented by all loadable libraries to
        initialize the library on load.

        \param  lib         The library structure
        \return             Values >= 0 indicate success, < 0 indicates failure.
                            A failure on the first lib_open is indicative that
                            the library should be removed from memory.
    */
    int (*lib_open)(struct klibrary *lib);

    /** \brief  Close an opened library.

        This function must be implemented by all loadable libraries to close and
        deinitialize a library.

        \param  lib         The library structure
        \return             Values >= 0 indicate success, < 0 indicates failure
    */
    int (*lib_close)(struct klibrary *lib);
} klibrary_t;

/* Library flag values */
#define LIBRARY_DEFAULTS    0           /**< \brief Defaults: no flags */

/** \cond */
/* Library list; note: do not manipulate directly */
extern struct kllist library_list;
/** \endcond */

/** \brief  Look up a library by ID.

    This function looks up a library by its library ID.

    \param  libid           The library ID to look up
    \return                 The specified library, or NULL if not found
*/
klibrary_t *library_by_libid(libid_t libid);

/** \brief  Create a new library shell.

    This function creates a new library, adding it to the list of libraries. You
    generally should not call this function directly, unless you have some good
    reason to do so.

    \param  flags           Flags to create the library with.
    \return                 The newly created library, or NULL on error
*/
klibrary_t *library_create(int flags);

/** \brief  Destroy a library.

    This function will take a loaded library and destroy it, unloading it
    completely. Generally, you should not call this function, but rather use
    library_close() to make sure that you're not closing something that is still
    in use.

    \param  lib             The library to close
    \retval 0               Upon successfully destroying the library
*/
int library_destroy(klibrary_t *lib);

/** \brief  Try to open a library by name.

    This function attempts to open a library by its name. If it cannot be found
    by name, this function will attempt to open by filename. If it cannot be found
    by filename, this function will attempt to load the library from the specified
    filename. If the library is already opened, this may only involve increasing
    the reference count.

    \param  name            The symbolic name of the library
    \param  fn              The filename to load the library from
    \return                 A handle to the library, or NULL on error with errno
                            set as appropriate

    \par    Error Conditions:
    \em     EINVAL - the library was found or loaded, but invalid \n
    \em     ENOMEM - out of memory \n
    \em     ENOENT - library not found and no filename given
*/
klibrary_t *library_open(const char *name, const char *fn);

/** \brief  Look up a library by name.

    This is useful if you want to reuse opened library and
    this is used for library_open().

    \param  name            The name of the library to search for
    \return                 The library, if found. NULL if not found, errno set
                            as appropriate.

    \par    Error Conditions:
    \em     ENOENT - the library was not found
*/
klibrary_t *library_lookup(const char *name);

/** \brief  Look up a library by filename.

    This is useful if you want to reuse opened library and
    this is used for library_open().

    \param  fn              The filename of the library to search for
    \return                 The library, if found. NULL if not found, errno set
                            as appropriate.

    \par    Error Conditions:
    \em     ENOENT - the library was not found
*/
klibrary_t *library_lookup_fn(const char *fn);

/** \brief  Close a previously opened library.

    This function will close the specified library. This may involve simply
    decrementing its reference count, however, it may also involve actually
    closing and freeing the library. Thus, don't try to use the library after
    calling this without reopening it first.

    \param  lib             The library to close
    \retval 0               On success
    \retval -1              On error, errno may be set to an appropriate code

    \par    Error Conditions:
    \em     EINVAL - the library is not valid
*/
int library_close(klibrary_t *lib);

/** \brief  Retrieve the specified library's runtime-assigned ID.
    \param  lib             The library to examine
    \return                 The library's ID, or -1 on error

    \par    Error Conditions:
    \em     EINVAL - the library is not valid
*/
libid_t library_get_libid(klibrary_t *lib);

/** \brief  Retrieve the specified library's reference count.
    \param  lib             The library to examine
    \return                 The library's reference count, or -1 on error

    \par    Error Conditions:
    \em     EINVAL - the library is not valid
*/
int library_get_refcnt(klibrary_t *lib);

/** \brief  Retrieve the specified library's name.
    \param  lib             The library to examine
    \return                 The library's symbolic name, or NULL on error

    \par    Error Conditions:
    \em     EINVAL - the library is not valid
*/
const char *library_get_name(klibrary_t *lib);

/** \brief  Retrieve the specified library's version.
    \param  lib             The library to examine
    \return                 The library's version number, or 0 on error

    \par    Error Conditions
    \em     EINVAL - the library is not valid
*/
uint32_t library_get_version(klibrary_t *lib);

/** \cond */
/* Init */
void library_init(void);

/* Shutdown */
void library_shutdown(void);
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_LIBRARY_H */
