#ifndef __AICA_H
#define __AICA_H

/* volatile unsigned char *dc_snd_base = (unsigned char *)0x00800000; */
#define dc_snd_base ((volatile unsigned char *)0x00800000)

/* Some convenience macros */
#define SNDREG32A(x) ((volatile unsigned long *)(dc_snd_base + (x)))
#define SNDREG32(x) (*SNDREG32A(x))
#define SNDREG8A(x) (dc_snd_base + (x))
#define SNDREG8(x) (*SNDREG8A(x))
#define CHNREG32A(chn, x) SNDREG32A(0x80*(chn) + (x))
#define CHNREG32(chn, x) (*CHNREG32A(chn, x))
#define CHNREG8A(chn, x) SNDREG8A(0x80*(chn) + (x))
#define CHNREG8(chn, x) (*CHNREG8A(chn, x))

void aica_init(void);
void aica_play(int ch, int delay);
void aica_sync_play(uint32 chmap);
void aica_stop(int ch);
void aica_vol(int ch);
void aica_pan(int ch);
void aica_freq(int ch);
int aica_get_pos(int ch);

#endif  /* __AICA_H */

