/* KallistiOS ##version##

   dc/sound/stream.h
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2020 Lawrence Sebald
   Copyright (C) 2023, 2024 Ruslan Rostovtsev

*/

/** \file    dc/sound/stream.h
    \brief   Sound streaming support.
    \ingroup audio_streaming

    This file contains declarations for doing streams of sound. This underlies
    pretty much any decoded sounds you might use, including the Ogg Vorbis
    libraries. Note that this does not actually handle decoding, so you'll have
    to worry about that yourself (or use something in kos-ports).

    \author Megan Potter
    \author Florian Schulze
    \author Lawrence Sebald
    \author Ruslan Rostovtsev
*/

#ifndef __DC_SOUND_STREAM_H
#define __DC_SOUND_STREAM_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup audio_streaming   Streaming
    \brief                      Streaming audio playback and management
    \ingroup                    audio
    @{
*/

/** \brief  The maximum number of streams that can be allocated at once. */
#define SND_STREAM_MAX 4

/** \brief  The maximum buffer size for each channel of PCM 16-bit stream. */
#define SND_STREAM_BUFFER_MAX_PCM16 (128 << 10)

/** \brief  The maximum buffer size for each channel of PCM 8-bit stream. */
#define SND_STREAM_BUFFER_MAX_PCM8  (64 << 10)

/** \brief  The maximum buffer size for each channel of ADPCM stream. */
#define SND_STREAM_BUFFER_MAX_ADPCM (32 << 10)

/** \brief  The maximum buffer size for each channel of streams by default
            and for backward compatibility. */
#define SND_STREAM_BUFFER_MAX       (64 << 10)

/** \brief  Stream handle type.

    Each stream will be assigned a handle, which will be of this type. Further
    operations on the stream will use the handle to identify which stream is
    being referred to.
*/
typedef int snd_stream_hnd_t;

/** \brief  Invalid stream handle.

    If a stream cannot be allocated, this will be returned.
*/
#define SND_STREAM_INVALID -1

/** \brief  Stream get data callback type.

    Functions for providing stream data will be of this type, and can be
    registered with snd_stream_set_callback().

    \param  hnd             The stream handle being referred to.
    \param  smp_req         The number of samples requested.
    \param  smp_recv        Used to return the number of samples available.
    \return                 A pointer to the buffer of samples. If stereo, the
                            samples should be interleaved. For best performance
                            use 32-byte aligned pointer.
*/
typedef void *(*snd_stream_callback_t)(snd_stream_hnd_t hnd, int smp_req,
                                       int *smp_recv);

/** \brief  Direct stream data transfer callback type.

    Functions for providing stream data will be of this type, and can be
    registered with snd_stream_set_callback_direct().

    \param  hnd             The stream handle being referred to.
    \param  left            Left channel buffer address on AICA side.
    \param  right           Right channel buffer address on AICA side.
    \param  size_req        Requested size for each channel.
    \retval -1              On failure.
    \retval size_recv       On success, received size.
*/
typedef size_t (*snd_stream_callback_direct_t)(snd_stream_hnd_t hnd,
    uintptr_t left,  uintptr_t right,  size_t size_req);

/** \brief  Set the callback for a given stream.

    This function sets the get data callback function for a given stream,
    overwriting any old callback that may have been in place.

    \param  hnd             The stream handle for the callback.
    \param  cb              A pointer to the callback function.
*/
void snd_stream_set_callback(snd_stream_hnd_t hnd, snd_stream_callback_t cb);

/** \brief  Set the callback for a given stream with direct transfer.

    This function sets the get data callback function for a given stream,
    overwriting any old callback that may have been in place.

    \param  hnd             The stream handle for the callback.
    \param  cb              A pointer to the callback function.
*/
void snd_stream_set_callback_direct(snd_stream_hnd_t hnd, snd_stream_callback_direct_t cb);

/** \brief  Set the user data for a given stream.

    This function sets the user data pointer for the given stream, overwriting
    any existing one that may have been in place. This is designed to allow the
    user the ability to associate a piece of data with the stream for instance
    to assist in identifying what sound is playing on a stream. The driver does
    not attempt to use this data in any way.

    \param  hnd             The stream handle to look up.
    \param  d               A pointer to the user data.
*/
void snd_stream_set_userdata(snd_stream_hnd_t hnd, void *d);

/** \brief  Get the user data for a given stream.

    This function retrieves the set user data pointer for a given stream.

    \param  hnd             The stream handle to look up.
    \return                 The user data pointer set for this stream or NULL
                            if no data pointer has been set.
*/
void *snd_stream_get_userdata(snd_stream_hnd_t hnd);

/** \brief  Stream filter callback type.

    Functions providing filters over the stream data will be of this type, and
    can be set with snd_stream_filter_add().

    \param  hnd             The stream being referred to.
    \param  obj             Filter user data.
    \param  hz              The frequency of the sound data.
    \param  channels        The number of channels in the sound data.
    \param  buffer          A pointer to the buffer to process. This is before
                            any stereo separation is done. Can be changed by the
                            filter, if appropriate.
    \param  samplecnt       A pointer to the number of samples. This can be
                            modified by the filter, if appropriate.
*/
typedef void (*snd_stream_filter_t)(snd_stream_hnd_t hnd, void *obj, int hz,
                                    int channels, void **buffer,
                                    int *samplecnt);

/** \brief  Add a filter to the specified stream.

    This function adds a filter to the specified stream. The filter will be
    called on each block of data input to the stream from then forward.

    When the stream buffer filler needs more data, it starts out by calling
    the initial callback (set above). It then calls each function in the
    effect filter chain, which can modify the buffer and the amount of data
    available as well. Filters persist across multiple calls to _init()
    but will be emptied by _shutdown().

    \param  hnd             The stream to add the filter to.
    \param  filtfunc        A pointer to the filter function.
    \param  obj             Filter function user data.
*/
void snd_stream_filter_add(snd_stream_hnd_t hnd, snd_stream_filter_t filtfunc,
                           void *obj);

/** \brief  Remove a filter from the specified stream.

    This function removes a filter that was previously added to the specified
    stream.

    \param  hnd             The stream to remove the filter from.
    \param  filtfunc        A pointer to the filter function to remove.
    \param  obj             The filter function's user data. Must be the same as
                            what was passed as obj to snd_stream_filter_add().
*/
void snd_stream_filter_remove(snd_stream_hnd_t hnd,
                              snd_stream_filter_t filtfunc, void *obj);

/** \brief  Prefill the stream buffers.

    This function has no effect. The stream is prefilled on start.
    This is deprecated and should be removed if used.

*/
static const int __snd_stream_prefill   __depr("snd_stream_prefill has no effect and should be removed") = 0;
#define snd_stream_prefill(x)  ((void)__snd_stream_prefill)

/** \brief  Initialize the stream system.

    This function initializes the sound stream system and allocates memory for
    it as needed. Note, this is not done by the default init, so if you're using
    the streaming support and not using something like the kos-ports Ogg Vorbis
    library, you'll need to call this yourself. This will implicitly call
    snd_init(), so it will potentially overwrite anything going on the AICA.

    \retval -1              On failure.
    \retval 0               On success.
*/
int snd_stream_init(void);

/** \brief  Initialize the stream system with limits.

    The same as \ref snd_stream_init but it can either reduce or not allocate
    the buffer for splitting the stereo stream at all.

    \param  channels        Max channels for any streams.
    \param  buffer_size     Max channel buffer size for any streams.

    \retval -1              On failure.
    \retval 0               On success.
*/
int snd_stream_init_ex(int channels, size_t buffer_size);

/** \brief  Shut down the stream system.

    This function shuts down the stream system and frees the memory associated
    with it. This does not call snd_shutdown().
*/
void snd_stream_shutdown(void);

/** \brief  Allocate a stream.

    This function allocates a stream and sets its parameters.

    \param  cb              The get data callback for the stream.
    \param  bufsize         The size of the buffer for each channel of the stream.
    \return                 A handle to the new stream on success,
                            SND_STREAM_INVALID on failure.
*/
snd_stream_hnd_t snd_stream_alloc(snd_stream_callback_t cb, int bufsize);

/** \brief  Reinitialize a stream.

    This function reinitializes a stream, resetting its callback function.

    \param  hnd             The stream handle to reinit.
    \param  cb              The new get data callback for the stream.
    \return                 hnd
*/
int snd_stream_reinit(snd_stream_hnd_t hnd, snd_stream_callback_t cb);

/** \brief  Destroy a stream.

    This function destroys a previously created stream, freeing all memory
    associated with it.

    \param  hnd             The stream to clean up.
*/
void snd_stream_destroy(snd_stream_hnd_t hnd);

/** \brief  Enable queueing on a stream.

    This function enables queueing on the specified stream. This will make it so
    that you must call snd_stream_queue_go() to actually start the stream, after
    scheduling the start. This is useful for getting something ready but not
    firing it right away.

    \param  hnd             The stream to enable queueing on.
*/
void snd_stream_queue_enable(snd_stream_hnd_t hnd);

/** \brief  Disable queueing on a stream.

    This function disables queueing on the specified stream. This does not imply
    that a previously queued start on the stream will be fired if queueing was
    enabled before.

    \param  hnd             The stream to disable queueing on.
*/
void snd_stream_queue_disable(snd_stream_hnd_t hnd);

/** \brief  Start a stream after queueing the request.

    This function makes the stream start once a start request has been queued,
    if queueing mode is enabled on the stream.

    \param  hnd             The stream to start the queue on.
*/
void snd_stream_queue_go(snd_stream_hnd_t hnd);

/** \brief  Start a 16-bit PCM stream.

    This function starts processing the given stream, prefilling the buffers as
    necessary. In queueing mode, this will not start playback.

    \param  hnd             The stream to start.
    \param  freq            The frequency of the sound.
    \param  st              1 if the sound is stereo, 0 if mono.
*/
void snd_stream_start(snd_stream_hnd_t hnd, uint32 freq, int st);

/** \brief  Start a 8-bit PCM stream.

    This function starts processing the given stream, prefilling the buffers as
    necessary. In queueing mode, this will not start playback.

    \param  hnd             The stream to start.
    \param  freq            The frequency of the sound.
    \param  st              1 if the sound is stereo, 0 if mono.
*/
void snd_stream_start_pcm8(snd_stream_hnd_t hnd, uint32 freq, int st);

/** \brief  Start a 4-bit ADPCM stream.

    This function starts processing the given stream, prefilling the buffers as
    necessary. In queueing mode, this will not start playback.

    \param  hnd             The stream to start.
    \param  freq            The frequency of the sound.
    \param  st              1 if the sound is stereo, 0 if mono.
*/
void snd_stream_start_adpcm(snd_stream_hnd_t hnd, uint32 freq, int st);

/** \brief  Stop a stream.

    This function stops a stream, stopping any sound playing from it. This will
    happen immediately, regardless of whether queueing is enabled or not.

    \param  hnd             The stream to stop.
*/
void snd_stream_stop(snd_stream_hnd_t hnd);

/** \brief  Poll a stream.

    This function polls the specified stream to load more data if necessary. If
    using the streaming support, you must call this function periodically (most
    likely in a thread), or you won't get any sound output.

    \param  hnd             The stream to poll.
    \retval -3              If NULL was returned from the callback.
    \retval -1              If no callback is set, or if the state has been
                            corrupted.
    \retval 0               On success.
*/
int snd_stream_poll(snd_stream_hnd_t hnd);

/** \brief  Set the volume on the stream.

    This function sets the volume of the specified stream.

    \param  hnd             The stream to set volume on.
    \param  vol             The volume to set. Valid values are 0-255.
*/
void snd_stream_volume(snd_stream_hnd_t hnd, int vol);

/** \brief  Set the panning on the stream.

    This function sets the panning of the specified stream.

    \param  hnd             The stream to set volume on.
    \param  left_pan        The left panning to set. Valid values are 0-255.
    \param  right_pan       The right panning to set. Valid values are 0-255.
*/
void snd_stream_pan(snd_stream_hnd_t hnd, int left_pan, int right_pan);

/** @} */

__END_DECLS

#endif  /* __DC_SOUND_STREAM_H */
