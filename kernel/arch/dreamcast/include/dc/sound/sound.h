/* KallistiOS ##version##

   dc/sound/sound.h
   Copyright (C) 2002 Megan Potter

*/

/** \file   dc/sound/sound.h
    \brief  Low-level sound support and memory management.

    This file contains declarations for low-level sound operations and for SPU
    RAM pool memory management. Most of the time you'll be better off using the
    higher-level functionality in the sound effect support or streaming support,
    but this stuff can be very useful for some things.

    \author Megan Potter
*/

#ifndef __DC_SOUND_SOUND_H
#define __DC_SOUND_SOUND_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \brief  Allocate memory in the SPU RAM pool

    This function acts as the memory allocator for the SPU RAM pool. It acts
    much like one would expect a malloc() function to act, although it does not
    return a pointer directly, but rather an offset in SPU RAM.

    \param  size            The amount of memory to allocate, in bytes.
    \return                 The location of the start of the block on success,
                            or 0 on failure.
*/
uint32 snd_mem_malloc(size_t size);

/** \brief  Free a block of allocated memory in the SPU RAM pool.

    This function frees memory previously allocated with snd_mem_malloc().

    \param  addr            The location of the start of the block to free.
*/
void snd_mem_free(uint32 addr);

/** \brief  Get the size of the largest allocateable block in the SPU RAM pool.

    This function returns the largest size that can be currently passed to
    snd_mem_malloc() and expected to not return failure. There may be more
    memory available in the pool, especially if multiple blocks have been
    allocated and freed, but calls to snd_mem_malloc() for larger blocks will
    return failure, since the memory is not available contiguously.

    \return                 The size of the largest available block of memory in
                            the SPU RAM pool.
*/
uint32 snd_mem_available();

/** \brief  Reinitialize the SPU RAM pool.

    This function reinitializes the SPU RAM pool with the given base offset
    within the memory space. There is generally not a good reason to do this in
    your own code, but the functionality is there if needed.

    \param  reserve         The amount of memory to reserve as a base.
    \retval 0               On success (no failure conditions defined).
*/
int snd_mem_init(uint32 reserve);

/** \brief  Shutdown the SPU RAM allocator.

    There is generally no reason to be calling this function in your own code,
    as doing so will cause problems if you try to allocate SPU memory without
    calling snd_mem_init() afterwards.
*/
void snd_mem_shutdown();

/** \brief  Initialize the sound system.

    This function reinitializes the whole sound system. It will not do anything
    unless the sound system has been shut down previously or has not been
    initialized yet. This will implicitly replace the program running on the
    AICA's ARM processor when it actually initializes anything. The default
    snd_stream_drv will be loaded if a new program is uploaded to the SPU.
*/
int snd_init();

/** \brief  Shut down the sound system.

    This function shuts down the whole sound system, freeing memory and
    disabling the SPU in the process. There's not generally many good reasons
    for doing this in your own code.
*/
void snd_shutdown();

/** \brief  Copy a request packet to the AICA queue.

    This function is to put in a low-level request using the built-in streaming
    sound driver.

    \param  packet          The packet of data to copy.
    \param  size            The size of the packet, in 32-bit increments.
    \retval 0               On success (no error conditions defined).
*/
int snd_sh4_to_aica(void *packet, uint32 size);

/** \brief  Begin processing AICA queue requests.

    This function begins processing of any queued requests in the AICA queue.
*/
void snd_sh4_to_aica_start();

/** \brief  Stop processing AICA queue requests.

    This function stops the processing of any queued requests in the AICA queue.
*/
void snd_sh4_to_aica_stop();

/** \brief  Transfer a packet of data from the AICA's SH4 queue.

    This function is used to retrieve a packet of data from the AICA back to the
    SH4. The buffer passed in should at least contain 1024 bytes of space to
    make sure any packet can fit.

    \param  packetout       The buffer to store the retrieved packet in.
    \retval -1              On failure. Failure probably indicates the queue has
                            been corrupted, and thus should be reinitialized.
    \retval 0               If no packets are available.
    \retval 1               On successful copy of one packet.
*/
int snd_aica_to_sh4(void *packetout);

/** \brief  Poll for a response from the AICA.

    This function waits for the AICA to respond to a previously sent request.
    This function is not safe to call in an IRQ, as it does implicitly wait.
*/
void snd_poll_resp();

__END_DECLS

#endif  /* __DC_SOUND_SOUND_H */

