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

/**
 * scramble - faithful port of scramble.c (Marcus Comstedt / KallistiOS,
 * utils/scramble): Fisher-Yates shuffle of 32-byte slices, in chunks that
 * shrink from 2 MB down to 32 bytes, with a PRNG seeded by the file size.
 *
 * The algorithm is fully deterministic (it depends only on the file size),
 * so scramble()/descramble() can be computed exactly, with no guesswork.
 * Validated by round-trip on real samples (loader.bin <-> 1st_read.bin,
 * dcload-ip): byte-exact in both directions.
 */

#include "scramble.h"

#include <stdlib.h>
#include <string.h>

#define SC_SLICE      32u
#define SC_MAXCHUNK   (2048u * 1024u)

typedef struct {
    uint32_t seed;
} prng_t;

static void
prng_seed(prng_t *p, uint32_t n)
{
    p->seed = n & 0xffffu;
}

static uint32_t
prng_next(prng_t *p)
{
    p->seed = (p->seed * 2109u + 9273u) & 0x7fffu;
    return (p->seed + 0xc000u) & 0xffffu;
}

/* Build the slice sequence in "file" order (the order used by
 * save_chunk/load_chunk in the original) for a chunk of `nslices` slices.
 * seq[k] is the index, in the linear (unscrambled) buffer, of the k-th
 * slice written/read sequentially. Standard Fisher-Yates, with values
 * taken as the shuffle progresses (as in the original). */
static void
build_chunk_sequence(prng_t *p, uint32_t *idx, uint32_t *seq, uint32_t nslices)
{
    uint32_t i, k;

    for (i = 0; i < nslices; i++) {
        idx[i] = i;
    }

    k = 0;
    for (i = nslices; i-- > 0; ) {
        uint32_t x = ((uint32_t)prng_next(p) * i) >> 16;
        uint32_t tmp = idx[i];
        idx[i] = idx[x];
        idx[x] = tmp;
        seq[k++] = idx[i];
    }
}

/* Apply the transformation (scramble or descramble) to a whole buffer.
 * direction is DC_SCRAMBLE or DC_DESCRAMBLE. dst must be distinct from src,
 * since reads and writes hit different positions within the same chunk. */
static bool
transform_buffer(const uint8_t *src, uint8_t *dst, size_t size, dc_scramble_dir_t direction)
{
    prng_t p;
    uint32_t *idx = NULL, *seq = NULL;
    uint64_t remaining;
    size_t base;
    uint32_t chunkbytes;

    if (!src || !dst || size == 0) return false;

    idx = (uint32_t *)malloc((SC_MAXCHUNK / SC_SLICE) * sizeof(uint32_t));
    seq = (uint32_t *)malloc((SC_MAXCHUNK / SC_SLICE) * sizeof(uint32_t));
    if (!idx || !seq) {
        free(idx);
        free(seq);
        return false;
    }

    prng_seed(&p, (uint32_t)size);

    remaining = size;
    base = 0;

    for (chunkbytes = SC_MAXCHUNK; chunkbytes >= SC_SLICE; chunkbytes >>= 1) {
        while (remaining >= chunkbytes) {
            uint32_t nslices = chunkbytes / SC_SLICE;
            uint32_t k;

            build_chunk_sequence(&p, idx, seq, nslices);

            if (direction == DC_SCRAMBLE) {
                /* sequential position k <- source slice seq[k] */
                for (k = 0; k < nslices; k++) {
                    memcpy(dst + base + (size_t)k * SC_SLICE,
                           src + base + (size_t)seq[k] * SC_SLICE,
                           SC_SLICE);
                }
            } else {
                /* destination slice seq[k] <- sequential position k */
                for (k = 0; k < nslices; k++) {
                    memcpy(dst + base + (size_t)seq[k] * SC_SLICE,
                           src + base + (size_t)k * SC_SLICE,
                           SC_SLICE);
                }
            }

            base += chunkbytes;
            remaining -= chunkbytes;
        }
    }

    /* Trailing remainder (< 32 bytes) is copied as-is, not shuffled */
    if (remaining > 0) {
        memcpy(dst + base, src + base, (size_t)remaining);
    }

    free(idx);
    free(seq);
    return true;
}

bool
dc_scramble_buffer(const uint8_t *src, uint8_t *dst, size_t size)
{
    return transform_buffer(src, dst, size, DC_SCRAMBLE);
}

bool
dc_descramble_buffer(const uint8_t *src, uint8_t *dst, size_t size)
{
    return transform_buffer(src, dst, size, DC_DESCRAMBLE);
}
