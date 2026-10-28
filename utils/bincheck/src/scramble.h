/* Sega Dreamcast Binary Checker (bincheck)
 *
 * Copyright (C) 2026 The KOS Team and contributors.
 * All rights reserved.
 *
 * This code was contributed to KallistiOS (KOS) by SiZiOUS. It is a rewrite
 * of Binary Checker (BinCheck), originally made by SiZiOUS in 2005. The
 * scrambling algorithm is based on scramble.c by Marcus Comstedt (zeldin).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SCRAMBLE_H
#define SCRAMBLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DC_SCRAMBLE = 0,
    DC_DESCRAMBLE = 1
} dc_scramble_dir_t;

/* Scramble a linear (unscrambled) buffer into dst.
 * `src` and `dst` must both be `size` bytes and must NOT overlap. */
bool dc_scramble_buffer(const uint8_t *src, uint8_t *dst, size_t size);

/* Descramble a scrambled buffer into dst (linear/unscrambled).
 * `src` and `dst` must both be `size` bytes and must NOT overlap. */
bool dc_descramble_buffer(const uint8_t *src, uint8_t *dst, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SCRAMBLE_H */
