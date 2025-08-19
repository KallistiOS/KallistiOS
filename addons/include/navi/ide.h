/* KallistiOS ##version##

   navi/ide.h
   Copyright (C) 2002 Megan Potter

*/

/** \file   navi/ide.h
    \brief  External G2 Bus-based IDE support.

    This file is involved with accessing an IDE controller that is attached to
    the G2 Bus expansion port. Exact details of how to build such a device have
    been posted in various places around the Internet. This driver refers to the
    device built by Megan as a part of the Navi project.

    \author Megan Potter
*/

#ifndef __NAVI_IDE_H
#define __NAVI_IDE_H

#include <stdint.h>

/** \brief  Read sectors from the hard disk via PIO.
    \param  linear          The address to begin reading from.
    \param  numsects        The number of sectors to read.
    \param  bufptr          The buffer to read into.
    \return                 0 on success, <0 on error.
*/
int ide_read(uint32_t linear, uint32_t numsects, void *bufptr);

/** \brief  Write sectors from the hard disk via PIO.
    \param  linear          The address to begin writing to.
    \param  numsects        The number of sectors to write.
    \param  bufptr          The buffer to write out of.
    \return                 0 on success, <0 on error.
*/
int ide_write(uint32_t linear, uint32_t numsects, void *bufptr);

/** \brief  Retrieve the number of sectors from the hard disk.
    \returns                The total number of linear sectors.
*/
uint32_t ide_num_sectors(void);

/** \brief  Initialize Navi IDE.
    \return                 0 on success (no error conditions defined).
*/
int ide_init(void);

/** \brief  Shutdown Navi IDE. */
void ide_shutdown(void);

#endif  /* __NAVI_IDE_H */
