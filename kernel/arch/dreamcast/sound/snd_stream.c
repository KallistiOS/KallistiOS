/* KallistiOS ##version##

   snd_stream.c
   Copyright (C) 2000, 2001, 2002, 2003, 2004 Megan Potter
   Copyright (C) 2002 Florian Schulze
   Copyright (C) 2020 Lawrence Sebald
   Copyright (C) 2023, 2024 Ruslan Rostovtsev
   Copyright (C) 2024 Stefanos Kornilios Mitsis Poiitidis

   SH-4 support routines for SPU streaming sound driver
*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/queue.h>

#include <kos/dbglog.h>
#include <kos/mutex.h>
#include <arch/cache.h>
#include <arch/timer.h>
#include <dc/g2bus.h>
#include <dc/sq.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>
#include <dc/sound/stream.h>
#include <dc/sound/sfxmgr.h>

#include "arm/aica_cmd_iface.h"

/*

This module uses a nice circularly queued data stream in SPU RAM, which is
looped by a program running in the SPU itself.

Basically the poll routine checks to see if a certain minimum amount of
data is available to the SPU to be played, and if not, we ask the user
routine for more sound data and load it up. That's about it.

This version is capable of playing back N streams at once, with the limit
being available CPU time and channels.

*/

typedef struct filter {
    TAILQ_ENTRY(filter) lent;
    snd_stream_filter_t func;
    void *data;
} filter_t;

/* Each of these represents an active streaming channel */
typedef struct strchan {
    /* Which AICA channels are we using? */
    int ch[2];

    /* The last write position in the playing buffer */
    uint32_t last_write_pos;

    /* The buffer size allocated for this stream. */
    size_t buffer_size;

    /* Stream data location in AICA RAM */
    uint32_t spu_ram_sch[2];

    /* "Get data" callback; we'll call this any time we want to get
       another buffer of output data. */
    snd_stream_callback_t get_data;

    /* "Request data" callback; we'll call this any time we want to fill
       buffers of AICA channels directly. */
    snd_stream_callback_direct_t req_data;

    /* Our list of filter callback functions for this stream */
    TAILQ_HEAD(filterlist, filter) filters;

    /* Sample type */
    int type;

    /* Sample size */
    int bitsize;

    /* Stereo/mono flag */
    int channels;

    /* Playback frequency */
    int frequency;

    /* Stream queueing is where we get everything ready to go but don't
       actually start it playing until the signal (for music sync, etc) */
    int queueing;

    /* Have we been initialized yet? (and reserved a buffer, etc) */
    volatile int initted;

    /* User data. */
    void *user_data;

    uint32_t dma_length;
    uintptr_t dma_dest;
    kthread_t *mutex_thd;
} strchan_t;

/* Our stream structs */
static strchan_t streams[SND_STREAM_MAX];

/* Separation buffers (for stereo) */
static uint32_t *sep_buffer[2] = {NULL, NULL};

static mutex_t stream_mutex = MUTEX_INITIALIZER;

static int max_channels = 2;

/* Check an incoming handle */
#define CHECK_HND(x) do { \
        assert( (x) >= 0 && (x) < SND_STREAM_MAX ); \
        assert( streams[(x)].initted ); \
    } while(0)

static size_t snd_stream_fill(snd_stream_hnd_t hnd, uint32_t offset, size_t size);

static inline size_t samples_to_bytes(snd_stream_hnd_t hnd, size_t samples) {
    switch(streams[hnd].bitsize) {
        case 4:
            return samples >> 1;
        case 8:
            return samples;
        case 16:
        default:
            return samples << 1;
    }
}

static inline size_t bytes_to_samples(snd_stream_hnd_t hnd, size_t bytes) {
    switch(streams[hnd].bitsize) {
        case 4:
            return bytes << 1;
        case 8:
            return bytes;
        case 16:
        default:
            return bytes >> 1;
    }
}

/* Set "get data" callback */
void snd_stream_set_callback(snd_stream_hnd_t hnd, snd_stream_callback_t cb) {
    CHECK_HND(hnd);
    streams[hnd].get_data = cb;
}

