/* KallistiOS ##version##

   dirent.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Falco Girgis

*/

/** \file    dirent.h
    \brief   Directory entry functionality.
    \ingroup vfs_posix

    This partially implements the standard POSIX dirent.h functionality.

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __SYS_DIRENT_H
#define __SYS_DIRENT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <unistd.h>
#include <stdint.h>
#include <kos/fs.h>
#include <kos/limits.h>

/** \addtogroup vfs_posix
    @{
*/

/** \name  Directory File Types
    \brief POSIX file types for dirent::d_type

    \remark
    These directory entry types are not part of the POSIX specifican per-se,
    but are used by BSD and glibc.

    \todo Ensure each VFS driver maps its directory types accordingly

    @{
*/
#define DT_UNKNOWN  0   /**< \brief Unknown */
#define DT_FIFO     1   /**< \brief Named Pipe or FIFO */
#define DT_CHR      2   /**< \brief Character Device */
#define DT_DIR      4   /**< \brief Directory */
#define DT_BLK      6   /**< \brief Block Device */
#define DT_REG      8   /**< \brief Regular File */
#define DT_LNK      10  /**< \brief Symbolic Link */
#define DT_SOCK     12  /**< \brief Local-Domain Socket */
#define DT_WHT      14  /**< \brief Whiteout (ignored) */
/** @} */

/** \brief POSIX directory entry structure.

    This structure contains information about a single entry in a directory in
    the VFS.
 */
struct dirent {
    int      d_ino;    /**< \brief File unique identifier */
    off_t    d_off;    /**< \brief File offset */
    uint16_t d_reclen; /**< \brief Record length */
    uint8_t  d_type;   /**< \brief File type */
    /** \brief File name

        \warning
        This field is a flexible array member, which means the structure
        requires manual over-allocation to reserve storage for this string.
        \note
        This allows us to optimize our memory usage by only allocating
        exactly as many bytes as the string is long for this field.
    */
    char     d_name[];
};

/** \brief Type representing a directory stream.

    This type represents a directory stream and is used by the directory reading
    functions to trace their position in the directory.

    \note
    The end of this structure is providing extra fixed storage for its inner
    d_ent.d_name[] FAM, hence the unionization of the d_ent structure along
    with a d_name[NAME_MAX] extension.
*/
typedef struct {
    /** \brief File descriptor for the directory */
    file_t                fd;
    /** \brief Union of dirent + extended dirent required for C++ */
    union {
        /** \brief Current directory entry */
        struct dirent     d_ent; 
        /** \brief Extended dirent structure with name storage */
        struct {
            /** \brief Current directory entry (alias) */
            struct dirent d_ent2;
            /** \brief Storage for d_ent::d_name[] FAM */
            char          d_name[NAME_MAX + 1];
        };
    };
} DIR;

/* The following functions that will be defined in <dirent.h> are not currently
    implemented in KOS:

    DIR *fdopendir(int);
    readdir_r(DIR *__restrict, struct dirent *__restrict,
    struct dirent **__restrict);
    void _seekdir(DIR *, long);
    void seekdir(DIR *, long);
    long telldir(DIR *);
    int scandirat(int, const char *, struct dirent ***,
    int (*) (const struct dirent *), int (*) (const struct dirent **,
    const struct dirent **));
    int versionsort(const struct dirent **, const struct dirent **);
    ssize_t posix_getdents(int, void *, size_t, int);
*/

/** @} */

__END_DECLS

#endif
