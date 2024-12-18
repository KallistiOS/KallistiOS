/* KallistiOS ##version##

   kos/fs.h
   Copyright (C) 2000, 2001, 2002, 2003 Megan Potter
   Copyright (C) 2012, 2013, 2014, 2016 Lawrence Sebald

*/

/** \file    kos/fs.h
    \brief   Virtual filesystem support.
    \ingroup vfs_generic

    This file contains the interface to the virtual filesystem (VFS) of KOS. The
    functions defined in this file make up the base of the filesystem operations
    that can be performed by programs. The functions in here are abstracted by
    various other layers in libc, and shouldn't be necessarily used (for
    portability reasons). However, if you want only to interact with KOS in your
    programs, feel free to use them to your heart's content!

    \author Megan Potter
    \author Lawrence Sebald
*/

#ifndef __KOS_FS_H
#define __KOS_FS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/types.h>
#include <kos/limits.h>
#include <kos/opts.h>
#include <time.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <kos/nmmgr.h>

/** \defgroup vfs_generic   Generic
    \brief                  Generic, uniform access to a virtual filesystems
    \ingroup                vfs
    @{
*/

/** \brief   Directory entry.

    All VFS handlers must conform to this interface in their directory entries.

    \headerfile kos/fs.h
*/
typedef struct kos_dirent {
    int size;               /**< \brief Size of the file in bytes. */
    char name[NAME_MAX];  /**< \brief Name of the file. */
    time_t time;            /**< \brief Last access/mod/change time (depends on VFS) */
    uint32 attr;            /**< \brief Attributes of the file. */
} dirent_t;

/* Forward declaration */
struct vfs_handler;

/* stat_t.unique */
/** \brief stat_t.unique: Constant to use denoting file has no unique ID */
#define STAT_UNIQUE_NONE    0

/* stat_t.type */
/** \brief stat_t.type: Unknown / undefined / not relevant */
#define STAT_TYPE_NONE      0

/** \brief stat_t.type: Standard file */
#define STAT_TYPE_FILE      1

/** \brief stat_t.type: Standard directory */
#define STAT_TYPE_DIR       2

/** \brief stat_t.type: A virtual device of some sort (pipe, socket, etc) */
#define STAT_TYPE_PIPE      3

/** \brief stat_t.type: Meta data */
#define STAT_TYPE_META      4

/** \brief stat_t.type: Symbolic link */
#define STAT_TYPE_SYMLINK   5

/* stat_t.attr */
#define STAT_ATTR_NONE  0x00    /**< \brief stat_t.attr: No attributes */
#define STAT_ATTR_R     0x01    /**< \brief stat_t.attr: Read-capable */
#define STAT_ATTR_W     0x02    /**< \brief stat_t.attr: Write-capable */

/** \brief stat_t.attr: Read/Write capable */
#define STAT_ATTR_RW    (STAT_ATTR_R | STAT_ATTR_W)

/** \brief  File descriptor type */
typedef int file_t;

/** \brief  Invalid file handle constant (for open failure, etc) */
#define FILEHND_INVALID ((file_t)-1)