void snd_stream_set_callback_direct(snd_stream_hnd_t hnd, snd_stream_callback_direct_t cb) {
    CHECK_HND(hnd);
    streams[hnd].req_data = cb;
}

void snd_stream_set_userdata(snd_stream_hnd_t hnd, void *d) {
    CHECK_HND(hnd);
    streams[hnd].user_data = d;
}

void *snd_stream_get_userdata(snd_stream_hnd_t hnd) {
    CHECK_HND(hnd);
    return streams[hnd].user_data;
}

void snd_stream_filter_add(snd_stream_hnd_t hnd, snd_stream_filter_t filtfunc, void * obj) {
    filter_t *f;

    CHECK_HND(hnd);

    f = malloc(sizeof(filter_t));
    f->func = filtfunc;
    f->data = obj;
    TAILQ_INSERT_TAIL(&streams[hnd].filters, f, lent);
}

void snd_stream_filter_remove(snd_stream_hnd_t hnd, snd_stream_filter_t filtfunc, void * obj) {
    filter_t *f;

    CHECK_HND(hnd);

    TAILQ_FOREACH(f, &streams[hnd].filters, lent) {
        if(f->func == filtfunc && f->data == obj) {
            TAILQ_REMOVE(&streams[hnd].filters, f, lent);
            free(f);
            return;
        }
    }
}

static inline void process_filters(snd_stream_hnd_t hnd, void **buffer, int *samplecnt) {
    filter_t *f;

    TAILQ_FOREACH(f, &streams[hnd].filters, lent) {
        f->func(hnd, f->data, streams[hnd].frequency, streams[hnd].channels, buffer, samplecnt);
    }
}

static void snd_pcm16_split_unaligned(void *buffer, void *left, void *right, size_t len) {
    uint32_t *buf = (uint32_t *)buffer;
    uint32_t *left_ptr = (uint32_t *)left;
    uint32_t *right_ptr = (uint32_t *)right;
    uint32_t data;
    uint32_t left_val;
    uint32_t right_val;

    for(; len >= 8; len -= 8) {
        dcache_pref_block(buf + 8);

        data = *buf++;
        left_val = (data >> 16);
        right_val = (data & 0xffff);

        data = *buf++;
        left_val |= (data & 0xffff0000);
        right_val |= (data & 0xffff) << 16;

        if(((uintptr_t)left_ptr & 31) == 0) {
            dcache_alloc_block(left_ptr++, left_val);
            dcache_alloc_block(right_ptr++, right_val);
        }
        else {
            *left_ptr++ = left_val;
            *right_ptr++ = right_val;
        }
    }
    if(len) {
        data = *buf++;
        *(uint16_t *)left_ptr = (data >> 16);
        *(uint16_t *)right_ptr = (data & 0xffff);
    }
}

void snd_pcm16_split_sq(uint32_t *data, uintptr_t left, uintptr_t right, size_t size) {
    g2_ctx_t ctx;
    uint32_t i;
    uint16 *s = (uint16 *)data;
    size_t remain = size;
    uint32_t *masked_left;
    uint32_t *masked_right;

    /* SPU memory in cached area */
    left |= SPU_RAM_BASE;
    right |= SPU_RAM_BASE;

    masked_left = SQ_MASK_DEST(left);
    masked_right = SQ_MASK_DEST(right);

    sq_lock((void *)left);
    dcache_pref_block(s);

    ctx = g2_lock();

    /* Make sure the FIFOs are empty */
    g2_fifo_wait();

    /* Separating channels and fill/write queues as many times as necessary. */
    for(; remain >= 128; remain -= 128) {

        /* Fill SQ0 */
        for(i = 0; i < 16; i += 2) {
            masked_left[i / 2] = (s[i * 2] << 16) | s[(i + 1) * 2];
        }

        /* Write-back SQ0 */
        sq_flush(masked_left);

        /* Fill SQ1 */
        for(i = 16; i < 32; i += 2) {
            masked_left[i / 2] = (s[i * 2] << 16) | s[(i + 1) * 2];
        }

        /* Write-back SQ1 */
        sq_flush(masked_left + 8);
        masked_left += 16;

        /* Fill SQ0 */
        for(i = 0; i < 16; i += 2) {
            masked_right[i / 2] = (s[(i * 2) + 1] << 16) | s[((i + 1) * 2) + 1];
        }

        /* Write-back SQ0 */
        sq_flush(masked_right);

        /* Fill SQ1 */
        for(i = 16; i < 32; i += 2) {
            masked_right[i / 2] = (s[(i * 2) + 1] << 16) | s[((i + 1) * 2) + 1];
        }

        /* Write-back SQ1 */
        sq_flush(masked_right + 8);
        masked_right += 16;
        s += 64;
    }

    sq_unlock();

    /* We can wait after unlock because G2 lock disables IRQ */
    sq_wait();

    if(remain) {
        left |= MEM_AREA_P2_BASE;
        right |= MEM_AREA_P2_BASE;
        left += size - remain;
        right += size - remain;

        for(; remain >= 4; remain -= 4) {
            *((vuint16 *)left) = *s++;
            *((vuint16 *)right) = *s++;
            left += 2;
            right += 2;
        }
    }
    g2_unlock(ctx);
}


