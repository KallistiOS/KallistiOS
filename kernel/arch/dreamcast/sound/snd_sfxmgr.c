/* KallistiOS ##version##

   snd_sfxmgr.c
   Copyright (C) 2000, 2001, 2002, 2003, 2004 Megan Potter
   Copyright (C) 2023, 2024 Ruslan Rostovtsev
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2024 Stefanos Kornilios Mitsis Poiitidis

   Sound effects management system; this thing loads and plays sound effects
   during game operation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/queue.h>
#include <sys/ioctl.h>
#include <kos/fs.h>
#include <arch/irq.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

#include "arm/aica_cmd_iface.h"

struct snd_effect;
LIST_HEAD(selist, snd_effect);

typedef struct snd_effect {
    uint32_t  locl, locr;
    uint32_t  len;
    uint32_t  rate;
    uint32_t  used;
    uint32_t  fmt;
    uint16_t  stereo;

    LIST_ENTRY(snd_effect)  list;
} snd_effect_t;

struct selist snd_effects;

/* The next channel we'll use to play sound effects. */
static int sfx_nextchan = 0;

/* Our channel-in-use mask. */
static uint64_t sfx_inuse = 0;

/* Unload all loaded samples and free their SPU RAM */
void snd_sfx_unload_all(void) {
    snd_effect_t *t, *n;

    t = LIST_FIRST(&snd_effects);

    while(t) {
        n = LIST_NEXT(t, list);

        snd_mem_free(t->locl);

        if(t->stereo)
            snd_mem_free(t->locr);

        free(t);

        t = n;
    }

    LIST_INIT(&snd_effects);
}

/* Unload a single sample */
void snd_sfx_unload(sfxhnd_t idx) {
    snd_effect_t *t = (snd_effect_t *)idx;

    if(idx == SFXHND_INVALID) {
        dbglog(DBG_WARNING, "snd_sfx: can't unload an invalid SFXHND\n");
        return;
    }

    snd_mem_free(t->locl);

    if(t->stereo)
        snd_mem_free(t->locr);

    LIST_REMOVE(t, list);
    free(t);
}

typedef struct __attribute__((__packed__)) {
    uint8_t riff[4];
    int32_t totalsize;
    uint8_t riff_format[4];
} wavmagic_t;

typedef struct __attribute__((__packed__)) {
    uint8_t id[4];
    size_t size;
} chunkhdr_t;

typedef struct __attribute__((__packed__)) {
    int16_t format;
    int16_t channels;
    int32_t sample_rate;
    int32_t byte_per_sec;
    int16_t blocksize;
    int16_t sample_size;
} fmthdr_t;

/* WAV header */
typedef struct __attribute__((__packed__)) {
    wavmagic_t magic;

    chunkhdr_t chunk;

    fmthdr_t fmt;
} wavhdr_t;

/* WAV sample formats */
#define WAVE_FMT_PCM                   0x0001 /* PCM */
#define WAVE_FMT_YAMAHA_ADPCM_ITU_G723 0x0014 /* ITU G.723 Yamaha ADPCM (KallistiOS) */
#define WAVE_FMT_YAMAHA_ADPCM          0x0020 /* Yamaha ADPCM (ffmpeg) */

