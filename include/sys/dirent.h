/* KallistiOS ##version##

   dirent.h
   Copyright (C) 2003 Megan Potter

*/

/** \file   dirent.h
    \brief  Directory entry functionality.

    This partially implements the standard POSIX dirent.h functionality.

    \author Megan Potter
*/

#ifndef __SYS_DIRENT_H
#define __SYS_DIRENT_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <unistd.h>
#include <arch/types.h>
#include <kos/fs.h>

/** \brief  POSIX directory entry structure.

    This structure contains information about a single entry in a directory in
    the VFS.

    \headerfile sys/dirent.h
 */
struct dirent {
    int     d_ino;              /**< \brief File unique identifier. */
    off_t   d_off;              /**< \brief File offset */
    uint16  d_reclen;           /**< \brief Record length */
    uint8   d_type;             /**< \brief File type */
    char    d_name[256];        /**< \brief Filename */
};

/** \brief  Type representing a directory stream.

    This type represents a directory stream and is used by the directory reading
    functions to trace their position in the directory.

    The values in this function are all private and subject to change. Do not
    attempt to use any of them directly.

    \headerfile sys/dirent.h
*/
typedef struct {
    file_t          fd;         /**< \brief File descriptor for the directory */
    struct dirent   d_ent;      /**< \brief Current directory entry */
} DIR;

// Standard UNIX dir functions. Not all of these are fully functional
// right now due to lack of support in KOS.

/** \brief  Open a directory based on the specified name.

    The directory specified is opened if it exists. A directory stream object is
    returned for accessing the entries of the directory.

    \param  name        The name of the directory to open.
    \return             A directory stream object to be used with readdir() on
                        success, NULL on failure. Sets errno as appropriate.
    \note               As with other functions for opening files on the VFS,
                        relative paths are permitted for the name parameter of
                        this function.
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

    \param  dir         The directory stream to read from.
    \return             A pointer to the next directory entry in the directory
                        or NULL if there are no other entries in the directory.
                        If an error is incurred, NULL will be returned and errno
                        set to indicate the error.

    \note               Do not free the returned dirent!
*/
struct dirent *readdir(DIR *dir);

/** \brief  Retrieve the file descriptor of an opened directory stream.

    This function retrieves the file descriptor of a directory stream that was
    previously opened with opendir().

    \param  dirp        The directory stream to retrieve the descriptor of.
    \return             The file descriptor from the directory stream on success
                        or -1 on failure (sets errno as appropriate).

    \note               Do not close() the returned file descriptor. It will be
                        closed when closedir() is called on the directory
                        stream.
*/
int dirfd(DIR *dirp);

/** \brief  Rewind a directory stream to the start of the directory.

    This function rewinds the directory stream so that the next call to the
    readdir() function will return the first entry in the directory.

    \param  dir         The directory stream to rewind.

    \note               Some filesystems do not support this call. Notably, none
                        of the dcload filesystems support it. Error values will
                        be returned in errno (so set errno to 0, then check
                        after calling the function to see if there was a problem
                        anywhere).
*/
void rewinddir(DIR *dir);

/** \brief Not implemented */
int scandir(const char *dir, struct dirent ***namelist,
            int(*filter)(const struct dirent *),
            int(*compar)(const struct dirent **, const struct dirent **));

/** \brief Not implemented */
void seekdir(DIR *dir, off_t offset);

/** \brief Not implemented */
off_t telldir(DIR *dir);

__END_DECLS

#endif
