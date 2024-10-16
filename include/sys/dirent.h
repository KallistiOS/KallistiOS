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

#include <kos/cdefs.h>

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

// Standard UNIX dir functions. Not all of these are fully functional
// right now due to lack of support in KOS.

/** \brief  Open a directory based on the specified name.

    The directory specified is opened if it exists. A directory stream object is
    returned for accessing the entries of the directory.

    \note               As with other functions for opening files on the VFS,
                        relative paths are permitted for the name parameter of
                        this function.

    \param  name        The name of the directory to open.

    \return             A directory stream object to be used with readdir() on
                        success, NULL on failure. Sets errno as appropriate.
    \see    closedir
    \see    readdir
*/
DIR *opendir(const char *name);

/** \brief  Closes a directory that was previously opened.

    This function is used to close a directory stream that was previously opened
    with the opendir() function. You must do this to clean up any resources
    associated with the directory stream.

    \param  dir         The directory stream to close.

    \return             0 on success. -1 on error, setting errno as appropriate.
*/
int closedir(DIR *dir);

/** \brief  Read an entry from a directory stream.

    This function reads the next entry from the directory stream provided,
    returning the directory entry associated with the next object in the
    directory.

    \warning            Do not free the returned dirent!

    \param  dir         The directory stream to read from.

    \return             A pointer to the next directory entry in the directory
                        or NULL if there are no other entries in the directory.
                        If an error is incurred, NULL will be returned and errno
                        set to indicate the error.
*/
struct dirent *readdir(DIR *dir);

/** \brief  Retrieve the file descriptor of an opened directory stream.

    This function retrieves the file descriptor of a directory stream that was
    previously opened with opendir().

    \warning            Do not close() the returned file descriptor. It will be
                        closed when closedir() is called on the directory
                        stream.

    \param  dirp        The directory stream to retrieve the descriptor of.

    \return             The file descriptor from the directory stream on success
                        or -1 on failure (sets errno as appropriate).
*/
int dirfd(DIR *dirp);

/** \brief  Rewind a directory stream to the start of the directory.

    This function rewinds the directory stream so that the next call to the
    readdir() function will return the first entry in the directory.

    \warning            Some filesystems do not support this call. Notably, none
                        of the dcload filesystems support it. Error values will
                        be returned in errno (so set errno to 0, then check
                        after calling the function to see if there was a problem
                        anywhere).

    \param  dir         The directory stream to rewind.
*/
void rewinddir(DIR *dir);

/** \brief Scan, filter, and sort files within a directory.

    This function scans through all files within the directory located at the
    path given by \p dir, calling \p filter on each entry. Entries for which
    \p filter returns nonzero are stored within \p namelist and are sorted
    using qsort() with the comparison function, \p compar. The resulting
    directory entries are accumulated and stored witin \p namelist.

    \note
    \p filter and \p compar may be NULL, if you do not wish to filter or sort
    the files.

    \warning
    The entries within \p namelist are each independently heap-allocated, then
    the list itself heap allocated, so each entry must be freed within the list
    followed by the list itself.

    \param  dir         The path to the directory to scan
    \param  namelist    A pointer through which the list of entries will be
                        returned.
    \param  filter      The callback used to filter each directory entry
                        (returning 1 for inclusion, 0 for exclusion).
    \param  compar      The callback passed to qsort() to sort \p namelist by

    \retval >=0         On success, the number of directory entries within \p
                        namelist is returned
    \retval -1          On failure, -1 is returned and errno is set

    \sa alphasort
*/
int scandir(const char *__RESTRICT dir, struct dirent ***__RESTRICT namelist,
            int(*filter)(const struct dirent *),
            int(*compar)(const struct dirent **, const struct dirent **));


/** \brief Comparison function for sorting directory entries alphabetically

    Sorts two directory entries, \p a and \p b in alphabetical order.

    \note
    This function can be used as the comparison callback passed to scandir(),
    to sort the returned list of entries in alphabetical order.

    \param  a   The first directory entry to sort
    \param  b   The second directory entry to sort

    \retval     Returns an integer value greater than, equal to, or less than
                zero, depending on whether the name of the directory entry
                pointed to by \p a is lexically greater than, equal to, or
                less than the directory entry pointed to by \p b.

    \sa scandir()
*/
int alphasort(const struct dirent **a, const struct dirent **b);

/** \brief Not implemented */
void seekdir(DIR *dir, off_t offset);

/** \brief Not implemented */
off_t telldir(DIR *dir);

/** @} */

__END_DECLS

#endif