static int read_wav_header(file_t fd, wavhdr_t *wavhdr) {
    if(fs_read(fd, &(wavhdr->magic), sizeof(wavhdr->magic)) != sizeof(wavhdr->magic)) {
        dbglog(DBG_WARNING, "snd_sfx: can't read wav header\n");
        return -1;
    }

    if(strncmp((const char*)wavhdr->magic.riff, "RIFF", 4)) {
        dbglog(DBG_WARNING, "snd_sfx: sfx file is not RIFF\n");
        return -1;
    }

    /* Check file magic */
    if(strncmp((const char*)wavhdr->magic.riff_format, "WAVE", 4)) {
        dbglog(DBG_WARNING, "snd_sfx: sfx file is not RIFF WAVE\n");
        return -1;
    }

    do {
        /* Read the chunk header */
        if(fs_read(fd, &(wavhdr->chunk), sizeof(wavhdr->chunk)) != sizeof(wavhdr->chunk)) {
            dbglog(DBG_WARNING, "snd_sfx: can't read chunk header\n");
            return -1;
        }

        /* If it is the fmt chunk, grab the fields we care about and skip the 
           rest of the section if there is more */
        if(strncmp((const char *)wavhdr->chunk.id, "fmt ", 4) == 0) {
            if(fs_read(fd, &(wavhdr->fmt), sizeof(wavhdr->fmt)) != sizeof(wavhdr->fmt)) {
                dbglog(DBG_WARNING, "snd_sfx: can't read fmt header\n");
                return -1;
            }

            /* Skip the rest of the fmt chunk */ 
            fs_seek(fd, wavhdr->chunk.size - sizeof(wavhdr->fmt), SEEK_CUR);
        }
        /* If we found the data chunk, we are done */
        else if(strncmp((const char *)wavhdr->chunk.id, "data", 4) == 0) {
            break;
        }
        /* Skip meta data */
        else { 
            fs_seek(fd, wavhdr->chunk.size, SEEK_CUR);
        }
    } while(1);

    return 0;
}

static int read_wav_header_buf(char *buf, wavhdr_t *wavhdr, size_t *bufidx) {
    /* maintain buffer index during function */
    size_t tmp_bufidx = *bufidx;

    memcpy(&(wavhdr->magic), buf, sizeof(wavhdr->magic));
    tmp_bufidx += sizeof(wavhdr->magic);

    if(strncmp((const char*)wavhdr->magic.riff, "RIFF", 4)) {
        dbglog(DBG_WARNING, "snd_sfx: sfx buffer is not RIFF\n");
        return -1;
    }

    /* Check file magic */
    if(strncmp((const char*)wavhdr->magic.riff_format, "WAVE", 4)) {
        dbglog(DBG_WARNING, "snd_sfx: sfx buffer is not RIFF WAVE\n");
        return -1;
    }

    do {
        /* Read the chunk header */
        memcpy(&(wavhdr->chunk), buf + tmp_bufidx, sizeof(wavhdr->chunk));
        tmp_bufidx += sizeof(wavhdr->chunk);

        /* If it is the fmt chunk, grab the fields we care about and skip the 
           rest of the section if there is more */
        if(strncmp((const char *)wavhdr->chunk.id, "fmt ", 4) == 0) {
            memcpy(&(wavhdr->fmt), buf + tmp_bufidx, sizeof(wavhdr->fmt));
            tmp_bufidx += sizeof(wavhdr->fmt);

            /* Skip the rest of the fmt chunk */ 
            tmp_bufidx += wavhdr->chunk.size - sizeof(wavhdr->fmt);
        }
        /* If we found the data chunk, we are done */
        else if(strncmp((const char *)wavhdr->chunk.id, "data", 4) == 0) {
            break;
        }
        /* Skip meta data */
        else { 
            tmp_bufidx += wavhdr->chunk.size;
        }
    } while(1);

    /* update buffer index for caller */
    *bufidx = tmp_bufidx;

    return 0;
}

static uint8_t *read_wav_data(file_t fd, wavhdr_t *wavhdr) {
    /* Allocate memory for WAV data */
    uint8_t *wav_data = aligned_alloc(32, wavhdr->chunk.size);

    if(wav_data == NULL)
        return NULL;

    /* Read WAV data */
    if((size_t)fs_read(fd, wav_data, wavhdr->chunk.size) != wavhdr->chunk.size) {
        dbglog(DBG_WARNING, "snd_sfx: file has not been fully read.\n");
        free(wav_data);
        return NULL;
    }

    return wav_data;
}