/* Prefill buffers -- implicitly called by snd_stream_start() */
void snd_stream_prefill(snd_stream_hnd_t hnd) {
    strchan_t *stream;

    CHECK_HND(hnd);
    stream = &streams[hnd];

    if(!stream->get_data && !stream->req_data) {
        return;
    }

    snd_stream_fill(hnd, 0, stream->buffer_size / 2);
    snd_stream_fill(hnd, stream->buffer_size / 2, stream->buffer_size / 2);

    /* Start playing from the beginning */
    stream->last_write_pos = 0;
}

/* Initialize stream system */
int snd_stream_init(void) {
    return snd_stream_init_ex(2, SND_STREAM_BUFFER_MAX);
}

int snd_stream_init_ex(int channels, size_t buffer_size) {
    max_channels = channels;

    if(buffer_size > 0) {
        /* Create stereo separation buffers. This buffer size for each channel.
           But half size of streams buffer is enough, because stream
           polling doesn't read more than half buffer at time.
           This can also be used for mono streams on unaligned data.
        */
        sep_buffer[0] = aligned_alloc(32, buffer_size);

        if(sep_buffer[0] == NULL) {
            dbglog(DBG_ERROR, "snd_stream_init_ex(): memory allocation failed\n");
            return -1;
        }
        sep_buffer[1] = sep_buffer[0] + (buffer_size / 8);
    }
    else {
        sep_buffer[0] = NULL;
        sep_buffer[1] = NULL;
    }

    /* Finish loading the stream driver */
    if(snd_init() < 0) {
        dbglog(DBG_ERROR, "snd_stream_init_ex(): snd_init() failed, giving up\n");
        return -1;
    }

    return 0;
}

snd_stream_hnd_t snd_stream_alloc(snd_stream_callback_t cb, int bufsize) {
    int i;
    snd_stream_hnd_t hnd;

    /* Get an unused handle */
    hnd = -1;

    for(i = 0; i < SND_STREAM_MAX; i++) {
        if(!streams[i].initted) {
            hnd = i;
            break;
        }
    }
    if(hnd == -1) {
        return SND_STREAM_INVALID;
    }

    streams[hnd].initted = 1;

    /* Default this for now */
    streams[hnd].buffer_size = bufsize;

    /* Start off with queueing disabled */
    streams[hnd].queueing = 0;

    /* Setup the callback */
    snd_stream_set_callback(hnd, cb);
    snd_stream_set_callback_direct(hnd, NULL);

    /* Initialize our filter chain list */
    TAILQ_INIT(&streams[hnd].filters);

    /* Allocate stream buffers */
    streams[hnd].spu_ram_sch[0] = snd_mem_malloc(streams[hnd].buffer_size * max_channels);

    /* And channels */
    streams[hnd].ch[0] = snd_sfx_chn_alloc();

    if(max_channels == 2) {
        streams[hnd].spu_ram_sch[1] = streams[hnd].spu_ram_sch[0] + streams[hnd].buffer_size;
        streams[hnd].ch[1] = snd_sfx_chn_alloc();
    }

    // dbglog(DBG_INFO, "snd_stream: alloc'd channels %d/%d\n", streams[hnd].ch[0], streams[hnd].ch[1]);
    return hnd;
}

