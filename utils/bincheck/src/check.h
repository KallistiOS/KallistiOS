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
 * check - scrambled/unscrambled detection for Sega Dreamcast
 * binaries (1ST_READ.BIN), built on the real (de)scramble algorithm
 * (Fisher-Yates over 32-byte slices, seeded by the file size; see
 * scramble.h). Deterministic, validated by round-trip on real samples.
 *
 * Approach (validated empirically on dcload-ip loader.bin/1st_read.bin):
 *
 *   - Disassembling the file is a POOR decision signal: the SH-4 opcode
 *     space is dense (~90% "valid" even on pure noise), so the ratio of
 *     valid opcodes barely changes between the scrambled and linear
 *     versions, and not always in the expected direction.
 *
 *   - The reliable signal is the VARIANCE of the per-block (256 bytes)
 *     Shannon entropy. A linear file (code plus distinct data sections)
 *     has high block variance because its regions differ. Scrambling mixes
 *     those regions at the 32-byte slice level, which evens out the local
 *     statistics and lowers the variance. On dcload-ip, the variance rose
 *     by 51% after the real descramble.
 *
 *   - So the variance is computed on the file as-is AND on the output of
 *     the REAL dc_descramble_buffer() (deterministic, not a guess). If the
 *     variance rises clearly after descrambling, the file was scrambled.
 *
 * NOTE: only applies to raw .BIN files (not ELF).
 */

#ifndef CHECK_H
#define CHECK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Block size for the local entropy variance */
#define BINCHECK_DEFAULT_ENTROPY_BLOCK   256u

/* Minimum relative variance increase (unscrambled vs as-is) needed to
 * conclude the file was scrambled. 0.15 = +15%. Validated on about 160
 * programs (KOS samples, dcload, older KOS releases, a NetBSD kernel), each
 * in both states: scrambled files rose by +31% (dcload-serial) to +696%,
 * unscrambled files dropped by -21% (dcload-serial) to -89%. */
#define BINCHECK_DEFAULT_VARIANCE_MARGIN 0.15

/* Minimum SH-4 code score (see sh4_score below) for the file to be
 * considered SH-4 code. Observed: 10.7 to 145 on raw Dreamcast binaries
 * (most above 45; the lowest ones embed large assets), about 12 on KOS ELF
 * files, and <= 0.85 on non-SH-4 files (x86 executables, ARM code, images,
 * fonts, text). */
#define BINCHECK_SH4_MIN_SCORE           2.0

typedef struct {
    bool     is_scrambled;          /* final verdict (variance based)       */
    double   variance_as_is;        /* per-block entropy variance, as-is    */
    double   variance_unscrambled;  /* per-block entropy variance, unscrambled */
    bool     is_elf;                /* file starts with the ELF magic       */
    bool     looks_like_sh4;        /* file appears to contain SH-4 code    */
    double   sh4_score;             /* "sts.l pr,@-r15" + "lds.l @r15+,pr"
                                       occurrences per 10,000 16-bit words
                                       (GCC function prologue/epilogue)    */
} bincheck_result_t;

typedef struct {
    uint32_t entropy_block;   /* block size for the entropy variance         */
    double   variance_margin; /* decision threshold (relative increase)      */
} bincheck_options_t;

/* Fill options with the default values */
void bincheck_options_init(bincheck_options_t *opts);

/* Analyze an in-memory buffer. Returns false on invalid arguments
 * (NULL buffer, zero size...). */
bool bincheck_analyze_buffer(const uint8_t *buf, size_t size,
                             const bincheck_options_t *opts,
                             bincheck_result_t *out_result);

/* Analyze a file on disk. Returns false on I/O error. */
bool bincheck_analyze_file(const char *filename,
                           const bincheck_options_t *opts,
                           bincheck_result_t *out_result);

#ifdef __cplusplus
}
#endif

#endif /* CHECK_H */
