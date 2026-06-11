/* KallistiOS ##version##

   kos_gcov.c
   Copyright (C) 2026 Andress Barajas

   File-writing glue for GCC's freestanding libgcov, so KOS programs built with
   --coverage can emit .gcda on a KOS target.

   GCC's freestanding libgcov provides the coverage runtime and the .gcda serializer
   (__gcov_info_to_gcda), but it does not open or write files. kos_gcov_dump()
   walks the gcov_info section and writes each .gcda at its build-time path
   under /pc, honoring the GCOV_PREFIX / GCOV_PREFIX_STRIP environment variables.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <gcov.h>          /* GCC: struct gcov_info + __gcov_info_to_gcda() */
#include <kos/gcov.h>
#include <kos/dbglog.h>

/* Bounds of the gcov_info section -- one pointer per instrumented translation
   unit. Weak so that linking this addon into a program with no instrumented
   objects is harmless (the symbols resolve to NULL and the dump is a no-op). */
extern const struct gcov_info *__start_gcov_info[] __attribute__((weak));
extern const struct gcov_info *__stop_gcov_info[] __attribute__((weak));

/* Default output directory: /pc is the loader host filesystem. */
#define KOS_GCOV_DEFAULT_PREFIX  "/pc"
#define GCOV_FILEPATH_MAX        1024

/* Strip the first strip_count components from `path`, collapse redundant
   slashes, and write the result into `out`. */
static void strip_leading_dirs(const char *path, int strip_count, char *out, size_t out_size) {
    const char *p = path;
    size_t i;
    bool last_was_slash = false;     /* Flag to track consecutive slashes */

    /* Skip any leading slashes */
    while(*p == '/') p++;

    /* Strip the specified number of path components */
    while(strip_count-- > 0) {
        /* Skip over one component (up to the next '/') */
        while(*p && *p != '/') p++;

        /* Skip the slash separator(s) */
        while(*p == '/') p++;
    }

    /* Copy the remaining path, collapsing multiple slashes into one */
    for(i = 0; *p && i < out_size - 1; ++p) {
        if(*p == '/') {
            if(last_was_slash) continue; /* Skip redundant slashes */
            last_was_slash = true;
        }
        else
            last_was_slash = false;

        out[i++] = *p;
    }

    /* Null terminate */
    out[i] = '\0';
}

/* Build the final .gcda path by combining GCOV_PREFIX (output directory, default
   /pc) with a GCOV_PREFIX_STRIP-stripped copy of the path GCC recorded at build
   time. */
static void gcov_build_filepath(const char *src_path, char *out_path) {
    /* GCOV_PREFIX_STRIP is a count of leading path components to drop. */
    const char *strip_str = getenv(GCOV_PREFIX_STRIP);
    int prefix_strip = strip_str ? atoi(strip_str) : 0;
    if(prefix_strip < 0)
        prefix_strip = 0;

    const char *prefix = getenv(GCOV_PREFIX);
    if(!prefix)
        prefix = KOS_GCOV_DEFAULT_PREFIX;

    /* Copy the prefix (minus one trailing slash) into out_path, clamped. */
    size_t prefix_len = strlen(prefix);
    if(prefix_len > 0 && prefix[prefix_len - 1] == '/')
        prefix_len--;
    if(prefix_len > GCOV_FILEPATH_MAX - 1)
        prefix_len = GCOV_FILEPATH_MAX - 1;
    memcpy(out_path, prefix, prefix_len);

    size_t pos = prefix_len;
    if(pos > 0 && pos < GCOV_FILEPATH_MAX - 1)
        out_path[pos++] = '/';   /* separator only when there is a prefix */

    /* Append the stripped source path into the room that remains. */
    strip_leading_dirs(src_path, prefix_strip, out_path + pos, GCOV_FILEPATH_MAX - pos);
}

/* __gcov_info_to_gcda() serializes one gcov_info into the .gcda byte stream and
   drives it through three callbacks we supply, plus an opaque `arg` threaded
   through all of them. We pass `arg = &(FILE *)` so the handle opened by the
   filename callback flows to the dump callback. */

/* filename callback. Called once with the absolute .gcda path GCC recorded at
   build time. We remap it under GCOV_PREFIX / GCOV_PREFIX_STRIP and open the
   file, stashing the handle in `arg` for gcda_write() to use. */
static void gcda_open(const char *path, void *arg) {
    FILE **fp = arg;
    char out[GCOV_FILEPATH_MAX];

    gcov_build_filepath(path, out);
    *fp = fopen(out, "wb");
    if(!*fp)
        dbglog(DBG_ERROR, "[GCOV]: cannot open %s\n", out);
}

/* dump callback. Called repeatedly with successive chunks of the serialized
   .gcda; we append each chunk to the file opened above (skipped if the open
   failed and left the handle NULL). */
static void gcda_write(const void *data, unsigned int len, void *arg) {
    FILE *f = *(FILE **)arg;
    if(f)
        fwrite(data, 1, len, f);
}

/* allocate callback. The serializer asks us for scratch buffers while building
   the .gcda image; hand back heap memory (libgcov frees it). */
static void *gcda_alloc(unsigned int len, void *arg) {
    (void)arg;

    return malloc(len);
}

void kos_gcov_dump(void) {
    const struct gcov_info **i;

    if(!__start_gcov_info || !__stop_gcov_info)
        return;

    for(i = __start_gcov_info; i < __stop_gcov_info; i++) {
        FILE *f = NULL;
        __gcov_info_to_gcda(*i, gcda_open, gcda_write, gcda_alloc, &f);
        if(f)
            fclose(f);
    }
}

/* Auto-register the flush at startup, so coverage is written at exit with no
   source changes. */
__attribute__((constructor))
static void kos_gcov_autostart(void) {
    dbglog(DBG_NOTICE, "[GCOV]: coverage instrumentation active\n");

    atexit(kos_gcov_dump);
}