/** \brief  VFS handler interface.

    All VFS handlers must implement this interface.

    \headerfile kos/fs.h
*/
typedef struct vfs_handler {
    /** \brief Name manager handler header */
    nmmgr_handler_t nmmgr;

    /* Some VFS-specific pieces */
    /** \brief Allow VFS caching; 0=no, 1=yes */
    int cache;
    /** \brief Pointer to private data for the handler */
    void *privdata;

    /** \brief Open a file on the given VFS; return a unique identifier */
    void *(*open)(struct vfs_handler *vfs, const char *fn, int mode);

    /** \brief Close a previously opened file */
    int (*close)(void *hnd);

    /** \brief Read from a previously opened file */
    ssize_t (*read)(void *hnd, void *buffer, size_t cnt);

    /** \brief Write to a previously opened file */
    ssize_t (*write)(void *hnd, const void *buffer, size_t cnt);

    /** \brief Seek in a previously opened file */
    off_t (*seek)(void *hnd, off_t offset, int whence);

    /** \brief Return the current position in a previously opened file */
    off_t (*tell)(void *hnd);

    /** \brief Return the total size of a previously opened file */
    size_t (*total)(void *hnd);

    /** \brief Read the next directory entry in a directory opened with O_DIR */
    dirent_t *(*readdir)(void *hnd);

    /** \brief Execute a device-specific call on a previously opened file */
    int (*ioctl)(void *hnd, int cmd, va_list ap);

    /** \brief Rename/move a file on the given VFS */
    int (*rename)(struct vfs_handler *vfs, const char *fn1, const char *fn2);

    /** \brief Delete a file from the given VFS */
    int (*unlink)(struct vfs_handler *vfs, const char *fn);

    /** \brief "Memory map" a previously opened file */
    void *(*mmap)(void *fd);

    /** \brief Perform an I/O completion (async I/O) for a previously opened
               file */
    int (*complete)(void *fd, ssize_t *rv);

    /** \brief Get status information on a file on the given VFS
        \note  path will not be passed through realpath() before calling the
               filesystem-level function. It is also important to not call
               realpath() in any implementation of this function as it is
               possible that realpath() will call this function. */
    int (*stat)(struct vfs_handler *vfs, const char *path, struct stat *buf,
                int flag);

    /** \brief Make a directory on the given VFS */
    int (*mkdir)(struct vfs_handler *vfs, const char *fn);

    /** \brief Remove a directory from the given VFS */
    int (*rmdir)(struct vfs_handler *vfs, const char *fn);

    /** \brief Manipulate file control flags on the given file */
    int (*fcntl)(void *fd, int cmd, va_list ap);

    /** \brief Check if an event is pending on the given file */
    short (*poll)(void *fd, short events);

    /** \brief Create a hard link */
    int (*link)(struct vfs_handler *vfs, const char *path1, const char *path2);

    /** \brief Create a symbolic link */
    int (*symlink)(struct vfs_handler *vfs, const char *path1,
                   const char *path2);

    /* 64-bit file access functions. Generally, you should only define one of
       the 64-bit or 32-bit versions of these functions. */

    /** \brief Seek in a previously opened file (64-bit offsets) */
    _off64_t (*seek64)(void *hnd, _off64_t offset, int whence);

    /** \brief Return the current position in an opened file (64-bit offset) */
    _off64_t (*tell64)(void *hnd);

    /** \brief Return the size of an opened file as a 64-bit integer */
    uint64 (*total64)(void *hnd);

    /** \brief Read the value of a symbolic link
        \note  path will not be passed through realpath() before calling the
               filesystem-level function. It is also important to not call
               realpath() in any implementation of this function as it is
               possible that realpath() will call this function. */
    ssize_t (*readlink)(struct vfs_handler *vfs, const char *path, char *buf,
                        size_t bufsize);

    /** \brief Rewind a directory stream to the start */
    int (*rewinddir)(void *hnd);

    /** \brief Get status information on an already opened file. */
    int (*fstat)(void *hnd, struct stat *st);
} vfs_handler_t;

/** \cond */
/* This is the private struct that will be used as raw file handles
   underlying descriptors. */
struct fs_hnd;

/* The kernel-wide file descriptor table. These will reference to open files. */
extern struct fs_hnd *fd_table[FD_SETSIZE];
/** \endcond */

/* Open modes */
#include <sys/fcntl.h>

/** \anchor vfs_fopen_modes
    \name   File Open Modes
    @{
*/
#define O_MODE_MASK 0x0f        /**< \brief Mask for mode numbers */
//#define O_TRUNC       0x0100      /* Truncate */
#define O_ASYNC     0x0200      /**< \brief Open for asynchronous I/O */
//#define O_NONBLOCK    0x0400      /* Open for non-blocking I/O */
#define O_DIR       0x1000      /**< \brief Open as directory */
#define O_META      0x2000      /**< \brief Open as metadata */
/** @} */

/** \anchor vfs_seek_modes
    \name   Seek Modes

    These are the values you can pass for the whence parameter to fs_seek().

    @{
*/
#define SEEK_SET    0           /**< \brief Set position to offset. */
#define SEEK_CUR    1           /**< \brief Seek from current position. */
#define SEEK_END    2           /**< \brief Seek from end of file. */
/** @} */

