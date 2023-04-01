/* KallistiOS ##version##

   snd_sfxmgr.c
   Copyright (C) 2000, 2001, 2002, 2003, 2004 Megan Potter

   Sound effects management system; this thing loads and plays sound effects
   during game operation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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
void snd_sfx_unload_all() {
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

/* Load a sound effect from a WAV file and return a handle to it */
sfxhnd_t snd_sfx_load(const char *fn) {
    file_t  fd;
    uint32  len, hz;
    uint16  *tmp, stereo, bitsize, fmt;
    snd_effect_t *t;
    int ownmem;

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

    if(strncmp((char*)&hz, "WAVE", 4)) {
        dbglog(DBG_WARNING, "snd_sfx: file is not RIFF WAVE\n");
        fs_close(fd);
        return SFXHND_INVALID;
    }

    /* Read WAV header info */
    fs_seek(fd, 0x14, SEEK_SET);
    fs_read(fd, &fmt, 2);
    fs_read(fd, &stereo, 2);
    fs_read(fd, &hz, 4);
    fs_seek(fd, 0x22, SEEK_SET);
    fs_read(fd, &bitsize, 2);

    /* Read WAV data */
    fs_seek(fd, 0x28, SEEK_SET);
    fs_read(fd, &len, 4);

    dbglog(DBG_DEBUG, "WAVE file is %s, %luHZ, %d bits/sample, %lu bytes total,"
           " format %d\n", stereo == 1 ? "mono" : "stereo", hz, bitsize, len, fmt);

    /* Try to mmap it and if that works, no need to copy it again */
    ownmem = 0;
    tmp = (uint16 *)fs_mmap(fd);

    if(!tmp) {
        tmp = malloc(len);
        fs_read(fd, tmp, len);
        ownmem = 1;
    }
    else {
        tmp = (uint16 *)(((uint8 *)tmp) + fs_tell(fd));
    }

    fs_close(fd);

    t = malloc(sizeof(snd_effect_t));
    memset(t, 0, sizeof(snd_effect_t));
    
    /* Common characteristics not impacted by stream type */
    t->rate = hz;
    t->stereo = stereo - 1;

    if(stereo == 1) {
        /* Mono PCM/ADPCM */
        t->len = len / 2; /* 16-bit samples */
        t->rate = hz;
        t->locl = snd_mem_malloc(len);

        if(t->locl)
            spu_memload(t->locl, tmp, len);

        t->locr = 0;
        t->stereo = 0;

        if(fmt == 20) {
            t->fmt = AICA_SM_ADPCM;
            t->len *= 4;    /* 4-bit packed samples */
        }
        else
            t->fmt = AICA_SM_16BIT;
    }
    else if(stereo == 2 && fmt == 1) {
        /* Stereo PCM */
        uint32 i;
        uint16 * sepbuf;

        sepbuf = malloc(len / 2);

        for(i = 0; i < len / 2; i += 2) {
            sepbuf[i / 2] = tmp[i + 1];
        }

        for(i = 0; i < len / 2; i += 2) {
            tmp[i / 2] = tmp[i];
        }

        t->len = len / 4; /* Two stereo, 16-bit samples */
        t->rate = hz;
        t->locl = snd_mem_malloc(len / 2);
        t->locr = snd_mem_malloc(len / 2);

        if(t->locl)
            spu_memload(t->locl, tmp, len / 2);

        if(t->locr)
            spu_memload(t->locr, sepbuf, len / 2);

        t->stereo = 1;
        t->fmt = AICA_SM_16BIT;

        free(sepbuf);
    }
    else if(stereo == 2 && fmt == 20) {
        /* Stereo ADPCM */

        /* We have to be careful here, because the second sample might not
           start on a nice even dword boundary. We take the easy way out
           and just malloc a second buffer. */
        uint8 * buf2 = malloc(len / 2);
        memcpy(buf2, ((uint8*)tmp) + len / 2, len / 2);

        t->len = len;   /* Two stereo, 4-bit samples */
        t->rate = hz;
        t->locl = snd_mem_malloc(len / 2);
        t->locr = snd_mem_malloc(len / 2);

        if(t->locl)
            spu_memload(t->locl, tmp, len / 2);

        if(t->locr)
            spu_memload(t->locr, buf2, len / 2);

        t->stereo = 1;
        t->fmt = AICA_SM_ADPCM;

        free(buf2);
    }
    else {
        free(t);
        t = SFXHND_INVALID;
    }

    if(ownmem)
        free(tmp);

    if(t != SFXHND_INVALID) 
        LIST_INSERT_HEAD(&snd_effects, t, list);

    return (sfxhnd_t)t;
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

void snd_sfx_stop_all() {
    int i;

    for(i = 0; i < 64; i++) {
        if(sfx_inuse & (1 << i))
            continue;

        snd_sfx_stop(i);
    }
}

int snd_sfx_chn_alloc() {
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