snd_stream_hnd_t snd_stream_reinit(snd_stream_hnd_t hnd, snd_stream_callback_t cb) {
    CHECK_HND(hnd);

    /* Start off with queueing disabled */
    streams[hnd].queueing = 0;

    /* Setup the callback */
    snd_stream_set_callback(hnd, cb);
    snd_stream_set_callback_direct(hnd, NULL);

    return hnd;
}

void snd_stream_destroy(snd_stream_hnd_t hnd) {
    filter_t *c, *n;

    assert(hnd >= 0 && hnd < SND_STREAM_MAX);

    if(!streams[hnd].initted) {
        return;
    }

    mutex_lock(&stream_mutex);
    snd_stream_stop(hnd);
    snd_sfx_chn_free(streams[hnd].ch[0]);

    if(max_channels == 2) {
        snd_sfx_chn_free(streams[hnd].ch[1]);
    }

    c = TAILQ_FIRST(&streams[hnd].filters);

    while(c) {
        n = TAILQ_NEXT(c, lent);
        free(c);
        c = n;
    }

    TAILQ_INIT(&streams[hnd].filters);

    snd_mem_free(streams[hnd].spu_ram_sch[0]);
    // dbglog(DBG_INFO, "snd_stream: dealloc'd channels %d/%d\n", streams[hnd].ch[0], streams[hnd].ch[1]);
    memset(streams + hnd, 0, sizeof(streams[0]));

    mutex_unlock(&stream_mutex);
}

/* Shut everything down and free mem */
void snd_stream_shutdown(void) {
    /* Stop and destroy all active stream */
    int i;

    for(i = 0; i < SND_STREAM_MAX; i++) {
        if(streams[i].initted)
            snd_stream_destroy(i);
    }

    /* Free global buffers */
    if(sep_buffer[0]) {
        free(sep_buffer[0]);
        sep_buffer[0] = NULL;
        sep_buffer[1] = NULL;
    }
}

/* Enable / disable stream queueing */
void snd_stream_queue_enable(snd_stream_hnd_t hnd) {
    CHECK_HND(hnd);
    streams[hnd].queueing = 1;
}

void snd_stream_queue_disable(snd_stream_hnd_t hnd) {
    CHECK_HND(hnd);
    streams[hnd].queueing = 0;
}

