/* KallistiOS ##version##

   dc/fs_vmu.h
   (c)2000-2001 Jordan DeLong

*/

/** \file    dc/fs_vmu.h
    \brief   VMU filesystem driver.
    \ingroup vfs_vmu

    The VMU filesystem driver mounts itself on /vmu of the VFS. Each memory card
    has its own subdirectory off of that directory (i.e, /vmu/a1 for slot 1 of
    the first controller). VMUs themselves have no subdirectories, so the driver
    itself is fairly simple.

    Files on a VMU must be multiples of 512 bytes in size, and should have a
    header attached so that they show up in the BIOS menu.

    This layer is built off of the vmufs layer, which does all the low-level
    operations. It is generally easier to work with things at this level though,
    so that you can use the normal libc file access functions.

    \author Megan Potter

    \see    dc/vmu_pkg.h
    \see    dc/vmufs.h
*/

#ifndef __DC_FS_VMU_H
#define __DC_FS_VMU_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>
#include <dc/vmu_pkg.h>

/** \defgroup vfs_vmu   VMU
    \brief              VFS driver for accessing Visual Memory Unit storage
    \ingroup            vfs

    @{
*/

/* \cond */
/* Initialization */
int fs_vmu_init(void);
int fs_vmu_shutdown(void);

#define IOCTL_VMU_SET_HDR          0x564d5530 /* "VMU0" */
#define IOCTL_VMU_GET_HDR_STATE    0x564d5531 /* "VMU1" */
/* \endcond */

/** \defgroup file_hdr_status      File header parse status
    \brief                         Values returned from IOCTL_VMU_GET_HDR_STATE
    \ingroup                       VMU

    When O_META flag is not used, fs_open() function will parse the VMU file header.

    If successful the header is hidden, so any operation in the VMU file won't see the header.
    However, if it fails, the entire file is available in the same way as with O_META (raw access).

    The codes returned by IOCTL_VMU_GET_HDR_STATE indicate status of that parse.

    @{
*/
#define FSVMU_HEADEROK     0   /**< \brief Header parsed correctly */
#define FSVMU_BADHEADER    1   /**< \brief Header is missing, corrupted or bad CRC */
#define FSVMU_NOFILE       2   /**< \brief File was missing, no header parsed. Not an error per-se */
#define FSVMU_READERROR    3   /**< \brief Error before read, while reading or truncated header data */
/** @} */

/** \brief  Set a header to an opened VMU file

    This function can be used to set a specific header (which contains the
    metadata, icons...) to an opened VMU file, replacing the one it previously
    had (if any).

    Note that the "pkg" pointer as well as the eyecatch/icon data pointers it
    contain can be freed (if dynamically allocated) as soon as this function
    return, as the filesystem code will keep an internal copy.

    It is valid to pass NULL as the header pointer, in which case the header
    for the file will be discarded.

    \param fd               A file descriptor corresponding to the VMU file
    \param pkg              The header to set to the VMU file
    \retval 0               On success.
    \retval -1              On error.
*/
static inline int fs_vmu_set_header(file_t fd, const vmu_pkg_t *pkg) {
    return fs_ioctl(fd, IOCTL_VMU_SET_HDR, pkg);
}

/** \brief  Retrieves the header parse status from an opened VMU file

    This function can be used to check if the header was parsed correctly (which contains the
    metadata, icons...) from an opened VMU file.

    Note: always returns FSVMU_HEADEROK if the file was truncated.

    \see file_hdr_status
    \param fd               A file descriptor corresponding to the VMU file
    \return                 0 on success, otherwise, the error code
*/
static inline int fs_vmu_get_header_parse_status(file_t fd) {
    return fs_ioctl(fd, IOCTL_VMU_GET_HDR_STATE);
}

/** \brief  Set a default header for newly created VMU files

    This function will set a default header, that will be used for new files
    which were not assignated their own header.

    Note that files which already have a header, either because they were opened
    read-write or because they were given one, will use their own header instead
    of the default one.

    Note that the "pkg" pointer as well as the eyecatch/icon data pointers it
    contain can be freed (if dynamically allocated) as soon as this function
    return, as the filesystem code will keep an internal copy.

    It is valid to pass NULL as the header pointer, in which case the default
    header will be discarded.

    \param pkg              The header to set to the VMU file
    \retval 0               On success.
    \retval -1              On error.
*/
static inline int fs_vmu_set_default_header(const vmu_pkg_t *pkg) {
    file_t fd;
    int ret;

    fd = fs_open("/vmu", O_RDONLY | O_DIR);
    if(!fd)
        return -1;

    ret = fs_vmu_set_header(fd, pkg);
    fs_close(fd);

    return ret;
}

/** @} */


__END_DECLS

#endif  /* __DC_FS_VMU_H */

