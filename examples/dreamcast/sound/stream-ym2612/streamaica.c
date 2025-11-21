#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include <dc/sound/sound.h>
#include <dc/sound/stream.h>
#include <dc/spu.h>
#include <dc/sq.h>
#include <kos/timer.h>


// true for DMA, otherwise, false for SQ.
#define DMA_ENABLED true

#define STREAM_MAX_BYTES (SND_STREAM_BUFFER_MAX)
#define STREAM_MAX_SAMPLES (STREAM_MAX_BYTES / sizeof(int16_t))


#define MIN_VALUE(VALUE_1, VALUE_2) ({ \
    __typeof(VALUE_1) v1 = VALUE_1;    \
    __typeof(VALUE_2) v2 = VALUE_2;    \
    v1 < v2 ? v1 : v2;                 \
})


extern void spu_dma_transfer_enqueue(void* from, uintptr_t dest, size_t length);
extern void spu_dma_transfer_flush();
extern void spu_dma_transfer_wait();

static size_t stream_channels;
static size_t stream_frequency;
static snd_stream_hnd_t streams[4];
static size_t buffers_available[8];
static size_t buffers_offset[8];
static int16_t* buffers[8];
static uint8_t panning[8];
static uint64_t elapsed;


static void streamaica_transfer(int16_t* buffer, uintptr_t dest, size_t needed_bytes) {
    dest &= ~SPU_RAM_UNCACHED_BASE;

#if DMA_ENABLED
    uintptr_t buffer_address = (uintptr_t)buffer;
    if (__is_aligned(buffer_address, 32)) {
        dcache_purge_range(buffer_address, needed_bytes);
        spu_dma_transfer_enqueue(buffer, dest, needed_bytes);
        return;
    }
#endif

    spu_memload_sq(dest, buffer, needed_bytes);
}

static size_t streamaica_callback(snd_stream_hnd_t hnd, uintptr_t left, uintptr_t right, size_t size_req) {
    size_t base_index = (size_t)snd_stream_get_userdata(hnd);

    size_t needed_bytes = size_req / 2;

    size_t index1 = base_index + 0;
    size_t index2 = base_index + 1;

    int16_t* buffer1 = buffers[index1] + buffers_offset[index1];
    int16_t* buffer2 = buffers[index2] + buffers_offset[index2];

    size_t available_samples = MIN_VALUE(buffers_available[index1], buffers_available[index2]);
    size_t available_bytes = available_samples * sizeof(int16_t);

    if (available_bytes < needed_bytes) {
        needed_bytes = available_bytes;
    } else {
        available_samples = needed_bytes / sizeof(int16_t);
    }

    if (needed_bytes < 1) {
        // one or both buffers are depleted
        return 0;
    }

    buffers_offset[index1] += available_samples;
    buffers_offset[index2] += available_samples;
    buffers_available[index1] -= available_samples;
    buffers_available[index2] -= available_samples;

    streamaica_transfer(buffer1, left, needed_bytes);
    streamaica_transfer(buffer2, right, needed_bytes);

    return needed_bytes * 2;
}


void streamaica_init(size_t channels, uint32_t sample_rate) {
    snd_stream_init_ex(2, 0);

    assert_msg(channels % 2 == 0, "channel count must be an even number");
    stream_channels = channels;
    stream_frequency = sample_rate;

    for (size_t i = 0; i < channels; i++) {
        buffers[i] = memalign(32, STREAM_MAX_BYTES);
        panning[i] = 128;
        buffers_offset[0] = 0;
        buffers_available[i] = 0;
    }

    for (size_t i = 0; i < 4; i++) streams[i] = SND_STREAM_INVALID;

    if (channels > 0) {
        streams[0] = snd_stream_alloc(NULL, STREAM_MAX_BYTES);
        snd_stream_set_callback_direct(streams[0], streamaica_callback);
        snd_stream_set_userdata(streams[0], (void*)0);
    }

    if (channels > 2) {
        streams[1] = snd_stream_alloc(NULL, STREAM_MAX_BYTES);
        snd_stream_set_callback_direct(streams[1], streamaica_callback);
        snd_stream_set_userdata(streams[1], (void*)2);
    }

    if (channels > 4) {
        streams[2] = snd_stream_alloc(NULL, STREAM_MAX_BYTES);
        snd_stream_set_callback_direct(streams[2], streamaica_callback);
        snd_stream_set_userdata(streams[2], (void*)4);
    }

    if (channels > 6) {
        streams[3] = snd_stream_alloc(NULL, STREAM_MAX_BYTES);
        snd_stream_set_callback_direct(streams[3], streamaica_callback);
        snd_stream_set_userdata(streams[3], (void*)6);
    }

    elapsed = 0;
}