static uint8_t *read_wav_data_buf(char *buf, wavhdr_t *wavhdr, size_t *bufidx) {
    /* maintain buffer index during function */
    size_t tmp_bufidx = *bufidx;

    /* Allocate memory for WAV data */
    uint8_t *wav_data = aligned_alloc(32, wavhdr->chunk.size);

    if(wav_data == NULL)
        return NULL;

    /* Read WAV data */
    memcpy(wav_data, buf + tmp_bufidx, wavhdr->chunk.size);
    tmp_bufidx += wavhdr->chunk.size;

    /* update buffer index for caller */
    *bufidx = tmp_bufidx;

    return wav_data;
}

static snd_effect_t *create_snd_effect(wavhdr_t *wavhdr, uint8_t *wav_data) {
    snd_effect_t *effect;
    uint32_t len, rate;
    uint16_t channels, bitsize, fmt;
    
    effect = malloc(sizeof(snd_effect_t));
    if(effect == NULL)
        return NULL;

    memset(effect, 0, sizeof(snd_effect_t));

    fmt = wavhdr->fmt.format;
    channels = wavhdr->fmt.channels;
    rate = wavhdr->fmt.sample_rate;
    bitsize = wavhdr->fmt.sample_size;
    len = wavhdr->chunk.size;

    effect->rate = rate;
    effect->stereo = channels > 1;
    effect->locl = snd_mem_malloc(len / channels);

    if(!effect->locl) {
        goto err_occurred;
    }
    if(channels > 1) {
        effect->locr = snd_mem_malloc(len / channels);
        if(!effect->locr) {
            snd_mem_free(effect->locl);
            goto err_occurred;
        }
    }

    if(fmt == WAVE_FMT_YAMAHA_ADPCM_ITU_G723 || fmt == WAVE_FMT_YAMAHA_ADPCM) {
        effect->fmt = AICA_SM_ADPCM;
        effect->len = (len * 2) / channels; /* 4-bit packed samples */
    }
    else if(fmt == WAVE_FMT_PCM && bitsize == 8) {
        effect->fmt = AICA_SM_8BIT;
        effect->len = len / channels;
    }
    else if(fmt == WAVE_FMT_PCM && bitsize == 16) {
        effect->fmt = AICA_SM_16BIT;
        effect->len = (len / 2) / channels;
    }
    else {
        goto err_occurred;
    }

    if(channels == 1) {
        /* Mono PCM/ADPCM */
        spu_memload_sq(effect->locl, wav_data, len);
    }
    else if(channels == 2 && fmt == WAVE_FMT_PCM && bitsize == 16) {
        /* Stereo 16-bit PCM */
        snd_pcm16_split_sq((uint32_t *)wav_data, effect->locl, effect->locr, len);
    }
    else if(channels == 2 && fmt == WAVE_FMT_PCM && bitsize == 8) {
        /* Stereo 8-bit PCM */
        uint32_t *left_buf = aligned_alloc(32, len / 2), *right_buf;

        if(left_buf == NULL)
            goto err_occurred;

        right_buf = aligned_alloc(32, len / 2);
        if(right_buf == NULL) {
            free(left_buf);
            goto err_occurred;
        }

        snd_pcm8_split((uint32_t *)wav_data, left_buf, right_buf, len);
        spu_memload_sq(effect->locl, left_buf, len / 2);
        spu_memload_sq(effect->locr, right_buf, len / 2);

        free(left_buf);
        free(right_buf);
    }
    else if(channels == 2 && fmt == WAVE_FMT_YAMAHA_ADPCM_ITU_G723) {
        /* Stereo ADPCM ITU G.723 (channels are not interleaved) */
        uint8_t *right_buf = wav_data + (len / 2);
        int ownmem = 0;

        if(((uintptr_t)right_buf) & 3) {
            right_buf = (uint8_t *)aligned_alloc(32, len / 2);

            if(right_buf == NULL)
                goto err_occurred;

            ownmem = 1;
            memcpy(right_buf, wav_data + (len / 2), len / 2);
        }

        spu_memload_sq(effect->locl, wav_data, len / 2);
        spu_memload_sq(effect->locr, right_buf, len / 2);

        if(ownmem)
            free(right_buf);
    }
    else if(channels == 2 && fmt == WAVE_FMT_YAMAHA_ADPCM) {
        /* Stereo Yamaha ADPCM (channels are interleaved) */
        uint32_t *left_buf = (uint32_t *)aligned_alloc(32, len / 2), *right_buf;

        if(left_buf == NULL)
            goto err_occurred;

        right_buf = (uint32_t *)aligned_alloc(32, len / 2);

        if(right_buf == NULL) {
            free(left_buf);
            goto err_occurred;
        }

        snd_adpcm_split((uint32_t *)wav_data, left_buf, right_buf, len);
        spu_memload_sq(effect->locl, left_buf, len / 2);
        spu_memload_sq(effect->locr, right_buf, len / 2);

        free(left_buf);
        free(right_buf);
    }
    else {
err_occurred:
        if(effect->locl)
            snd_mem_free(effect->locl);
        if(effect->locr)
            snd_mem_free(effect->locr);
        free(effect);
        effect = SFXHND_INVALID;
    }

    return effect;
}