/* Standard file descriptor functions */
/** \brief   Open a file on the VFS.

    This function opens the specified file, returning a new file descriptor to
    access the file.

    \param  fn              The path to open.
    \param  mode            The mode to use with opening the file. This may
                            include the standard open modes (O_RDONLY, O_WRONLY,
                            etc), as well as values from the \ref vfs_fopen_modes
                            "File Open Modes" list. Multiple values can be ORed
                            together.
    
    \return                 The new file descriptor on success, -1 on error.
*/
file_t fs_open(const char *fn, int mode);

/** \brief   Close an opened file.

    This function closes the specified file descriptor, releasing all resources
    associated with the descriptor.

    \param  hnd             The file descriptor to close.
    
    \return                 0 for success, -1 for error
*/
int fs_close(file_t hnd);

/** \brief   Read from an opened file.

    This function reads into the specified buffer from the file at its current
    file pointer.

    \param  hnd             The file descriptor to read from.
    \param  buffer          The buffer to read into.
    \param  cnt             The size of the buffer (or the number of bytes
                            requested).
    
    \return                 The number of bytes read, or -1 on error. Note that
                            this may not be the full number of bytes requested.
*/
ssize_t fs_read(file_t hnd, void *buffer, size_t cnt);

/** \brief   Write to an opened file.

    This function writes the specified buffer into the file at the current file
    pointer.

    \param  hnd             The file descriptor to write into.
    \param  buffer          The data to write into the file.
    \param  cnt             The size of the buffer, in bytes.
    
    \return                 The number of bytes written, or -1 on failure. Note
                            that the number of bytes written may be less than
                            what was requested.
*/
ssize_t fs_write(file_t hnd, const void *buffer, size_t cnt);

/** \brief   Seek to a new position within a file.

    This function moves the file pointer to the specified position within the
    file (the base of this position is determined by the whence parameter).

    \param  hnd             The file descriptor to move the pointer for.
    \param  offset          The offset in bytes from the specified base.
    \param  whence          The base of the pointer move. This should be one of
                            the \ref vfs_seek_modes "Seek Modes" values.
    
    \return                 The new position of the file pointer.
*/
off_t fs_seek(file_t hnd, off_t offset, int whence);

/** \brief   Seek to a new position within a file (64-bit offsets).

    This function moves the file pointer to the specified position within the
    file (the base of this position is determined by the whence parameter).

    \param  hnd             The file descriptor to move the pointer for.
    \param  offset          The offset in bytes from the specified base.
    \param  whence          The base of the pointer move. This should be one of
                            the \ref vfs_seek_modes "Seek Modes" values.
    
    \return                 The new position of the file pointer.
*/
_off64_t fs_seek64(file_t hnd, _off64_t offset, int whence);

/** \brief   Retrieve the position of the pointer within a file.

    This function retrieves the current location of the file pointer within an
    opened file. This is an offset in bytes from the start of the file.

    \param  hnd             The file descriptor to retrieve the pointer from.
    
    \return                 The offset within the file for the pointer.
*/
off_t fs_tell(file_t hnd);

/** \brief   Retrieve the position of the 64-bit pointer within a file.

    This function retrieves the current location of the file pointer within an
    opened file. This is an offset in bytes from the start of the file.

    \param  hnd             The file descriptor to retrieve the pointer from.
    
    \return                 The offset within the file for the pointer.
*/
_off64_t fs_tell64(file_t hnd);

/** \brief   Retrieve the length of an opened file.

    This file retrieves the length of the file associated with the given file
    descriptor.

    \note                   size_t is unsigned, so the error return value is not
                            less than 0.

    \param  hnd             The file descriptor to retrieve the size from.
    
    \return                 The length of the file on success, -1 on failure.
*/
size_t fs_total(file_t hnd);

/** \brief   Retrieve the length of an opened file as a 64-bit integer.

    This file retrieves the length of the file associated with the given file
    descriptor.

    \note                   uint64 is unsigned, so the error return value is not
                            less than 0.                            

    \param  hnd             The file descriptor to retrieve the size from.
    
    \return                 The length of the file on success, -1 on failure.
*/
uint64 fs_total64(file_t hnd);