/* Start streaming (or if queueing is enabled, just get ready) */
static void snd_stream_start_type(snd_stream_hnd_t hnd, uint32_t type, uint32_t freq, int st) {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    CHECK_HND(hnd);

    if(!streams[hnd].get_data && !streams[hnd].req_data) {
        return;
    }

    streams[hnd].type = type;
    streams[hnd].channels = st ? 2 : 1;
    streams[hnd].frequency = freq;

    if(streams[hnd].channels > max_channels) {
        dbglog(DBG_ERROR, "snd_stream_start_type: initted only for mono\n");
        return;
    }

    if(streams[hnd].type == AICA_SM_16BIT) {
        streams[hnd].bitsize = 16;

        if(streams[hnd].buffer_size > SND_STREAM_BUFFER_MAX_PCM16) {
            streams[hnd].buffer_size = SND_STREAM_BUFFER_MAX_PCM16;
        }
    }
    else if(streams[hnd].type == AICA_SM_8BIT) {
        streams[hnd].bitsize = 8;

        if(streams[hnd].buffer_size > SND_STREAM_BUFFER_MAX_PCM8) {
            streams[hnd].buffer_size = SND_STREAM_BUFFER_MAX_PCM8;
        }
    }
    else if(streams[hnd].type == AICA_SM_ADPCM_LS) {
        streams[hnd].bitsize = 4;

        if(streams[hnd].buffer_size > SND_STREAM_BUFFER_MAX_ADPCM) {
            streams[hnd].buffer_size = SND_STREAM_BUFFER_MAX_ADPCM;
        }
    }

    /* Prefill buffers */
    snd_stream_prefill(hnd);

    /* Make sure these are sync'd (and/or delayed) */
    snd_sh4_to_aica_stop();

    /* Channel 0 */
    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = streams[hnd].ch[0];
    chan->cmd = AICA_CH_CMD_START | AICA_CH_START_DELAY;
    chan->base = streams[hnd].spu_ram_sch[0];
    chan->type = type;
    chan->length = bytes_to_samples(hnd, streams[hnd].buffer_size);
    chan->loop = 1;
    chan->loopstart = 0;
    chan->loopend = chan->length - 1;
    chan->freq = freq;
    chan->vol = 255;
    chan->pan = streams[hnd].channels == 2 ? 0 : 128;
    snd_sh4_to_aica(tmp, cmd->size);

    if(streams[hnd].channels == 2) {
        /* Channel 1 */
        cmd->cmd_id = streams[hnd].ch[1];
        chan->base = streams[hnd].spu_ram_sch[1];
        chan->pan = 255;
        snd_sh4_to_aica(tmp, cmd->size);

        /* Start both channels simultaneously */
        cmd->cmd_id = (1ULL << streams[hnd].ch[0]) |
                      (1ULL << streams[hnd].ch[1]);
    }
    else {
        /* Start one channel */
        cmd->cmd_id = (1ULL << streams[hnd].ch[0]);
    }

    chan->cmd = AICA_CH_CMD_START | AICA_CH_START_SYNC;
    snd_sh4_to_aica(tmp, cmd->size);

    /* Process the changes */
    if(!streams[hnd].queueing)
        snd_sh4_to_aica_start();
}

void snd_stream_start(snd_stream_hnd_t hnd, uint32_t freq, int st) {
    snd_stream_start_type(hnd, AICA_SM_16BIT, freq, st);
}

void snd_stream_start_pcm8(snd_stream_hnd_t hnd, uint32_t freq, int st) {
    snd_stream_start_type(hnd, AICA_SM_8BIT, freq, st);
}

void snd_stream_start_adpcm(snd_stream_hnd_t hnd, uint32_t freq, int st) {
    snd_stream_start_type(hnd, AICA_SM_ADPCM_LS, freq, st);
}

/* Actually make it go (in queued mode) */
void snd_stream_queue_go(snd_stream_hnd_t hnd) {
    (void)hnd;
    CHECK_HND(hnd);
    snd_sh4_to_aica_start();
}

/* Stop streaming */
void snd_stream_stop(snd_stream_hnd_t hnd) {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    CHECK_HND(hnd);

    if(!streams[hnd].get_data && !streams[hnd].req_data) {
        return;
    }

    if(streams[hnd].channels == 2) {
        snd_sh4_to_aica_stop();
    }

    /* Stop stream */
    /* Channel 0 */
    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = streams[hnd].ch[0];
    chan->cmd = AICA_CH_CMD_STOP;
    snd_sh4_to_aica(tmp, cmd->size);

    if(streams[hnd].channels == 2) {
        /* Channel 1 */
        cmd->cmd_id = streams[hnd].ch[1];
        snd_sh4_to_aica(tmp, cmd->size);
        snd_sh4_to_aica_start();
    }
}

/* The DMA will chain to this to start the second DMA. */
static inline void dma_done(void *data) {
    strchan_t *stream = (strchan_t *)data;
    mutex_unlock_as_thread(&stream_mutex, stream->mutex_thd);
}

static inline void dma_chain(void *data) {
    strchan_t *stream = (strchan_t *)data;
    int rs = spu_dma_transfer(sep_buffer[1],
        stream->dma_dest, stream->dma_length, 0, dma_done, data);
    if(rs < 0) {
        dma_done(data);
    }
}

