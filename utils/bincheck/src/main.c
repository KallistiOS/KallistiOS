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
 * bincheck - scrambled/unscrambled detector for Dreamcast binaries
 * (.BIN only). Actually descrambles the file and compares the per-block
 * entropy variance before and after.
 */

#include "check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Normally set by the Makefile (VERSION) */
#ifndef BINCHECK_VERSION
#define BINCHECK_VERSION "unknown"
#endif

/* Exit codes, for use in scripts (like grep: 0/1 for the verdict, 2 for an
 * error). The verdict codes are swapped with -i. */
#define EXIT_SCRAMBLED    0
#define EXIT_UNSCRAMBLED  1
#define EXIT_ERROR        2

/* Extract the program name from argv[0]: strip the directory (both '/' and
 * '\' separators, for Windows) and the extension (e.g. ".exe"). */
static const char *
program_name(const char *argv0)
{
    static char name[256];
    const char *base = argv0;
    const char *p;
    char *dot;

    for (p = argv0; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }

    strncpy(name, base, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    dot = strrchr(name, '.');
    if (dot && dot != name) *dot = '\0';

    return name[0] ? name : "bincheck";
}

static void
usage(const char *argv0)
{
    const char *prog = program_name(argv0);

    fprintf(stderr,
        "Sega Dreamcast Binary Checker (bincheck) v%s\n\n"
        "Usage: %s [options] <file.bin>\n\n"
        "Options:\n"
        "  -b <bytes>     Block size for the entropy variance (default: %u)\n"
        "  -m <margin>    Decision threshold, relative increase (default: %.2f)\n"
        "  -i             Invert the exit code (scrambled: %d, unscrambled: %d)\n"
        "  -v             Verbose mode (show details)\n"
        "  -h             Show this help\n"
        "  --version      Show the version\n\n"
        "Exit status:\n"
        "  %d if the file is scrambled, %d if it is unscrambled (swapped with -i),\n"
        "  %d if an error occurred.\n",
        BINCHECK_VERSION, prog, BINCHECK_DEFAULT_ENTROPY_BLOCK,
        BINCHECK_DEFAULT_VARIANCE_MARGIN,
        EXIT_UNSCRAMBLED, EXIT_SCRAMBLED,
        EXIT_SCRAMBLED, EXIT_UNSCRAMBLED, EXIT_ERROR);
}

int
main(int argc, char **argv)
{
    bincheck_options_t opts;
    bincheck_result_t result;
    const char *filename = NULL;
    int verbose = 0;
    int invert = 0;
    int i;

    bincheck_options_init(&opts);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("Sega Dreamcast Binary Checker (bincheck) v%s\n", BINCHECK_VERSION);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            invert = 1;
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            opts.entropy_block = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            opts.variance_margin = strtod(argv[++i], NULL);
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n\n", argv[i]);
            usage(argv[0]);
            return EXIT_ERROR;
        }
    }

    if (!filename) {
        usage(argv[0]);
        return EXIT_ERROR;
    }

    if (!bincheck_analyze_file(filename, &opts, &result)) {
        fprintf(stderr, "Error: unable to analyze \"%s\"\n", filename);
        return EXIT_ERROR;
    }

    if (verbose) {
        printf("File                         : %s\n", filename);
        printf("Entropy variance, as-is      : %.4f\n", result.variance_as_is);
        printf("Entropy variance, unscrambled: %.4f\n", result.variance_unscrambled);
        printf("Threshold (relative increase): +%.0f%%\n", opts.variance_margin * 100.0);
        printf("SH-4 code score              : %.2f (minimum: %.2f)\n",
               result.sh4_score, BINCHECK_SH4_MIN_SCORE);
        printf("---\n");
    }

    fflush(stdout);
    if (result.is_elf) {
        fprintf(stderr, "Warning: \"%s\" is an ELF file; bincheck only works on "
                        "raw binary files so this result is meaningless\n",
                filename);
    } else if (!result.looks_like_sh4) {
        fprintf(stderr, "Warning: \"%s\" does not look like SH-4 code (Dreamcast "
                        "program) so this result is probably meaningless\n",
                filename);
    }
    fflush(stderr);

    printf("%s: %s\n", filename, result.is_scrambled ? "Scrambled" : "Unscrambled");

    if (invert)
        return result.is_scrambled ? EXIT_UNSCRAMBLED : EXIT_SCRAMBLED;
    return result.is_scrambled ? EXIT_SCRAMBLED : EXIT_UNSCRAMBLED;
}
