/*
   AICAOS

   aica.h
   Copyright (C) 2000-2002 Megan Potter
   Copyright (C) 2024 Stefanos Kornilios Mitsis Poiitidis
   Copyright (C) 2025 Paul Cercueil

   ARM support routines for using the wavetable channels
*/

#ifndef __AICAOS_AICA_H
#define __AICAOS_AICA_H

#include <stdbool.h>
#include <stdint.h>

/* Old API below. All of these will eventually be trashed. */

void aica_play(uint8_t ch, bool delay);
void aica_sync_play(uint32_t chmap);
void aica_vol(uint8_t ch);
void aica_pan(uint8_t ch);
void aica_freq(uint8_t ch);
uint16_t aica_get_pos(uint8_t ch);

/* New API below. */

struct aica_channel_data;

void aica_update(uint8_t chn, const struct aica_channel_data *data);
void aica_start(uint8_t chn);
void aica_stop(uint8_t chn);

uint8_t aica_reserve_channel(void);
void aica_unreserve_channel(uint8_t ch);

/* Read the hardware value of the counter. */
uint16_t aica_read_counter(void);

#endif  /* __AICAOS_AICA_H */

