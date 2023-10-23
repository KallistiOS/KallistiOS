/* KallistiOS ##version##

   dc/sound/pcm_split.h
   Copyright (C) 2023 Ruslan Rostovtsev

*/

/** \file   dc/sound/pcm_split.h
    \brief  Separating stereo PCM 16-bit to single channels

    \author Ruslan Rostovtsev
*/

#ifndef __DC_SOUND_PCM_SPLIT_H
#define __DC_SOUND_PCM_SPLIT_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <stdint.h>


/** \brief  Separates stereo PCM samples into 2 mono channels.


    Splits a buffer containing 2 interleaved channels of 16-bit PCM samples
    into 2 separate buffers of 16-bit PCM samples.

    \warning 
    All arguments must be 32-byte aligned.
    
    \param data   Source buffer of interleaved stereo samples
    \param left   Destination buffer for left mono samples
    \param right  Destination buffer for right mono samples
    \param size   Size of the source buffer in bytes (must be divisible by 32)
    
    \sa snd_pcm16_split_sq()
*/
void snd_pcm16_split(uint32_t *data, uint32_t *left, uint32_t *right, size_t size);

/** \brief  Separates stereo PCM samples into 2 mono channels with SQ transfer.

    Splits a buffer containing 2 interleaved channels of 16-bit PCM samples
    into 2 separate buffers of 16-bit PCM samples by using the store queues
    for data transfer.
    
    \warning 
    All arguments must be 32-byte aligned.
    
    \warning 
    The store queues must be configured for transferring to the left and right
    destination buffers beforehand (QACRO <= left, QACRO1 <= right).
    
    \param data   Source buffer of interleaved stereo samples
    \param left   SQ-masked left destination buffer address
    \param right  SQ-masked right destination buffer address
    \param size   Size of the source buffer in bytes (must be divisible by 32)
    
    \sa snd_pcm16_split()
    Store queues must be prepared before.
*/
void snd_pcm16_split_sq(uint32_t *data, uintptr_t left, uintptr_t right, size_t size);


__END_DECLS

#endif  /* __DC_SOUND_PCM_SPLIT_H */