static int snd_stream_transfer(strchan_t *stream, void *first_buf,
                                uint32_t offset, size_t size) {
    int rs;

    dcache_purge_range((uintptr_t)first_buf, size);
    stream->mutex_thd = thd_current;

    if(stream->channels == 2) {
        dcache_purge_range((uintptr_t)sep_buffer[1], size);
        stream->dma_dest = stream->spu_ram_sch[1] + offset;
        stream->dma_length = size;
    }

    do {
        rs = spu_dma_transfer(first_buf,
            stream->spu_ram_sch[0] + offset,
            size,
            0,
            (stream->channels == 1 ? dma_done : dma_chain),
            (void *)stream);

        if(rs == 0) {
            break;
        }
        if(errno != EINPROGRESS) {
            mutex_unlock(&stream_mutex);
            return -1;
        }
        thd_pass();
    } while(1);

    return 0;
}

static size_t snd_stream_fill(snd_stream_hnd_t hnd, uint32_t offset, size_t size) {
    strchan_t *stream = &streams[hnd];
    const int chans = stream->channels;
    const uintptr_t left = stream->spu_ram_sch[0] + offset;
    const uintptr_t right = stream->spu_ram_sch[1] + offset;
    const int needed_bytes = size * chans;
    int got_bytes = 0;
    void *data = NULL;

    if(stream->req_data) {
        got_bytes = stream->req_data(hnd,
            (left | SPU_RAM_UNCACHED_BASE),
            (chans == 2 ? (right | SPU_RAM_UNCACHED_BASE) : 0),
            needed_bytes);
    }
    if(got_bytes > 0) {
        return got_bytes;
    }
    if(stream->get_data) {
        data = stream->get_data(hnd, needed_bytes, &got_bytes);
    }

    if(data == NULL || got_bytes == 0) {
        /* sep_buffer isn't allocated if all streams are mono
           or direct streams are used. */
        if(sep_buffer[0] == NULL) {
            spu_memset_sq(left, 0, needed_bytes);
        }
        else {
            mutex_lock(&stream_mutex);
            memset(sep_buffer[0], 0, needed_bytes / chans);
            if(chans == 2) {
                memset(sep_buffer[1], 0, needed_bytes / chans);
            }
            snd_stream_transfer(stream, sep_buffer[0], offset, needed_bytes / chans);
        }
        return 0;
    }

    if(got_bytes > needed_bytes) {
        got_bytes = needed_bytes;
    }

    process_filters(hnd, &data, &got_bytes);

    if(chans == 1) {
        if(got_bytes & 3) {
            got_bytes = (got_bytes + 4) & ~3;
        }
        if(((uintptr_t)data & 31) && sep_buffer[0] == NULL) {
            spu_memload_sq(left, data, got_bytes);
            return got_bytes;
        }
        mutex_lock(&stream_mutex);

        if((uintptr_t)data & 31) {
            memcpy(sep_buffer[0], data, got_bytes);
            data = sep_buffer[0];
        }
        if(snd_stream_transfer(stream, data, offset, got_bytes) < 0) {
            return 0;
        }
        return got_bytes;
    }

    if(got_bytes & 7) {
        got_bytes = (got_bytes + 8) & ~7;
    }

    mutex_lock(&stream_mutex);

    if(stream->bitsize == 16) {
        if((uintptr_t)data & 31) {
            snd_pcm16_split_unaligned(data, sep_buffer[0], sep_buffer[1], got_bytes);
        }
        else {
            snd_pcm16_split((uint32_t *)data, sep_buffer[0], sep_buffer[1], got_bytes);
        }
    }
    else if(stream->bitsize == 8) {
        snd_pcm8_split(data, sep_buffer[0], sep_buffer[1], got_bytes);
    }
    else if(stream->bitsize == 4) {
        snd_adpcm_split(data, sep_buffer[0], sep_buffer[1], got_bytes);
    }

    if(snd_stream_transfer(stream, sep_buffer[0], offset, got_bytes / chans) < 0) {
        return 0;
    }
    return got_bytes;
}

