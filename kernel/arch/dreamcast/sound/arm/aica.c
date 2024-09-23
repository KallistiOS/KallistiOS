/* KallistiOS ##version##

   aica.c
   (c)2000-2002 Megan Potter
   (c)2024 Stefanos Kornilios Mitsis Poiitidis

   ARM support routines for using the wavetable channels
*/

#include "aica_cmd_iface.h"
#include "aica.h"

extern volatile aica_channel_t *chans;

void aica_init(void) {
    int i, j;

    /* Initialize AICA channels */
    SNDREG32(0x2800) = 0x0000;

    for(i = 0; i < 64; i++) {
        CHNREG32(i, 0) = 0x8000;

        for(j = 4; j < 0x80; j += 4)
            CHNREG32(i, j) = 0;

        CHNREG32(i, 20) = 0x1f;
    }

    SNDREG32(0x2800) = 0x000f;
}

/* Translates a volume from linear form to logarithmic form (required by
   the AICA chip

    Calculated by
        for (int i = 0; i < 256; i++)
            if (i == 0)
                logs[i] = 255;
            else
                logs[i] = 16.0 * log2(255.0 / i);
   */
static uint8 logs[] = {
    255, 127, 111, 102, 95, 90, 86, 82, 79, 77, 74, 72, 70, 68, 66, 65,
    63, 62, 61, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 50, 49, 48,
    47, 47, 46, 45, 45, 44, 43, 43, 42, 42, 41, 41, 40, 40, 39, 39,
    38, 38, 37, 37, 36, 36, 35, 35, 34, 34, 34, 33, 33, 33, 32, 32,
    31, 31, 31, 30, 30, 30, 29, 29, 29, 28, 28, 28, 27, 27, 27, 27,
    26, 26, 26, 25, 25, 25, 25, 24, 24, 24, 24, 23, 23, 23, 23, 22,
    22, 22, 22, 21, 21, 21, 21, 20, 20, 20, 20, 20, 19, 19, 19, 19,
    18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16,
    15, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13,
    13, 13, 12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 11, 10,
    10, 10, 10, 10, 10, 10, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8,
    8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6,
    6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static inline uint8 calc_aica_vol(uint8 x) {
    return logs[x];
}

static inline int calc_aica_pan(int x) {
    if(x == 0x80)
        return 0;
    else if(x < 0x80) {
        return 0x10 | ((0x7f - x) >> 3);
    }
    else {
        return (x - 0x80) >> 3;
    }
}

/* Sets up a sound channel completely. This is generally good if you want
   a quick and dirty way to play notes. If you want a more comprehensive
   set of routines (more like PC wavetable cards) see below.

   ch is the channel to play on (0 - 63)
   smpptr is the pointer to the sound data; if you're running off the
     SH4, then this ought to be (ptr - 0xa0800000); otherwise it's just
     ptr. Basically, it's an offset into sound ram.
   mode is one of the mode constants (16 bit, 8 bit, ADPCM)
   nsamp is the number of samples to play (not number of bytes!)
   freq is the sampling rate of the sound
   vol is the volume, 0 to 0xff (0xff is louder)
   pan is a panning constant -- 0 is left, 128 is center, 255 is right.

   This routine (and the similar ones) owe a lot to Marcus' sound example --
   I hadn't gotten quite this far into dissecting the individual regs yet. */
void aica_play(int ch, int delay) {
    uint32 smpptr   = chans[ch].base;
    uint32 mode     = chans[ch].type;
    uint32 loopst   = chans[ch].loopstart;
    uint32 loopend  = chans[ch].loopend;
    uint32 freq     = chans[ch].freq;
    uint32 vol      = chans[ch].vol;
    uint32 pan      = chans[ch].pan;
    uint32 loopflag = chans[ch].loop;

    uint32 freq_lo, freq_base = 5644800;
    int freq_hi = 7;
    uint32 playCont;

    /* Stop the channel (if it's already playing) */
    aica_stop(ch);

    /* Need to convert frequency to floating point format
       (freq_hi is exponent, freq_lo is mantissa)
       Formula is freq = 44100*2^freq_hi*(1+freq_lo/1024) */
    while(freq < freq_base && freq_hi > -8) {
        freq_base >>= 1;
        --freq_hi;
    }

    freq_lo = (freq << 10) / freq_base;

    /* Envelope setup. The first of these is the loop point,
       e.g., where the sample starts over when it loops. The second
       is the loop end. This is the full length of the sample when
       you are not looping, or the loop end point when you are (though
       storing more than that is a waste of memory if you're not doing
       volume enveloping). */
    CHNREG32(ch, 8) = loopst & 0xffff;
    CHNREG32(ch, 12) = loopend & 0xffff;

    /* Write resulting values */
    CHNREG32(ch, 24) = (freq_hi << 11) | (freq_lo & 1023);

    /* Convert the incoming pan into a hardware value and set it */
    CHNREG8(ch, 36) = calc_aica_pan(pan);
    CHNREG8(ch, 37) = 0xf;
    /* turn off Low Pass Filter (LPF) */
    CHNREG8(ch, 40) = 0x24;
    /* Convert the incoming volume into a hardware value and set it */
    CHNREG8(ch, 41) = calc_aica_vol(vol);

    /* If we supported volume envelopes (which we don't yet) then
       this value would set that up. The top 4 bits determine the
       envelope speed. f is the fastest, 1 is the slowest, and 0
       seems to be an invalid value and does weird things). The
       default (below) sets it into normal mode (play and terminate/loop).
    CHNREG32(ch, 16) = 0xf010;
    */
    CHNREG32(ch, 16) = 0x1f;    /* No volume envelope */


    /* Set sample format, buffer address, and looping control. If
       0x0200 mask is set on reg 0, the sample loops infinitely. If
       it's not set, the sample plays once and terminates. We'll
       also set the bits to start playback here. */
    CHNREG32(ch, 4) = smpptr & 0xffff;
    playCont = (mode << 7) | (smpptr >> 16);

    if(loopflag)
        playCont |= 0x0200;

    if(delay) {
        CHNREG32(ch, 0) = playCont;         /* key off */
    }
    else {
        CHNREG32(ch, 0) = 0xc000 | playCont;    /* key on */
    }
}

/* Start sound on all channels specified by chmap bitmap */
void aica_sync_play(uint32 chmap) {
    int i = 0;

    while(chmap) {
        if(chmap & 0x1)
            CHNREG32(i, 0) = CHNREG32(i, 0) | 0xc000;

        i++;
        chmap >>= 1;
    }
}

/* Stop the sound on a given channel */
void aica_stop(int ch) {
    CHNREG32(ch, 0) = (CHNREG32(ch, 0) & ~0x4000) | 0x8000;
}


/* The rest of these routines can change the channel in mid-stride so you
   can do things like vibrato and panning effects. */

/* Set channel volume */
void aica_vol(int ch) {
    CHNREG8(ch, 41) = calc_aica_vol(chans[ch].vol);
}

/* Set channel pan */
void aica_pan(int ch) {
    CHNREG8(ch, 36) = calc_aica_pan(chans[ch].pan);
}

/* Set channel frequency */
void aica_freq(int ch) {
    uint32 freq = chans[ch].freq;
    uint32 freq_lo, freq_base = 5644800;
    int freq_hi = 7;

    while(freq < freq_base && freq_hi > -8) {
        freq_base >>= 1;
        freq_hi--;
    }

    freq_lo = (freq << 10) / freq_base;
    CHNREG32(ch, 24) = (freq_hi << 11) | (freq_lo & 1023);
}

/* Get channel position */
int aica_get_pos(int ch) {
    int i;

    /* Observe channel ch */
    SNDREG8(0x280d) = ch;

    /* Wait a while */
    for(i = 0; i < 20; i++)
        __asm__ volatile ("nop");  /* Prevent loop from being optimized out */

    /* Update position counters */
    chans[ch].pos = SNDREG32(0x2814) & 0xffff;

    return chans[ch].pos;
}