/** \brief   Read an entry from an opened directory.

    This function reads the next entry from the directory specified by the given
    file descriptor.

    \param  hnd             The opened directory's file descriptor.
    
    \return                 The next entry, or NULL on failure.
*/
dirent_t *fs_readdir(file_t hnd);

/** \brief   Execute a device-specific command on a file descriptor.

    The types and formats of the commands are device/filesystem specific, and
    are not documented here. Each filesystem may define any commands that are
    specific to it with its implementation of this function.

    \param  hnd             The file descriptor to use.
    \param  cmd             The command to run.
    \param  ...             Arguments for the command specified.
    
    \return                 -1 on error.
*/
int fs_ioctl(file_t hnd, int cmd, ...);

/** \brief   Rename the specified file to the given filename.

    This function renames the file specified by the first argument to the second
    argument. The two paths should be on the same filesystem.

    \param  fn1             The existing file to rename.
    \param  fn2             The new filename to rename to.
    
    \return                 0 on success, -1 on failure.
*/
int fs_rename(const char *fn1, const char *fn2);

/** \brief   Delete the specified file.

    This function deletes the specified file from the filesystem. This should
    only be used for files, not for directories. For directories, use fs_rmdir()
    instead of this function.

    \param  fn              The path to remove.
    
    \return                 0 on success, -1 on failure.
*/
int fs_unlink(const char *fn);

/** \brief   Change the current working directory of the current thread.

    This function changes the current working directory for the current thread.
    Any relative paths passed into file-related functions will be relative to
    the path that is changed to.

    \param  fn              The path to set as the current working directory.
    
    \return                 0 on success, -1 on failure.
*/
int fs_chdir(const char *fn);

/** \brief   Memory-map a previously opened file.

    This file "maps" the opened file into memory, reading the whole file into a
    buffer, and returning that buffer. The returned buffer should not be freed,
    as it will be freed when the file is closed. Bytes written into the buffer,
    up to the original length of the file, will be written back to the file when
    it is closed, assuming that the file is opened for writing.

    \note                   Some of the filesystems in KallistiOS do not support
                            this operation. If you attempt to use this function
                            on a filesystem that does not support it, the
                            function will return NULL and set errno to EINVAL.

    \param  hnd             The descriptor to memory map.
    
    \return                 The memory mapped buffer, or NULL on failure.
*/
void *fs_mmap(file_t hnd);

/** \brief   Perform an I/O completion on the given file descriptor.

    This function is used with asynchronous I/O to perform an I/O completion on
    the given file descriptor.

    \note                   Most of the filesystems in KallistiOS do not support
                            this operation. If you attempt to use this function
                            on a filesystem that does not support it, the
                            function will return -1 and set errno to EINVAL.

    \param  fd              The descriptor to complete I/O on.
    \param  rv              A buffer to store the size of the I/O in.
    
    \return                 0 on success, -1 on failure.
*/
int fs_complete(file_t fd, ssize_t *rv);

/** \brief   Create a directory.

    This function creates the specified directory, if possible.

    \param  fn              The path of the directory to create.
    \return                 0 on success, -1 on failure.
*/
int fs_mkdir(const char *fn);

/** \brief   Remove a directory by name.

    This function removes the specified directory. The directory shall only be
    removed if it is empty.

    \param  fn              The path of the directory to remove.
    
    \return                 0 on success, -1 on failure.
*/
int fs_rmdir(const char *fn);

/** \brief   Manipulate file control flags.

    This function implements the standard C fcntl function.

    \param  fd              The file descriptor to use.
    \param  cmd             The command to run.
    \param  ...             Arguments for the command specified.
    
    \return                 -1 on error (generally).
*/
int fs_fcntl(file_t fd, int cmd, ...);

/** \brief   Create a hard link.

    This function implements the POSIX function link(), which creates a hard
    link for an existing file.

    \note                   Most filesystems in KallistiOS do not support hard
                            links. If you call this function on a filesystem
                            that does not support hard links, the function will
                            return -1 and set errno to EMLINK.

    \param  path1           An existing file to create a new link to.
    \param  path2           The pathname of the new link to be created.
    
    \return                 0 on success, -1 on failure.
*/
int fs_link(const char *path1, const char *path2);