/* Load a sound effect from a WAV file and return a handle to it */
sfxhnd_t snd_sfx_load(const char *fn) {
    file_t fd;
    wavhdr_t wavhdr;
    snd_effect_t *effect;
    uint8_t *wav_data;
    uint32_t sample_count;

    /* Open the sound effect file */
    fd = fs_open(fn, O_RDONLY);
    if(fd <= FILEHND_INVALID) {
        dbglog(DBG_ERROR, "snd_sfx_load: can't open %s\n", fn);
        return SFXHND_INVALID;
    }

    /* Read WAV header */
    if(read_wav_header(fd, &wavhdr) < 0) {
        fs_close(fd);
        dbglog(DBG_ERROR, "snd_sfx_load: can't read wav header %s\n", fn);
        return SFXHND_INVALID;
    }
    /*
    dbglog(DBG_DEBUG, "WAVE file is %s, %luHZ, %d bits/sample, "
        "%u bytes total, format %d\n", 
           wavhdr.fmt.channels == 1 ? "mono" : "stereo", 
           wavhdr.fmt.sample_rate, 
           wavhdr.fmt.sample_size, 
           wavhdr.chunk.size, 
           wavhdr.fmt.format);
    */
    sample_count = wavhdr.fmt.sample_size >= 8 
        ? wavhdr.chunk.size / ((wavhdr.fmt.sample_size / 8) * wavhdr.fmt.channels) 
        : (wavhdr.chunk.size * 2) / wavhdr.fmt.channels;

    if(sample_count > 65534) {
        dbglog(DBG_WARNING, "snd_sfx_load: WAVE file is over 65534 samples\n");
    }

    /* Read WAV data */
    wav_data = read_wav_data(fd, &wavhdr);
    fs_close(fd);
    if(!wav_data)
        return SFXHND_INVALID;

    /* Create and initialize sound effect */
    effect = create_snd_effect(&wavhdr, wav_data);
    if(!effect) {
        free(wav_data);
        return SFXHND_INVALID;
    }

    /* Finish up and return the sound effect handle */
    free(wav_data);
    LIST_INSERT_HEAD(&snd_effects, effect, list);

    return (sfxhnd_t)effect;
}

sfxhnd_t snd_sfx_load_ex(const char *fn, uint32_t rate, uint16_t bitsize, uint16_t channels) {
    sfxhnd_t effect;
    file_t fd = fs_open(fn, O_RDONLY);

    if(fd <= FILEHND_INVALID) {
        dbglog(DBG_ERROR, "snd_sfx_load_ex: can't open sfx %s\n", fn);
        return SFXHND_INVALID;
    }
    effect = snd_sfx_load_fd(fd, fs_total(fd), rate, bitsize, channels);
    fs_close(fd);
    return effect;
}

