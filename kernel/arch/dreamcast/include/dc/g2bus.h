/* KallistiOS ##version##

   g2bus.h
   (c)2002 Megan Potter

*/

/** \file   dc/g2bus.h
    \brief  G2 bus memory interface.

    This file provides low-level support for accessing devices on the G2 bus in
    the Dreamcast. The G2 bus contains the AICA, as well as the expansion port.
    Generally, you won't be dealing with things at this level, but rather on the
    level of the device you're actually interested in working with. Most of the
    expansion port devices (the modem, bba, and lan adapter) all have their own
    drivers that work off of this functionality.

    The G2 bus is notoroiously picky about a lot of things. You have to be
    careful to use the right access size for whatever you're working with. Also
    you can't be doing PIO and DMA at the same time. Finally, there's a FIFO to
    contend with when you're doing PIO stuff as well. Generally, G2 is a pain in
    the rear, so you really do want to be using the higher-level stuff related
    to each device if at all possible!

    \author Megan Potter
*/

#ifndef __DC_G2BUS_H
#define __DC_G2BUS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* DMA copy from SH-4 RAM to G2 bus (dir = 0) or the opposite;
   length must be a multiple of 32,
   and the source and destination addresses must be aligned on 32-byte
   boundaries. If block is non-zero, this function won't return until
   the transfer is complete. If callback is non-NULL, it will be called
   upon completion (in an interrupt context!). Returns <0 on error.

   Known working combination :

   g2chn = 0, sh4chn = 3 --> mode = 5 (but many other value seems OK ?)
   g2chn = 1, sh4chn = 1 --> mode = 0 (or 4 better ?)
   g2chn = 1, sh4chn = 0 --> mode = 3

   It seems that g2chn is not important when choosing mode, so this mode parameter is probably
   how we actually connect the sh4chn to the g2chn.

   Update : looks like there is a formula, mode = 3 + shchn

*/

/* We use sh channel 3 here to avoid conflicts with the PVR. */
#define SPU_DMA_MODE   6 /* should we use 6 instead, so that the formula is 3+shchn ?
6 works too, so ... */
#define SPU_DMA_G2CHN  0
#define SPU_DMA_SHCHN  3

/* For BBA : sh channel 1 (doesn't seem used) and g2 channel 1 to no conflict with SPU */
#define BBA_DMA_MODE   4
#define BBA_DMA_G2CHN  1
#define BBA_DMA_SHCHN  1

/* For BBA2 : sh channel 0 (doesn't seem used) and g2 channel 2 to no conflict with SPU */
/* This is a second DMA channels used for the BBA, just for fun and see if we can initiate
   two DMA transfers with the BBA concurently. */
#define BBA_DMA2_MODE   3
#define BBA_DMA2_G2CHN  2
#define BBA_DMA2_SHCHN  0

typedef void (*g2_dma_callback_t)(ptr_t data);
int g2_dma_transfer(void *from, void * dest, uint32 length, int block,
                    g2_dma_callback_t callback, ptr_t cbdata,
                    uint32 dir, uint32 mode, uint32 g2chn, uint32 sh4chn);

/** \brief  Read one byte from G2.

    This function reads a single byte from the specified address, taking all
    necessary precautions that are required for accessing G2.

    \param  address         The address in memory to read.
    \return                 The byte read from the address specified.
*/
uint8 g2_read_8(uint32 address);

/** \brief  Write a single byte to G2.

    This function writes one byte to the specified address, taking all the
    necessary precautions to ensure your write actually succeeds.

    \param  address         The address in memory to write to.
    \param  value           The value to write to that address.
*/
void g2_write_8(uint32 address, uint8 value);

/** \brief  Read one 16-bit word from G2.

    This function reads a single word from the specified address, taking all
    necessary precautions that are required for accessing G2.

    \param  address         The address in memory to read.
    \return                 The word read from the address specified.
*/
uint16 g2_read_16(uint32 address);

