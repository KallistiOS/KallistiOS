/* KallistiOS ##version##

   dc/vmu_pkg.h
   Copyright (C) 2002 Megan Potter

*/

/** \file    dc/vmu_pkg.h
    \brief   VMU Packaging functionality.
    \ingroup vmu_package

    This file provides declarations for managing the headers that must be
    attached to VMU files for the BIOS to pay attention to them. This does not
    handle reading/writing files directly.

    \author Megan Potter
    \see    dc/fs_vmu.h
*/

#ifndef __DC_VMU_PKG_H
#define __DC_VMU_PKG_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup   vmu_package     Header Package
    \brief                      API for Managing VMU File Headers
    \ingroup                    vmu

    This API is provided as a utility for easy management of VMS file headers.
    These headers must be present on every file saved within the VMU's
    filesystem for both the Dreamcast and VMU's BIOS to detect them properly.
*/

/** \brief   VMU Package type.
    \ingroup vmu_package

    Anyone wanting to package a VMU file should create one of these somewhere;
    eventually it will be turned into a flat file that you can save using
    fs_vmu.

    \headerfile dc/vmu_pkg.h
*/
typedef struct vmu_pkg {
    char        desc_short[20];     /**< \brief Short file description */
    char        desc_long[36];      /**< \brief Long file description */
    char        app_id[20];         /**< \brief Application ID */
    int         icon_cnt;           /**< \brief Number of icons */
    int         icon_anim_speed;    /**< \brief Icon animation speed */
    int         eyecatch_type;      /**< \brief "Eyecatch" type */
    int         data_len;           /**< \brief Number of data (payload) bytes */
    uint16      icon_pal[16];       /**< \brief Icon palette (ARGB4444) */
    uint8       *icon_data;         /**< \brief 512*n bytes of icon data */
    const uint8 *eyecatch_data;     /**< \brief Eyecatch data */
    const uint8 *data;              /**< \brief Payload data */
} vmu_pkg_t;

/** \brief   Final VMU package type.
    \ingroup vmu_package

    This structure will be written into the file itself, not vmu_pkg_t.

    \headerfile dc/vmu_pkg.h
*/
typedef struct vmu_hdr {
    char    desc_short[16];     /**< \brief Space-padded short description */
    char    desc_long[32];      /**< \brief Space-padded long description*/
    char    app_id[16];         /**< \brief Null-padded application ID */
    uint16  icon_cnt;           /**< \brief Number of icons */
    uint16  icon_anim_speed;    /**< \brief Icon animation speed */
    uint16  eyecatch_type;      /**< \brief Eyecatch type */
    uint16  crc;                /**< \brief CRC of the file */
    uint32  data_len;           /**< \brief Payload size */
    uint8   reserved[20];       /**< \brief Reserved (all zero) */
    uint16  icon_pal[16];       /**< \brief Icon palette (ARGB4444) */
    /* 512*n Icon Bitmaps */
    /* Eyecatch palette + bitmap */
} vmu_hdr_t;

/** \defgroup vmu_ectype    Eyecatch Types
    \brief                  Values for various VMU eyecatch formats
    \ingroup                vmu_package

    All eyecatches are 72x56, but the pixel format is variable. Note that in all
    of the cases which use a palette, the palette entries are in ARGB4444 format
    and come directly before the pixel data itself.

    @{
*/
#define VMUPKG_EC_NONE      0   /**< \brief No eyecatch */
#define VMUPKG_EC_16BIT     1   /**< \brief 16-bit ARGB4444 */
#define VMUPKG_EC_256COL    2   /**< \brief 256-color palette */
#define VMUPKG_EC_16COL     3   /**< \brief 16-color palette */
/** @} */

/** \brief   Convert a vmu_pkg_t into an array of uint8s.
    \ingroup vmu_package

    This function converts a vmu_pkg_t structure into an array of uint8's which
    may be written to a VMU file via fs_vmu, or whatever.

    \param  src             The vmu_pkg_t to convert.
    \param  dst             The buffer (will be allocated for you).
    \param  dst_size        The size of the output.
    \return                 0 on success, <0 on failure.
*/
int vmu_pkg_build(vmu_pkg_t *src, uint8 ** dst, int * dst_size);

/** \brief   Parse an array of uint8s into a vmu_pkg_t.
    \ingroup vmu_package

    This function does the opposite of vmu_pkg_build and is used to parse VMU
    files read in.

    \param  data            The buffer to parse.
    \param  pkg             Where to store the vmu_pkg_t.
    \retval -1              On invalid CRC in the data.
    \retval 0               On success.
*/
int vmu_pkg_parse(uint8 *data, vmu_pkg_t *pkg);

/** \brief   Load a .ico file to use as a VMU file's icon.
    \ingroup vmu_package

    Icon files must be in the ICO file format, be 32x32 in size, contain a
    bitmap (no PNG or compressed BMP), and use paletted 4bpp.
    They can contain more than one frame, and they can use up to 16 colors
    if transparency is not used, or 15 colors otherwise. Finally, all frames
    must use the same palette.

    This function assumes that the vmu_pkg_t has been properly initialized;
    in particular, the .icon_cnt must be set, and the .icon_data must point to
    a valid buffer (of 512 bytes per frame).

    If the .ico file contains more frames than requested, only the first ones
    are loaded. If it contains less frames than requested, the .icon_cnt field
    will be updated to the new frame count.

    \param  pkg             A pointer a pre-initialized vmu_pkg_t
    \param  icon_fn         The file path to the .ico file
    \retval -1              If the .ico file cannot be loaded.
    \retval 0               On success.
*/
int vmu_pkg_load_icon(vmu_pkg_t *pkg, const char *icon_fn);

__END_DECLS

#endif  /* __DC_VMU_PKG_H */

