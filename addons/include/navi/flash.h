/* KallistiOS ##version##

   navi/flash.h
   Copyright (C) 2002 Megan Potter

*/

/** \file   navi/flash.h
    \brief  BIOS replacement flashrom support.

    This file is involved with accessing an flashrom chip soldered in place to
    replace the BIOS ROM. Specifically, this driver is for accessing a STMicro
    M29W800B, but should also work with other chips like the M29W160B or
    equivalent AMD chips.

    \author Megan Potter
*/

#ifndef __NAVI_FLASH_H
#define __NAVI_FLASH_H

#include <stdint.h>

/** \brief  Try to detect a compatible flashrom.
    \return                 0 if a compatible flashrom is detected, <0 if the
                            normal Dreamcast BIOS is detected.
*/
int nvflash_detect(void);

/** \brief  Erase a single block of flashrom.
    \param  addr            The block of the flashrom to erase.
    \return                 0 on success, <0 on error.
*/
int nvflash_erase_block(uint32_t addr);

/** \brief  Write data to the flashrom.
    \param  addr            The block of the flashrom to write to.
    \param  data            The data to write.
    \param  len             The length of the data, in bytes.
    \return                 0 on success, <0 on error.
*/
int nvflash_write_block(uint32_t addr, void *data, uint32_t len);

/* Erase the whole flash chip */
/** \brief  Erase the whole flashrom.
    \return                 0 on success, <0 on error.
*/
int nvflash_erase_all(void);

#endif  /* __NAVI_FLASH_H */