/** \brief  Write a 16-bit word to G2.

    This function writes one word to the specified address, taking all the
    necessary precautions to ensure your write actually succeeds.

    \param  address         The address in memory to write to.
    \param  value           The value to write to that address.
*/
void g2_write_16(uint32 address, uint16 value);

/** \brief  Read one 32-bit dword from G2.

    This function reads a single dword from the specified address, taking all
    necessary precautions that are required for accessing G2.

    \param  address         The address in memory to read.
    \return                 The dword read from the address specified.
*/
uint32 g2_read_32(uint32 address);

/** \brief  Write a 32-bit dword to G2.

    This function writes one dword to the specified address, taking all the
    necessary precautions to ensure your write actually succeeds.

    \param  address         The address in memory to write to.
    \param  value           The value to write to that address.
*/
void g2_write_32(uint32 address, uint32 value);

/** \brief  Read a block of bytes from G2.

    This function acts as memcpy() for copying data from G2 to system memory. It
    will take the necessary precautions before accessing G2 for you as well.

    \param  output          Pointer in system memory to write to.
    \param  address         The address in G2-space to read from.
    \param  amt             The number of bytes to read.
*/
void g2_read_block_8(uint8 * output, uint32 address, int amt);

/** \brief  Write a block of bytes to G2.

    This function acts as memcpy() for copying data to G2 from system memory. It
    will take the necessary precautions for accessing G2.

    \param  input           The pointer in system memory to read from.
    \param  address         The address in G2-space to write to.
    \param  amt             The number of bytes to write.
*/
void g2_write_block_8(const uint8 * input, uint32 address, int amt);

/** \brief  Read a block of words from G2.

    This function acts as memcpy() for copying data from G2 to system memory,
    but it copies 16 bits at a time. It will take the necessary precautions
    before accessing G2 for you as well.

    \param  output          Pointer in system memory to write to.
    \param  address         The address in G2-space to read from.
    \param  amt             The number of words to read.
*/
void g2_read_block_16(uint16 * output, uint32 address, int amt);

/** \brief  Write a block of words to G2.

    This function acts as memcpy() for copying data to G2 from system memory,
    copying 16 bits at a time. It will take the necessary precautions for
    accessing G2.

    \param  input           The pointer in system memory to read from.
    \param  address         The address in G2-space to write to.
    \param  amt             The number of words to write.
*/
void g2_write_block_16(const uint16 * input, uint32 address, int amt);

/** \brief  Read a block of dwords from G2.

    This function acts as memcpy() for copying data from G2 to system memory,
    but it copies 32 bits at a time. It will take the necessary precautions
    before accessing G2 for you as well.

    \param  output          Pointer in system memory to write to.
    \param  address         The address in G2-space to read from.
    \param  amt             The number of dwords to read.
*/
void g2_read_block_32(uint32 * output, uint32 address, int amt);

/** \brief  Write a block of dwords to G2.

    This function acts as memcpy() for copying data to G2 from system memory,
    copying 32 bits at a time. It will take the necessary precautions for
    accessing G2.

    \param  input           The pointer in system memory to read from.
    \param  address         The address in G2-space to write to.
    \param  amt             The number of dwords to write.
*/
void g2_write_block_32(const uint32 * input, uint32 address, int amt);

/** \brief  Set a block of bytes to G2.

    This function acts as memset() for setting a block of bytes on G2. It will
    take the necessary precautions for accessing G2.

    \param  address         The address in G2-space to write to.
    \param  c               The byte to write.
    \param  amt             The number of bytes to write.
*/
void g2_memset_8(uint32 address, uint8 c, int amt);

/** \brief  Wait for the G2 write FIFO to empty.

    This function will spinwait until the G2 FIFO indicates that it has been
    drained. The FIFO is 32 bytes in length, and thus when accessing AICA you
    must do this at least for every 8 32-bit writes that you execute.
*/
void g2_fifo_wait();

__END_DECLS

#endif  /* __DC_G2BUS_H */

