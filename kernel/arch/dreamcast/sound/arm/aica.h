#ifndef __AICA_H
#define __AICA_H

#include <stdint.h>

/* flags for aica_play() */
#define AICA_PLAY_DELAY 0x1
#define AICA_PLAY_LOOP  0x2

void aica_init(void);
void aica_play(uint8_t ch, void *data, uint32_t mode,
               uint16_t start, uint16_t end, uint32_t freq,
               uint8_t vol, uint8_t pan, uint32_t flags);
void aica_sync_play(uint64_t chmap);
void aica_stop(uint8_t ch);
void aica_vol(uint8_t ch, uint8_t vol);
void aica_pan(uint8_t ch, uint8_t pan);
void aica_freq(uint8_t ch, uint32_t freq);
uint16_t aica_get_pos(uint8_t ch);

#endif  /* __AICA_H */