sfxhnd_t snd_sfx_load_fd(file_t fd, size_t len, uint32_t rate, uint16_t bitsize, uint16_t channels) {
    snd_effect_t *effect;
    size_t chan_len, read_len;
    // uint32_t fs_rootbus_dma_ready = 0;
    // uint32_t fs_dma_len = 0;
    uint8_t *tmp_buff = NULL;

    chan_len = len / channels;
    effect = malloc(sizeof(snd_effect_t));

    if(effect == NULL) {
        return SFXHND_INVALID;
    }

    memset(effect, 0, sizeof(snd_effect_t));

    effect->rate = rate;
    effect->stereo = channels > 1;

    switch(bitsize) {
        case 4:
            effect->fmt = AICA_SM_ADPCM;
            effect->len = (len * 2) / channels;
            break;
        case 8:
            effect->fmt = AICA_SM_8BIT;
            effect->len = len / channels;
            break;
        case 16:
            effect->fmt = AICA_SM_16BIT;
            effect->len = (len / 2) / channels;
            break;
        default:
            goto err_occurred;
    }

    if(effect->len > 65534) {
        dbglog(DBG_WARNING, "snd_sfx_load_ex: PCM file is over 65534 samples\n");
    }

    effect->locl = snd_mem_malloc(chan_len);

    if(!effect->locl) {
        goto err_occurred;
    }
    read_len = chan_len;
    /* Uncomment when implementation is merged.
    if(fs_ioctl(fd, IOCTL_FS_ROOTBUS_DMA_READY, &fs_dma_len) == 0) {
        if(chan_len >= fs_dma_len) {
            fs_rootbus_dma_ready = 1;
        }
    }
    if(fs_rootbus_dma_ready) {
        read_len = chan_len & ~(fs_dma_len - 1);

        if(fs_read(fd, (void *)(effect->locl | SPU_RAM_UNCACHED_BASE), read_len) <= 0) {
            goto err_occurred;
        }
        read_len = chan_len - read_len;
    }
    */
    if(read_len > 0) {
        tmp_buff = aligned_alloc(32, read_len);

        if(fs_read(fd, tmp_buff, read_len) <= 0) {
            goto err_occurred;
        }
        spu_memload_sq(effect->locl, tmp_buff, read_len);
    }

    if(channels > 1) {
        effect->locr = snd_mem_malloc(chan_len);

        if(!effect->locr) {
            goto err_occurred;
        }
        read_len = chan_len;
        /* Uncomment when implementation is merged.
        if(fs_rootbus_dma_ready) {
            read_len = chan_len & ~(fs_dma_len - 1);

            if(fs_read(fd, (void *)(effect->locr | SPU_RAM_UNCACHED_BASE), chan_len) <= 0) {
                goto err_occurred;
            }
            read_len = chan_len - read_len;
        }
        */
        if(read_len > 0) {
            if(fs_read(fd, tmp_buff, read_len) <= 0) {
                goto err_occurred;
            }
            spu_memload_sq(effect->locr, tmp_buff, read_len);
        }
    }

    if(tmp_buff) {
        free(tmp_buff);
    }
    LIST_INSERT_HEAD(&snd_effects, effect, list);
    return (sfxhnd_t)effect;

err_occurred:
    if(effect->locl)
        snd_mem_free(effect->locl);
    if(effect->locr)
        snd_mem_free(effect->locr);
    if(tmp_buff)
        free(tmp_buff);

    free(effect);
    return SFXHND_INVALID;
}

