/* KallistiOS ##version##

   fifo.h
   Copyright (C) 2023 Andy Barajas

*/

/** \file    dc/fifo.h
    \brief   Macros to assess FIFO status.
    \ingroup system_fifo

    This header provides a set of macros to facilitate checking
    the status of various FIFOs on the system.

    \author Andy Barajas
*/

#ifndef __DC_FIFO_H
#define __DC_FIFO_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup system_fifo   FIFO
    \brief                  API for checking FIFO statuses
    \ingroup                system

    @{
*/

/** \brief Address of the FIFO status register. 
    Accessing this value provides the current status of all FIFOs.

*/
#define FIFO_STATUS     (*(volatile uint32_t const *)0xa05f688c)

/** \name        FIFO Status Indicators
 
    \note 
    To determine the empty status of a specific FIFO, AND the desired FIFO 
    status mask with the value returned by FIFO_STATUS.
    
    If the resulting value is non-zero, the FIFO is not empty. Otherwise, 
    it is empty.

    @{
*/

#define FIFO_AICA   (1 << 0)   /** \brief AICA FIFO status mask. */
#define FIFO_BBA    (1 << 1)   /** \brief BBA FIFO status mask. */
#define FIFO_EXT2   (1 << 2)   /** \brief EXT2 FIFO status mask. */
#define FIFO_EXTDEV (1 << 3)   /** \brief EXTDEV FIFO status mask. */
#define FIFO_G2     (1 << 4)   /** \brief G2 FIFO status mask. */
#define FIFO_SH4    (1 << 5)   /** \brief SH4 FIFO status mask. */

/** @} */

/** @} */

__END_DECLS

#endif  /* __DC_FIFO_H */