/* Poll streamer to load more data if necessary */
int snd_stream_poll(snd_stream_hnd_t hnd) {
    uint32_t write_pos;
    uint16_t current_play_pos;
    int needed_samples = 0;
    size_t needed_bytes = 0;
    int got_bytes = 0;
    strchan_t *stream;

    assert(hnd >= 0 && hnd < SND_STREAM_MAX);
    stream = &streams[hnd];

    if(!stream->initted || (!stream->get_data && !stream->req_data)) {
        return -1;
    }

    /* Get channels position */
    current_play_pos = g2_read_32(SPU_RAM_UNCACHED_BASE +
                        AICA_CHANNEL(stream->ch[0]) +
                        offsetof(aica_channel_t, pos)) & 0xffff;

    needed_bytes = samples_to_bytes(hnd, current_play_pos);

    if(needed_bytes >= stream->buffer_size) {
        dbglog(DBG_ERROR, "snd_stream_poll: chan0(%d).pos = %d\n", stream->ch[0], current_play_pos);
        return -1;
    }

    if(needed_bytes & 31) {
        /* Aligning for DMA. */
        current_play_pos &= ~(bytes_to_samples(hnd, 32) - 1);
    }

    /* Count just till the end of the buffer, so we don't have to
       handle buffer wraps */
    if(stream->last_write_pos <= current_play_pos) {
        needed_samples = current_play_pos - stream->last_write_pos - 1;
        /* Round it to max sector size of supported storage devices */
        needed_samples &= ~(bytes_to_samples(hnd, 2048 / stream->channels) - 1);
        needed_bytes = samples_to_bytes(hnd, needed_samples);
        /* Reduce data requests */
        if(needed_bytes < (stream->buffer_size / 2)) {
            return 0;
        }
    }
    else {
        needed_samples = bytes_to_samples(hnd, stream->buffer_size);
        needed_samples -= stream->last_write_pos;
        needed_bytes = samples_to_bytes(hnd, needed_samples);
    }

    if(needed_samples <= 0) {
        return 0;
    }

    if(needed_bytes > stream->buffer_size / 2) {
        needed_bytes = (int)stream->buffer_size / 2;
    }

    if(!stream->initted) {
        return -2;
    }

    write_pos = samples_to_bytes(hnd, stream->last_write_pos);
    got_bytes = snd_stream_fill(hnd, write_pos, needed_bytes);

    if(got_bytes == 0) {
        return -3;
    }

    needed_samples = bytes_to_samples(hnd, got_bytes / stream->channels);

    stream->last_write_pos += needed_samples;
    write_pos = (uint32_t)bytes_to_samples(hnd, stream->buffer_size);

    if(stream->last_write_pos >= write_pos) {
        stream->last_write_pos -= write_pos;
    }

    return 0;
}

/* Set the volume on the streaming channels */
void snd_stream_volume(snd_stream_hnd_t hnd, int vol) {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    CHECK_HND(hnd);

    if(streams[hnd].channels == 2) {
        snd_sh4_to_aica_stop();
    }

    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = streams[hnd].ch[0];
    chan->cmd = AICA_CH_CMD_UPDATE | AICA_CH_UPDATE_SET_VOL;
    chan->vol = vol;
    snd_sh4_to_aica(tmp, cmd->size);

    if(streams[hnd].channels == 2) {
        cmd->cmd_id = streams[hnd].ch[1];
        snd_sh4_to_aica(tmp, cmd->size);
        snd_sh4_to_aica_start();
    }
}

/* Set the panning on the streaming channels */
void snd_stream_pan(snd_stream_hnd_t hnd, int left_pan, int right_pan) {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    CHECK_HND(hnd);

    if(streams[hnd].channels == 2) {
        snd_sh4_to_aica_stop();
    }

    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = streams[hnd].ch[0];
    chan->cmd = AICA_CH_CMD_UPDATE | AICA_CH_UPDATE_SET_PAN;
    chan->pan = left_pan;
    snd_sh4_to_aica(tmp, cmd->size);

    if(streams[hnd].channels == 2) {
        cmd->cmd_id = streams[hnd].ch[1];
        chan->pan = right_pan;
        snd_sh4_to_aica(tmp, cmd->size);
        snd_sh4_to_aica_start();
    }
}