/* Load a sound effect from a WAV file and return a handle to it */
sfxhnd_t snd_sfx_load_buf(char *buf) {
    wavhdr_t wavhdr;
    snd_effect_t *effect;
    uint8_t *wav_data;
    uint32_t sample_count;
    size_t bufidx = 0;

    if(!buf) {
        dbglog(DBG_ERROR, "snd_sfx_load_buf: can't read wav data from NULL");
        return SFXHND_INVALID;
    }

    /* Read WAV header */
    if(read_wav_header_buf(buf, &wavhdr, &bufidx) < 0) {
        dbglog(DBG_ERROR, "snd_sfx_load_buf: error reading wav header from buffer %08x\n", (uintptr_t)buf);
        return SFXHND_INVALID;
    }
    /*
    dbglog(DBG_DEBUG, "WAVE file is %s, %luHZ, %d bits/sample, "
        "%u bytes total, format %d\n", 
           wavhdr.fmt.channels == 1 ? "mono" : "stereo", 
           wavhdr.fmt.sample_rate, 
           wavhdr.fmt.sample_size, 
           wavhdr.chunk.size, 
           wavhdr.fmt.format);
    */
    sample_count = wavhdr.fmt.sample_size >= 8 ?
        wavhdr.chunk.size / ((wavhdr.fmt.sample_size / 8) * wavhdr.fmt.channels) :
        (wavhdr.chunk.size * 2) / wavhdr.fmt.channels;

    if(sample_count > 65534) {
        dbglog(DBG_WARNING, "snd_sfx_load: WAVE file is over 65534 samples\n");
    }

    /* Read WAV data */
    wav_data = read_wav_data_buf(buf, &wavhdr, &bufidx);
    /* Caller manages buffer, don't free here */
    if(!wav_data)
        return SFXHND_INVALID;

    /* Create and initialize sound effect */
    effect = create_snd_effect(&wavhdr, wav_data);
    if(!effect) {
        free(wav_data);
        return SFXHND_INVALID;
    }

    /* Finish up and return the sound effect handle */
    free(wav_data);
    LIST_INSERT_HEAD(&snd_effects, effect, list);

    return (sfxhnd_t)effect;
}

sfxhnd_t snd_sfx_load_raw_buf(char *buf, size_t len, uint32_t rate, uint16_t bitsize, uint16_t channels) {
    snd_effect_t *effect;
    size_t chan_len, read_len;
    uint8_t *tmp_buff = NULL;
    size_t bufidx = 0;

    if(!buf) {
        dbglog(DBG_ERROR, "snd_sfx_load_raw_buf: can't read PCM buffer from NULL");
        return SFXHND_INVALID;
    }

    chan_len = len / channels;
    effect = malloc(sizeof(snd_effect_t));

    if(effect == NULL) {
        return SFXHND_INVALID;
    }

    memset(effect, 0, sizeof(snd_effect_t));

    effect->rate = rate;
    effect->stereo = channels > 1;

    switch(bitsize) {
        case 4:
            effect->fmt = AICA_SM_ADPCM;
            effect->len = (len * 2) / channels;
            break;
        case 8:
            effect->fmt = AICA_SM_8BIT;
            effect->len = len / channels;
            break;
        case 16:
            effect->fmt = AICA_SM_16BIT;
            effect->len = (len / 2) / channels;
            break;
        default:
            goto err_occurred;
    }

    if(effect->len > 65534) {
        dbglog(DBG_WARNING, "snd_sfx_load_raw_buf: PCM buffer is over 65534 samples\n");
    }

    effect->locl = snd_mem_malloc(chan_len);

    if(!effect->locl) {
        goto err_occurred;
    }

    read_len = chan_len;
    if(read_len > 0) {
        tmp_buff = aligned_alloc(32, read_len);
        memcpy(tmp_buff, buf, read_len);
        bufidx += read_len;

        spu_memload_sq(effect->locl, tmp_buff, read_len);
    }

    if(channels > 1) {
        effect->locr = snd_mem_malloc(chan_len);

        if(!effect->locr) {
            goto err_occurred;
        }

        read_len = chan_len;
        if(read_len > 0) {
            memcpy(tmp_buff, buf + bufidx, read_len);
            bufidx += read_len;
            spu_memload_sq(effect->locr, tmp_buff, read_len);
        }
    }

    if(tmp_buff) {
        free(tmp_buff);
    }

    LIST_INSERT_HEAD(&snd_effects, effect, list);
    return (sfxhnd_t)effect;

err_occurred:
    if(effect->locl)
        snd_mem_free(effect->locl);
    if(effect->locr)
        snd_mem_free(effect->locr);
    if(tmp_buff)
        free(tmp_buff);

    free(effect);
    return SFXHND_INVALID;
}

