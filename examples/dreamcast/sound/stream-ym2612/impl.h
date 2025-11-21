#pragma once

#include <stdint.h>

#include "chips/typedefs.h"


void ym2612_stream_update(int16_t* buffer, int samples);
void ym2612_stream_update_ex(int16_t** buffer, int buffer_offset, int length, uint8_t* panning);

int device_start_ym2612(int clock);
void device_stop_ym2612();
void device_reset_ym2612();

void ym2612_w(offs_t offset, UINT8 data);

void ym2612_set_options(UINT8 Flags);
void ym2612_set_mute_mask(UINT32 MuteMask);