/** \brief   Create a symbolic link.

    This function implements the POSIX function symlink(), which creates a
    symbolic link on the filesystem. Symbolic links are not required to point to
    an existing file (per POSIX) and may result in circular links if care is not
    taken. For now, symbolic links cannot cross filesystem boundaries in KOS.

    \note                   Most filesystems in KallistiOS do not support
                            symbolic links. Filesystems that do not support
                            symlinks will simply set errno to ENOSYS and return
                            -1.

    \param  path1           The content of the link (i.e, what to point at).
    \param  path2           The pathname of the new link to be created.
    
    \return                 0 on success, -1 on failure.
*/
int fs_symlink(const char *path1, const char *path2);

/** \brief   Read the value of a symbolic link.

    This function implements the POSIX function readlink(), which simply reads
    the value of the symbolic link at the end of a path. This does not resolve
    any internal links and it does not canonicalize the path either.

    \note                   Most filesystems in KallistiOS do not support
                            symbolic links. Filesystems that do not support
                            symlinks will simply set errno to ENOSYS and return
                            -1.

    \param  path            The symbolic link to read.
    \param  buf             The buffer to place the link's contents in.
    \param  bufsize         The number of bytes allocated to buf.
    
    \return                 -1 on failure, the number of bytes placed into buf
                            on success. If the return value is equal to bufsize,
                            you may not have the whole link -- provide a larger
                            buffer and try again.
*/
ssize_t fs_readlink(const char *path, char *buf, size_t bufsize);

/** \brief   Retrieve information about the specified path.

    This function retrieves status information on the given path. This function
    now returns the normal POSIX-style struct stat, rather than the old KOS
    stat_t structure. In addition, you can specify whether or not this function
    should resolve symbolic links on filesystems that support symlinks.

    \param  path            The path to retrieve information about.
    \param  buf             The buffer to store stat information in.
    \param  flag            Specifies whether or not to resolve a symbolic link.
                            If you don't want to resolve any symbolic links at
                            the end of the path, pass AT_SYMLINK_NOFOLLOW,
                            otherwise pass 0.
    
    \return                 0 on success, -1 on failure.
*/
int fs_stat(const char *path, struct stat *buf, int flag);

/** \brief   Rewind a directory to the start.

    This function rewinds the position of a directory stream to the beginning of
    the directory.

    \note                   Some filesystems may not support this function. If a
                            filesystem doesn't support it, errno will be set to
                            ENOSYS and -1 will be returned.

    \param  hnd             The opened directory's file descriptor.
    
    \return                 0 on success, -1 on failure.
*/
int fs_rewinddir(file_t hnd);

/** \brief   Retrieve information about an opened file.

    This function retrieves status information on the given file descriptor,
    which must correspond to an already opened file.

    \note                   Some filesystems may not support this function. If a
                            filesystem doesn't support it, errno will be set to
                            ENOSYS and -1 will be returned.

    \param  hnd             The file descriptor to retrieve information about.
    \param  buf             The buffer to store stat information in.
    
    \return                 0 on success, -1 on failure.
*/
int fs_fstat(file_t hnd, struct stat *buf);

/** \brief   Duplicate a file descriptor.

    This function duplicates the specified file descriptor, returning a new file
    descriptor that can be used to access the file. This is equivalent to the
    standard POSIX function dup().

    \param  oldfd           The old file descriptor to duplicate.
    
    \return                 The new file descriptor on success, -1 on failure.
*/
file_t fs_dup(file_t oldfd);

/** \brief   Duplicate a file descriptor onto the specified descriptor.

    This function duplicates the specified file descriptor onto the other file
    descriptor provided. If the newfd parameter represents an open file, that
    file will be closed before the old descriptor is duplicated onto it. This is
    equivalent to the standard POSIX function dup2().

    \param  oldfd           The old file descriptor to duplicate.
    \param  newfd           The descriptor to copy into.
    
    \return                 The new file descriptor on success, -1 on failure.
*/
file_t fs_dup2(file_t oldfd, file_t newfd);

