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

#include "check.h"
#include "scramble.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void
bincheck_options_init(bincheck_options_t *opts)
{
    if (!opts) return;
    opts->entropy_block   = BINCHECK_DEFAULT_ENTROPY_BLOCK;
    opts->variance_margin = BINCHECK_DEFAULT_VARIANCE_MARGIN;
}

/* SH-4 instructions used by GCC to save and restore the return address in
 * the prologue and epilogue of every non-leaf function. */
#define SH4_OP_STS_L_PR  0x4f22u  /* sts.l pr,@-r15 */
#define SH4_OP_LDS_L_PR  0x4f26u  /* lds.l @r15+,pr */

static bool
is_elf_file(const uint8_t *buf, size_t size)
{
    return size >= 4 && buf[0] == 0x7f && buf[1] == 'E'
        && buf[2] == 'L' && buf[3] == 'F';
}

/* Check whether the buffer contains SH-4 code, by counting the function
 * prologue/epilogue instructions above. Scrambling moves whole 32-byte
 * slices, so these 16-bit instructions are intact in both states and no
 * descrambling is needed. Both instructions must be present, and their
 * combined density must reach BINCHECK_SH4_MIN_SCORE per 10,000 words. */
static bool
looks_like_sh4_code(const uint8_t *buf, size_t size, double *out_score)
{
    size_t nwords = size / 2, i;
    uint32_t sts = 0, lds = 0;

    for (i = 0; i < nwords; i++) {
        uint16_t op = (uint16_t)(buf[2 * i] | (buf[2 * i + 1] << 8));
        if (op == SH4_OP_STS_L_PR) sts++;
        else if (op == SH4_OP_LDS_L_PR) lds++;
    }

    *out_score = (nwords > 0) ? (double)(sts + lds) * 10000.0 / (double)nwords : 0.0;
    return sts > 0 && lds > 0 && *out_score >= BINCHECK_SH4_MIN_SCORE;
}

static double
shannon_entropy(const uint8_t *buf, size_t size)
{
    uint32_t counts[256] = {0};
    size_t i;
    double entropy = 0.0;

    if (size == 0) return 0.0;
    for (i = 0; i < size; i++) counts[buf[i]]++;
    for (i = 0; i < 256; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] / (double)size;
        entropy -= p * (log(p) / log(2.0));
    }
    return entropy;
}

/* Population variance of the Shannon entropy of consecutive
 * `block_size`-byte blocks over the whole buffer. */
static double
block_entropy_variance(const uint8_t *buf, size_t size, uint32_t block_size)
{
    size_t nblocks, i;
    double *vals;
    double mean = 0.0, var = 0.0;

    if (block_size == 0 || size < block_size) block_size = (uint32_t)size;
    if (block_size == 0) return 0.0;

    nblocks = (size + block_size - 1) / block_size;
    vals = (double *)malloc(nblocks * sizeof(double));
    if (!vals) return 0.0;

    for (i = 0; i < nblocks; i++) {
        size_t off = i * block_size;
        size_t len = (off + block_size <= size) ? block_size : (size - off);
        vals[i] = shannon_entropy(buf + off, len);
        mean += vals[i];
    }
    mean /= (double)nblocks;

    for (i = 0; i < nblocks; i++) {
        double d = vals[i] - mean;
        var += d * d;
    }
    var /= (double)nblocks;

    free(vals);
    return var;
}

bool
bincheck_analyze_buffer(const uint8_t *buf, size_t size,
                        const bincheck_options_t *opts,
                        bincheck_result_t *out_result)
{
    bincheck_options_t default_opts;
    uint8_t *unscrambled = NULL;
    bool ok;

    if (!buf || size < 2 || !out_result) return false;

    if (!opts) {
        bincheck_options_init(&default_opts);
        opts = &default_opts;
    }

    /* Sanity checks: the verdict is only meaningful for raw SH-4 binaries.
     * They do not change the verdict; the caller decides how to report. */
    out_result->is_elf         = is_elf_file(buf, size);
    out_result->looks_like_sh4 = looks_like_sh4_code(buf, size, &out_result->sh4_score);

    unscrambled = (uint8_t *)malloc(size);
    if (!unscrambled) return false;

    ok = dc_descramble_buffer(buf, unscrambled, size);
    if (!ok) {
        free(unscrambled);
        return false;
    }

    /* Decision signal: per-block entropy variance, as-is vs after the real
     * descramble (see check.h for why this signal is used). */
    out_result->variance_as_is       = block_entropy_variance(buf, size, opts->entropy_block);
    out_result->variance_unscrambled = block_entropy_variance(unscrambled, size, opts->entropy_block);

    {
        double denom = (out_result->variance_as_is > 1e-9) ? out_result->variance_as_is : 1e-9;
        double relative_increase = (out_result->variance_unscrambled - out_result->variance_as_is) / denom;
        out_result->is_scrambled = relative_increase > opts->variance_margin;
    }

    free(unscrambled);
    return true;
}

bool
bincheck_analyze_file(const char *filename,
                      const bincheck_options_t *opts,
                      bincheck_result_t *out_result)
{
    FILE *f;
    long file_size;
    uint8_t *buf;
    bool ok;

    if (!filename || !out_result) return false;

    f = fopen(filename, "rb");
    if (!f) return false;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    file_size = ftell(f);
    if (file_size <= 0) { fclose(f); return false; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }

    buf = (uint8_t *)malloc((size_t)file_size);
    if (!buf) { fclose(f); return false; }

    if (fread(buf, 1, (size_t)file_size, f) != (size_t)file_size) {
        free(buf);
        fclose(f);
        return false;
    }
    fclose(f);

    ok = bincheck_analyze_buffer(buf, (size_t)file_size, opts, out_result);
    free(buf);
    return ok;
}