int streamaica_refill(int16_t** source, int samples) {
    spu_dma_transfer_wait();

    for (size_t i = 0; i < stream_channels; i++) {
        size_t max_samples = STREAM_MAX_SAMPLES - buffers_available[i];
        if (max_samples < samples) {
            samples = max_samples;
        }
    }

    if (samples < 1) return 0;

    for (size_t i = 0; i < stream_channels; i++) {
        size_t available = buffers_available[i];
        int16_t* buffer = buffers[i];

        memmove(buffer, buffer + buffers_offset[i], available * sizeof(int16_t));
        memcpy(buffer + available, source[i], samples * sizeof(int16_t));

        buffers_available[i] = available + samples;
        buffers_offset[i] = 0;
    }

    return samples;
}

void streamaica_pad_with_silence() {
    spu_dma_transfer_wait();

    for (size_t i = 0; i < stream_channels; i++) {
        size_t available = buffers_available[i];
        int16_t* buffer = buffers[i];
        size_t padding = STREAM_MAX_SAMPLES - available;

        if (padding > 0) {
            memmove(buffer, buffer + buffers_offset[i], available * sizeof(int16_t));
            memset(buffer + available, 0x00, padding * sizeof(int16_t));

            buffers_available[i] = available + padding;
            buffers_offset[i] = 0;
        }
    }
}

void streamaica_start() {
    if (stream_channels > 0) snd_stream_setup(streams[0], stream_frequency, 1, SND_STRMFMT_PCM16);
    if (stream_channels > 2) snd_stream_setup(streams[1], stream_frequency, 1, SND_STRMFMT_PCM16);
    if (stream_channels > 4) snd_stream_setup(streams[2], stream_frequency, 1, SND_STRMFMT_PCM16);
    if (stream_channels > 6) snd_stream_setup(streams[3], stream_frequency, 1, SND_STRMFMT_PCM16);

    spu_dma_transfer_flush();
    spu_dma_transfer_wait();

    if (stream_channels > 0) snd_stream_pan(streams[0], 128, 128);
    if (stream_channels > 2) snd_stream_pan(streams[1], 128, 128);
    if (stream_channels > 4) snd_stream_pan(streams[2], 128, 128);
    if (stream_channels > 6) snd_stream_pan(streams[3], 128, 128);

    snd_stream_start_streams((snd_stream_hnd_t[5]){
        streams[0], streams[1], streams[2], streams[3], SND_STREAM_INVALID
    });

    // Note: start timestamp is not precise
    elapsed = timer_ms_gettime64();
}

uint32_t streamaica_get_elapsed_ms() {
    return (uint32_t)(timer_ms_gettime64() - elapsed);
}

void streamaica_update_panning(uint8_t* pan_values) {
    bool has_updates = false;
    if (stream_channels > 0) {
        if (panning[0] != pan_values[0] || panning[1] != pan_values[1]) {
            has_updates = true;
            snd_stream_pan(streams[0], pan_values[0], pan_values[1]);
        }
    }
    if (stream_channels > 2) {
        if (panning[2] != pan_values[2] || panning[3] != pan_values[3]) {
            has_updates = true;
            snd_stream_pan(streams[1], pan_values[2], pan_values[3]);
        }
    }
    if (stream_channels > 4) {
        if (panning[4] != pan_values[4] || panning[5] != pan_values[5]) {
            has_updates = true;
            snd_stream_pan(streams[2], pan_values[4], pan_values[5]);
        }
    }
    if (stream_channels > 6) {
        if (panning[6] != pan_values[6] || panning[7] != pan_values[7]) {
            has_updates = true;
            snd_stream_pan(streams[3], pan_values[6], pan_values[7]);
        }
    }
    if (has_updates) {
        memcpy(panning, pan_values, sizeof(panning));
    }
}

void streamaica_poll() {
    if (stream_channels > 0) snd_stream_poll(streams[0]);
    if (stream_channels > 2) snd_stream_poll(streams[1]);
    if (stream_channels > 4) snd_stream_poll(streams[2]);
    if (stream_channels > 6) snd_stream_poll(streams[3]);

    spu_dma_transfer_flush();
}

void streamaica_finalize() {
    snd_sh4_to_aica_stop();
    {
        if (stream_channels > 0) snd_stream_destroy(streams[0]);
        if (stream_channels > 2) snd_stream_destroy(streams[1]);
        if (stream_channels > 4) snd_stream_destroy(streams[2]);
        if (stream_channels > 6) snd_stream_destroy(streams[3]);
    }
    snd_sh4_to_aica_start();

    for (size_t i = 0; i < stream_channels; i++) free(buffers[i]);
}