/** \brief   Create a "transient" file descriptor.

    This function creates and opens a new file descriptor that isn't associated
    directly with a file on the filesystem. This is used internally to actually
    open files, and should (in general) not be called by user code. Effectively,
    if you're trying to implement your own filesystem handler in your code, you
    may need this function, otherwise you should just ignore it.

    \param  vfs             The VFS handler structure to use for the file.
    \param  hnd             Internal handle data for the file.
    
    \return                 The opened descriptor on success, -1 on failure.
*/
file_t fs_open_handle(vfs_handler_t *vfs, void *hnd);

/** \brief   Retrieve the VFS Handler for a file descriptor.

    This function retrieves the Handler structure for the VFS of the specified
    file descriptor. There is generally no reason to call this function in user
    code, as it is meant for use internally.

    \param  fd              The file descriptor to retrieve the handler for.
    
    \return                 The VFS' handler structure.
*/
vfs_handler_t *fs_get_handler(file_t fd);

/** \brief   Retrieve the internal handle for a file descriptor.

    This function retrieves the internal file handle data of the specified file
    descriptor. There is generally no reason to call this function in user code,
    as it is meant for use internally.

    \param  fd              The file descriptor to retrieve the handler for.
    
    \return                 The internal handle for the file descriptor.
*/
void *fs_get_handle(file_t fd);

/** \brief   Get the current working directory of the running thread.

    \return                 The current working directory.
*/
const char *fs_getwd(void);

/* Couple of util functions */

/** \brief   Copy a file.

    This function copies the file at src to dst on the filesystem.

    \param  src             The filename to copy from.
    \param  dst             The filename to copy to.
    
    \return                 The number of bytes copied successfully.
*/
ssize_t fs_copy(const char *src, const char *dst);

/** \brief   Open and read a whole file into RAM.

    This function opens the specified file, reads it into memory (allocating the
    necessary space with malloc), and closes the file. The caller is responsible
    for freeing the memory when they are done with it.

    \param  src             The filename to open and read.
    \param  out_ptr         A pointer to the buffer on success, NULL otherwise.
    
    \return                 The size of the file on success, -1 otherwise.
*/
ssize_t fs_load(const char *src, void **out_ptr);

/** \brief   Append a path component to a string.

    This function acts mostly like the function strncat(), with a few slight
    differences. First, if the destination string doesn't end in a '/'
    character, this function will add it. Second, it returns the length of the
    resulting string, including the NUL terminator. Finally, no modification of
    the destination string will occur if there isn't enough space left in the
    string to do so.

    \param  dst             The string to modify.
    \param  src             The path component to append.
    \param  len             The length allocated for dst.
    
    \return                 The length of the new string (including the NUL
                            terminator) on success, -1 otherwise.

    \par    Error Conditions:
    \em     EFAULT - src or dst is a NULL pointer \n
    \em     EINVAL - len is zero \n
    \em     ENAMETOOLONG - the resulting path would be longer than len bytes \n
*/
ssize_t fs_path_append(char *dst, const char *src, size_t len);

/** \brief   Normalize the specified path. 
    This function acts mostly like the function realpath() but it only simplifies
    a path by resolving . and .. components and removing redundant slashes.  It 
    doesn't check if the path exists or resolve symbolic links.
    \param  path            The path to normalize.
    \param  resolved        The buffer to store resolved normalized path. It has 
                            to be PATH_MAX bytes in size.
    
    \return                 A pointer to the normalized path on success, 
                            or NULL on failure, in which case the path which 
                            caused trouble is left in resolved.
    \par    Error Conditions:
    \em     EINVAL - path or resolved is a NULL pointer \n
    \em     ENAMETOOLONG - the resulting path would be longer than PATH_MAX bytes \n
*/
char *fs_normalize_path(const char *__RESTRICT path, char *__RESTRICT resolved);

/** \brief   Initialize the virtual filesystem.

    This is normally done for you by default when KOS starts. In general, there
    should be no reason for you to call this function.

    \retval 0               On success.
*/
int fs_init(void);

/** \brief   Shut down the virtual filesystem.

    This is done for you by the normal shutdown procedure of KOS. There should
    not really be any reason for you to call this function yourself.
*/
void fs_shutdown(void);

/** @} */

__END_DECLS

#endif  /* __KOS_FS_H */
