/* KallistiOS ##version##

   snd_sfxmgr.c
   Copyright (C) 2000, 2001, 2002, 2003, 2004 Megan Potter
   Copyright (C) 2023 Ruslan Rostovtsev

   Sound effects management system; this thing loads and plays sound effects
   during game operation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include <sys/queue.h>
#include <kos/fs.h>
#include <arch/irq.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

#include "arm/aica_cmd_iface.h"

struct snd_effect;
LIST_HEAD(selist, snd_effect);

typedef struct snd_effect {
    uint32  locl, locr;
    uint32  len;
    uint32  rate;
    uint32  used;
    uint16  stereo;
    uint32  fmt;

    LIST_ENTRY(snd_effect)  list;
} snd_effect_t;

struct selist snd_effects;

/* The next channel we'll use to play sound effects. */
static int sfx_nextchan = 0;

/* Our channel-in-use mask. */
static uint64 sfx_inuse = 0;

/* Unload all loaded samples and free their SPU RAM */
void snd_sfx_unload_all(void) {
    snd_effect_t * t, * n;

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
    snd_effect_t * t = (snd_effect_t *)idx;

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

/* WAV header:
    0x08    -- "WAVE"
    0x14    -- 1 for PCM, 20 for ADPCM
    0x16    -- short num channels (1/2)
    0x18    -- long  HZ
    0x22    -- short 8 or 16 (bits)
    0x28    -- long  data length
    0x2c    -- data start

 */

/* WAV sample formats */
#define WAVE_FMT_PCM                   0x0001 /* PCM */
#define WAVE_FMT_YAMAHA_ADPCM_ITU_G723 0x0014 /* ITU G.723 Yamaha ADPCM (KallistiOS) */
#define WAVE_FMT_YAMAHA_ADPCM          0x0020 /* Yamaha ADPCM (ffmpeg) */

/* Load a sound effect from a WAV file and return a handle to it */
sfxhnd_t snd_sfx_load(const char *fn) {
    file_t fd;
    uint32_t len, hz, rd;
    uint8_t *tmp;
    uint16_t channels, bitsize, fmt;
    snd_effect_t *t;

    dbglog(DBG_DEBUG, "snd_sfx: loading effect %s\n", fn);

    fd = fs_open(fn, O_RDONLY);

    if(fd <= FILEHND_INVALID) {
        dbglog(DBG_WARNING, "snd_sfx: can't open sfx %s\n", fn);
        return SFXHND_INVALID;
    }

    /* Check file magic */
    hz = 0;
    fs_seek(fd, 0x08, SEEK_SET);
    fs_read(fd, &hz, 4);

    if(strncmp((char *)&hz, "WAVE", 4)) {
        dbglog(DBG_WARNING, "snd_sfx: file is not RIFF WAVE\n");
        fs_close(fd);
        return SFXHND_INVALID;
    }

    /* Read WAV header info */
    fs_seek(fd, 0x14, SEEK_SET);
    fs_read(fd, &fmt, 2);
    fs_read(fd, &channels, 2);
    fs_read(fd, &hz, 4);
    fs_seek(fd, 0x22, SEEK_SET);
    fs_read(fd, &bitsize, 2);

    /* Read WAV data */
    fs_seek(fd, 0x28, SEEK_SET);
    fs_read(fd, &len, 4);

    dbglog(DBG_DEBUG, "WAVE file is %s, %luHZ, %d bits/sample, %lu bytes total,"
           " format %d\n", channels == 1 ? "mono" : "stereo", hz, bitsize, len, fmt);

    tmp = (uint8_t *)memalign(32, len);

    if(tmp == NULL) {
        fs_close(fd);
        return SFXHND_INVALID;
    }

    rd = fs_read(fd, tmp, len);
    fs_close(fd);

    if (rd != len) {
        dbglog(DBG_WARNING, "snd_sfx: file has not been fully read.\n");
    }

    t = malloc(sizeof(snd_effect_t));

    if(t == NULL) {
        free(tmp);
        return SFXHND_INVALID;
    }

    memset(t, 0, sizeof(snd_effect_t));

    /* Common characteristics not impacted by stream type */
    t->rate = hz;
    t->stereo = channels - 1;

    if(channels == 1) {
        /* Mono PCM/ADPCM */
        if(fmt == WAVE_FMT_YAMAHA_ADPCM_ITU_G723 || fmt == WAVE_FMT_YAMAHA_ADPCM) {
            t->fmt = AICA_SM_ADPCM;
            t->len = len * 2; /* 4-bit packed samples */
        }
        else if(fmt == WAVE_FMT_PCM && bitsize == 8) {
            t->fmt = AICA_SM_8BIT;
            t->len = len;
        }
        else if(fmt == WAVE_FMT_PCM && bitsize == 16) {
            t->fmt = AICA_SM_16BIT;
            t->len = len / 2;
        } else {
            goto err_exit;
        }
        t->locl = snd_mem_malloc(len);

        if(t->locl)
            spu_memload_sq(t->locl, tmp, len);

        t->locr = 0;
    }
    else if(channels == 2 && fmt == WAVE_FMT_PCM && bitsize == 16) {
        /* Stereo 16-bit PCM */
        t->len = len / 4; /* Two stereo, 16-bit samples */
        t->fmt = AICA_SM_16BIT;
        t->locl = snd_mem_malloc(len / 2);
        t->locr = snd_mem_malloc(len / 2);

        if(t->locl && t->locr)
            snd_pcm16_split_sq((uint32_t *)tmp, t->locl, t->locr, len);
    }
    else if(channels == 2 && fmt == WAVE_FMT_PCM && bitsize == 8) {
        /* Stereo 8-bit PCM */
        uint32_t *left_buf = (uint32_t *)memalign(32, len / 2);

        if(left_buf == NULL) {
            goto err_exit;
        }
        uint32_t *right_buf = (uint32_t *)memalign(32, len / 2);

        if(right_buf == NULL) {
            free(left_buf);
            goto err_exit;
        }
        snd_pcm8_split((uint32_t *)tmp, left_buf, right_buf, len);

        t->fmt = AICA_SM_8BIT;
        t->len = len / 2;
        t->locl = snd_mem_malloc(len / 2);
        t->locr = snd_mem_malloc(len / 2);

        if(t->locl)
            spu_memload_sq(t->locl, left_buf, len / 2);

        if(t->locr)
            spu_memload_sq(t->locr, right_buf, len / 2);

        free(left_buf);
        free(right_buf);
    }
    else if(channels == 2 && fmt == WAVE_FMT_YAMAHA_ADPCM_ITU_G723) {
        /* Stereo ADPCM ITU G.723 (channels are not interleaved) */
        uint8_t *right_buf = tmp + (len / 2);
        int ownmem = 0;

        if(((uintptr_t)right_buf) & 3) {
            right_buf = (uint8_t *)memalign(32, len / 2);
            ownmem = 1;

            if(right_buf == NULL) {
                goto err_exit;
            }
            memcpy(right_buf, tmp + (len / 2), len / 2);
        }

        t->len = len;   /* Two stereo, 4-bit samples */
        t->fmt = AICA_SM_ADPCM;
        t->locl = snd_mem_malloc(len / 2);
        t->locr = snd_mem_malloc(len / 2);

        if(t->locl)
            spu_memload_sq(t->locl, tmp, len / 2);

        if(t->locr)
            spu_memload_sq(t->locr, right_buf, len / 2);

        if(ownmem)
            free(right_buf);
    }
    else if(channels == 2 && fmt == WAVE_FMT_YAMAHA_ADPCM) {
        /* Stereo Yamaha ADPCM (channels are interleaved) */
        uint32_t *left_buf = (uint32_t *)memalign(32, len / 2);

        if(left_buf == NULL) {
            goto err_exit;
        }
        uint32_t *right_buf = (uint32_t *)memalign(32, len / 2);

        if(right_buf == NULL) {
            free(left_buf);
            goto err_exit;
        }
        snd_adpcm_split((uint32_t *)tmp, left_buf, right_buf, len);

        t->len = len; /* Two stereo, 4-bit samples */
        t->fmt = AICA_SM_ADPCM;
        t->locl = snd_mem_malloc(len / 2);
        t->locr = snd_mem_malloc(len / 2);

        if(t->locl)
            spu_memload_sq(t->locl, left_buf, len / 2);

        if(t->locr)
            spu_memload_sq(t->locr, right_buf, len / 2);

        free(left_buf);
        free(right_buf);
    }
    else {
        free(t);
        t = SFXHND_INVALID;
    }

    free(tmp);

    if(t != SFXHND_INVALID)
        LIST_INSERT_HEAD(&snd_effects, t, list);

    return (sfxhnd_t)t;

err_exit:
    free(tmp);
    free(t);
    return SFXHND_INVALID;
}

int snd_sfx_play_chn(int chn, sfxhnd_t idx, int vol, int pan) {
    int size;
    snd_effect_t * t = (snd_effect_t *)idx;
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    size = t->len;

    if(size >= 65535) size = 65534;

    cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp = 0;
    cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id = chn;
    chan->cmd = AICA_CH_CMD_START;
    chan->base = t->locl;
    chan->type = t->fmt;
    chan->length = size;
    chan->loop = 0;
    chan->loopstart = 0;
    chan->loopend = size;
    chan->freq = t->rate;
    chan->vol = vol;

    if(!t->stereo) {
        chan->pan = pan;
        snd_sh4_to_aica(tmp, cmd->size);
    }
    else {
        chan->pan = 0;

        snd_sh4_to_aica_stop();
        snd_sh4_to_aica(tmp, cmd->size);

        cmd->cmd_id = chn + 1;
        chan->base = t->locr;
        chan->pan = 255;
        snd_sh4_to_aica(tmp, cmd->size);
        snd_sh4_to_aica_start();
    }

    return chn;
}

int snd_sfx_play(sfxhnd_t idx, int vol, int pan) {
    int chn, moved, old;

    /* This isn't perfect.. but it should be good enough. */
    old = irq_disable();
    chn = sfx_nextchan;
    moved = 0;

    while(sfx_inuse & (1 << chn)) {
        chn = (chn + 1) % 64;

        if(sfx_nextchan == chn)
            break;

        moved++;
    }

    irq_restore(old);

    if(moved && chn == sfx_nextchan) {
        return -1;
    }
    else {
        sfx_nextchan = (chn + 2) % 64;  // in case of stereo
        return snd_sfx_play_chn(chn, idx, vol, pan);
    }
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
        if(sfx_inuse & (1 << i))
            continue;

        snd_sfx_stop(i);
    }
}

int snd_sfx_chn_alloc(void) {
    int old, chn;

    old = irq_disable();

    for(chn = 0; chn < 64; chn++)
        if(!(sfx_inuse & (1 << chn)))
            break;

    if(chn >= 64)
        chn = -1;
    else
        sfx_inuse |= 1 << chn;

    irq_restore(old);

    return chn;
}

void snd_sfx_chn_free(int chn) {
    int old;

    old = irq_disable();
    sfx_inuse &= ~(1 << chn);
    irq_restore(old);
}