int snd_sfx_play_chn(int chn, sfxhnd_t idx, int vol, int pan) {
    sfx_play_data_t data = {0};
    data.chn = chn;
    data.idx = idx;
    data.vol = vol;
    data.pan = pan;
    return snd_sfx_play_ex(&data);
}

int find_free_channel(void) {
    int chn, moved, old;

    /* This isn't perfect.. but it should be good enough. */
    old = irq_disable();
    chn = sfx_nextchan;
    moved = 0;

    while(sfx_inuse & (1ULL << chn)) {
        chn = (chn + 1) % 64;

        if(sfx_nextchan == chn)
            break;

        moved++;
    }

    irq_restore(old);

    if(moved && chn == sfx_nextchan) {
        return -1;
    }

    sfx_nextchan = (chn + 2) % 64;  /* in case of stereo */
    return chn;
}

int snd_sfx_play(sfxhnd_t idx, int vol, int pan) {
    sfx_play_data_t data = {0};
    data.chn = -1;
    data.idx = idx;
    data.vol = vol;
    data.pan = pan;
    return snd_sfx_play_ex(&data);
}

int snd_sfx_play_ex(sfx_play_data_t *data) {
    if (data->chn < 0) {
        data->chn = find_free_channel();
        if (data->chn < 0) {
            return -1;
        }
    }

    int size;
    snd_effect_t *t = (snd_effect_t *)data->idx;
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    size = t->len;

    if(size >= 65535) size = 65534;

    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = data->chn;
    chan->cmd = AICA_CH_CMD_START;
    chan->base = t->locl;
    chan->type = t->fmt;
    chan->length = size;
    chan->loop = data->loop;
    chan->loopstart = data->loopstart;
    chan->loopend = data->loopend ? data->loopend : size;
    chan->freq = data->freq > 0 ? data->freq : t->rate;
    chan->vol = data->vol;

    if(!t->stereo) {
        chan->pan = data->pan;
        snd_sh4_to_aica(tmp, cmd->size);
    }
    else {
        chan->pan = 0;

        snd_sh4_to_aica_stop();
        snd_sh4_to_aica(tmp, cmd->size);

        cmd->cmd_id = data->chn + 1;
        chan->base = t->locr;
        chan->pan = 255;
        snd_sh4_to_aica(tmp, cmd->size);
        snd_sh4_to_aica_start();
    }

    return data->chn;
}

void snd_sfx_stop(int chn) {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = chn;
    chan->cmd = AICA_CH_CMD_STOP;
    chan->base = 0;
    chan->type = 0;
    chan->length = 0;
    chan->loop = 0;
    chan->loopstart = 0;
    chan->loopend = 0;
    chan->freq = 44100;
    chan->vol = 0;
    chan->pan = 0;
    snd_sh4_to_aica(tmp, cmd->size);
}

void snd_sfx_stop_all(void) {
    int i;

    for(i = 0; i < 64; i++) {
        if(sfx_inuse & (1ULL << i))
            continue;

        snd_sfx_stop(i);
    }
}

int snd_sfx_chn_alloc(void) {
    int old, chn;

    old = irq_disable();

    for(chn = 0; chn < 64; chn++)
        if(!(sfx_inuse & (1ULL << chn)))
            break;

    if(chn >= 64)
        chn = -1;
    else
        sfx_inuse |= 1ULL << chn;

    irq_restore(old);

    return chn;
}

void snd_sfx_chn_free(int chn) {
    int old;

    old = irq_disable();
    sfx_inuse &= ~(1ULL << chn);
    irq_restore(old);
}
