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


/** \brief  Separetes stereo PCM samples to 2 mono channel.

    All arguments must be 32-byte aligned.
*/
void snd_pcm16_split(uint32_t *data, uint32_t *left, uint32_t *right, uint32_t size);

/** \brief  Separetes stereo PCM samples to 2 mono channel with SQ transfer.

    All arguments must be 32-byte aligned.
    Store queues must be prepared before.
*/
void snd_pcm16_split_sq(uint32_t *data, uint32_t left, uint32_t right, uint32_t size);


__END_DECLS

#endif  /* __DC_SOUND_PCM_SPLIT_H */
