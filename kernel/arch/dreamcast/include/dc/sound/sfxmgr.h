/* KallistiOS ##version##

   dc/sound/sfxmgr.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2023, 2024 Ruslan Rostovtsev
   Copyright (C) 2023 Andy Barajas

*/

/** \file    dc/sound/sfxmgr.h
    \brief   Basic sound effect support.
    \ingroup audio_sfx

    This file contains declarations for doing simple sound effects. This code is
    only usable for simple WAV files containing either 8-bit or 16-bit samples (stereo
    or mono) or Yamaha ADPCM (4-bits, stereo or mono). Also, all sounds played in
    this manner must be at most 65534 samples in length, as this does not handle
    buffer chaining or anything else complex. For more interesting stuff, you
    should probably look at the sound stream stuff instead.

    \author Megan Potter
    \author Ruslan Rostovtsev
    \author Andy Barajas
*/

#ifndef __DC_SOUND_SFXMGR_H
#define __DC_SOUND_SFXMGR_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>
#include <stdint.h>

/** \defgroup audio_sfx     Sound Effects
    \brief                  Sound Effect Playback and Management
    \ingroup                audio

    @{
*/

/** \brief  Sound effect handle type.

    Each loaded sound effect will be assigned one of these, which is to be used
    for operations related to the effect, including playing it or unloading it.
*/
typedef uint32_t sfxhnd_t;

/** \brief  Invalid sound effect handle value.

    If a sound effect cannot be loaded, this value will be returned as the error
    condition.
*/
#define SFXHND_INVALID 0

/** \brief  Data structure for sound effect playback.

    This structure is used to pass data to the extended version of sound effect
    playback functions.
*/
typedef struct sfx_play_data {
    int chn;        /**< \brief The channel to play on. If chn == -1, the next
                            available channel will be used automatically. */
    sfxhnd_t idx;   /**< \brief The handle to the sound effect to play. */
    int vol;        /**< \brief The volume to play at (between 0 and 255). */
    int pan;        /**< \brief The panning value of the sound effect. 0 is all
                            the way to the left, 128 is center, 255 is all the way
                            to the right. */
    int loop;       /**< \brief Whether to loop the sound effect or not. */
    int freq;       /**< \brief Frequency */
    unsigned int loopstart;  /**< \brief Loop start index (in samples). */
    unsigned int loopend;    /**< \brief Loop end index (in samples). If loopend == 0,
                            the loop end will default to sfx size in samples. */
} sfx_play_data_t;

/** \brief  Load a sound effect.

    This function loads a sound effect from a WAV file and returns a handle to
    it. The sound effect can be either stereo or mono, and must either be 8-bit
    or 16-bit uncompressed PCM samples, or 4-bit Yamaha ADPCM.

    \warning The sound effect you are loading must be at most 65534 samples
    in length.

    \param  fn              The file to load.
    \return                 A handle to the sound effect on success. On error,
                            SFXHND_INVALID is returned.
*/
sfxhnd_t snd_sfx_load(const char *fn);

/** \brief  Load a sound effect without wav header.

    This function loads a sound effect from a RAW file and returns a handle to
    it. The sound effect can be either stereo or mono, and must either be 8-bit
    or 16-bit uncompressed PCM samples, or 4-bit Yamaha ADPCM.

    \warning The sound effect you are loading must be at most 65534 samples
    in length and multiple by 32 bytes for each channel.

    \param  fn              The file to load.
    \param  rate            The frequency of the sound.
    \param  bitsize         The sample size (bits per sample).
    \param  channels        Number of channels.
    \return                 A handle to the sound effect on success. On error,
                            SFXHND_INVALID is returned.
*/
sfxhnd_t snd_sfx_load_ex(const char *fn, uint32_t rate, uint16_t bitsize, uint16_t channels);

/** \brief  Load a sound effect without wav header by file handler.

    This function loads a sound effect from a RAW file and returns a handle to
    it. The sound effect can be either stereo or mono, and must either be 8-bit
    or 16-bit uncompressed PCM samples, or 4-bit Yamaha ADPCM.

    \warning The sound effect you are loading must be at most 65534 samples
    in length and multiple by 32 bytes for each channel.

    \param  fd              The file handler.
    \param  len             The file length.
    \param  rate            The frequency of the sound.
    \param  bitsize         The sample size (bits per sample).
    \param  channels        Number of channels.
    \return                 A handle to the sound effect on success. On error,
                            SFXHND_INVALID is returned.
*/
sfxhnd_t snd_sfx_load_fd(file_t fd, size_t len, uint32_t rate, uint16_t bitsize, uint16_t channels);

/** \brief  Load a sound effect.

    This function loads a sound effect from a WAV file contained in memory and
    returns a handle to it. The sound effect can be either stereo or mono, and
    must either be 8-bit or 16-bit uncompressed PCM samples, or 4-bit Yamaha
    ADPCM.

    \warning The sound effect you are loading must be at most 65534 samples
    in length.

    \param  buf             The buffer to load.
    \return                 A handle to the sound effect on success. On error,
                            SFXHND_INVALID is returned.
*/
sfxhnd_t snd_sfx_load_buf(char *buf);

/** \brief  Load a sound effect without wav header from buffer.

    This function loads a sound effect from raw data contained in memory and
    returns a handle to it. The sound effect can be either stereo or mono, and
    must either be 8-bit or 16-bit uncompressed PCM samples, or 4-bit Yamaha
    ADPCM.

    \warning The sound effect you are loading must be at most 65534 samples
    in length and multiple by 32 bytes for each channel.

    \param  buf             The buffer.
    \param  len             The file length.
    \param  rate            The frequency of the sound.
    \param  bitsize         The sample size (bits per sample).
    \param  channels        Number of channels.
    \return                 A handle to the sound effect on success. On error,
                            SFXHND_INVALID is returned.
*/
sfxhnd_t snd_sfx_load_raw_buf(char *buf, size_t len, uint32_t rate, uint16_t bitsize, uint16_t channels);

/** \brief  Unload a sound effect.

    This function unloads a previously loaded sound effect, and frees the memory
    associated with it.

    \param  idx             A handle to the sound effect to unload.
*/
void snd_sfx_unload(sfxhnd_t idx);

/** \brief  Unload all loaded sound effects.

    This function unloads all previously loaded sound effect, and frees the
    memory associated with them.
*/
void snd_sfx_unload_all(void);

/** \brief  Play a sound effect.

    This function plays a loaded sound effect with the specified volume (for
    both stereo or mono) and panning values (for mono sounds only).

    \param  idx             The handle to the sound effect to play.
    \param  vol             The volume to play at (between 0 and 255).
    \param  pan             The panning value of the sound effect. 0 is all the
                            way to the left, 128 is center, 255 is all the way
                            to the right.

    \return                 The channel used to play the sound effect (or the
                            left channel in the case of a stereo sound, the
                            right channel will be the next one) on success, or
                            -1 on failure.
*/
int snd_sfx_play(sfxhnd_t idx, int vol, int pan);

/** \brief  Play a sound effect on a specific channel.

    This function works similar to snd_sfx_play(), but allows you to specify the
    channel to play on. No error checking is done with regard to the channel, so
    be sure its safe to play on that channel before trying.

    \param  chn             The channel to play on (or in the case of stereo,
                            the left channel).
    \param  idx             The handle to the sound effect to play.
    \param  vol             The volume to play at (between 0 and 255).
    \param  pan             The panning value of the sound effect. 0 is all the
                            way to the left, 128 is center, 255 is all the way
                            to the right.

    \return                 chn
*/
int snd_sfx_play_chn(int chn, sfxhnd_t idx, int vol, int pan);

/** \brief  Extended sound effect playback function.

    This function plays a sound effect with the specified parameters. This is
    the extended version of the sound effect playback functions, and is used to
    pass a structure containing the parameters to the function. With this
    function, you can additionally specify extra parameters such as frequency
    and looping (see sfx_play_data_t structure).

    \param  data            The data structure containing the information needed
                            to play the sound effect.

    \return                 chn
*/
int snd_sfx_play_ex(sfx_play_data_t *data);

/** \brief  Stop a single channel of sound.

    This function stops the specified channel of sound from playing. It does no
    checking to make sure that a sound effect is playing on the channel
    specified, and thus can be used even if you're using the channel for some
    other purpose than sound effects.

    \param  chn             The channel to stop.
*/
void snd_sfx_stop(int chn);

/** \brief  Stop all channels playing sound effects.

    This function stops all channels currently allocated to sound effects from
    playing. It does not affect channels allocated for use by something other
    than sound effects..
*/
void snd_sfx_stop_all(void);

/** \brief  Allocate a sound channel for use outside the sound effect system.

    This function finds and allocates a channel for use for things other than
    sound effects. This is useful for, for instance, the streaming code.

    \returns                The allocated channel on success, -1 on failure.
*/
int snd_sfx_chn_alloc(void);

/** \brief  Free a previously allocated channel.

    This function frees a channel that was allocated with snd_sfx_chn_alloc(),
    returning it to the pool of available channels for sound effect use.

    \param  chn             The channel to free.
*/
void snd_sfx_chn_free(int chn);

/** @} */

__END_DECLS

#endif  /* __DC_SOUND_SFXMGR_H */

