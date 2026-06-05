/* KallistiOS ##version##

   gmon.c
   Copyright (C) 2025 Andress Barajas

   Portable gprof runtime core: histogram PC sampling, a binary-search-tree
   call graph, and a standard gmon.out writer. The architecture-specific
   function-entry hook lives in the per-arch backend (see gprof_internal.h);
   this file contains no CPU assumptions.

   The methods here are based on the principles outlined in the gprof profiling
   article below, except a binary search tree (BST) is used to store the call
   graph instead of a linked list.

   Article:
   (https://mcuoneclipse.com/2015/08/23/tutorial-using-gnu-profiling-gprof-with-arm-cortex-m/)

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gprof/gmon.h>
#include <kos/thread.h>
#include <kos/dbglog.h>
#include <arch/irq.h>

#include "gprof_internal.h"

#define ROUNDDOWN(x, y)  (((x) / (y)) * (y))
#define ROUNDUP(x, y)    ((((x) + (y) - 1) / (y)) * (y))

/* Histogram sampling interval, in milliseconds. */
#define HISTOGRAM_INTERVAL_MS  10

/* Sampling rate in samples/second, as stored in the gmon.out histogram
   header. gprof interprets this field as a frequency, not a period. */
#define PROF_HZ  (1000 / HISTOGRAM_INTERVAL_MS)

/* The type that holds the count for each histogram bucket. */
#define HIST_COUNTER_TYPE  uint16_t

/* Number of histogram buckets = text_size / HISTFRACTION */
#define HISTFRACTION  8

/* Hash-table reduction factor: froms[] slots = text_size / HASHFRACTION */
#define HASHFRACTION  16

/* Call-graph arc density as a percentage:
   number of arcs ~= (text_size * ARCDENSITY) / 100 */
#define ARCDENSITY  2

#define MAX_NODES  65535

#define GMON_MAGIC          "gmon"
#define GMON_VERSION        1
#define GMON_TAG_TIME_HIST  0
#define GMON_TAG_CG_ARC     1

/* gmon.out output location. The GMON_OUT_PREFIX environment variable redirects
   output to "<prefix>.<pid>"; when it is unset we fall back to the default path
   below. */
#define GMON_OUT_DEFAULT_PATH  "/pc/gmon.out"
#define GMON_OUT_PATH_MAX      512

/* gmon.out range symbols, supplied by the linker script. The explicit asm()
   labels name the linker symbols literally, sidestepping the leading-underscore
   user-label prefix that some architectures adds to C identifiers. */
extern char gprof_text_start __asm__("__executable_start");
extern char gprof_text_end   __asm__("__etext");

/* GMON file header */
typedef struct gmon_hdr {
    char cookie[4];
    int32_t version;
    char spare[3 * 4];
} gmon_hdr_t;

typedef struct gmon_hist_hdr {
    uintptr_t lowpc;
    uintptr_t highpc;
    uint32_t num_bins;
    uint32_t profrate;
    char units[15];
    char units_abbrev;
} gmon_hist_hdr_t;

/* GMON arc */
typedef struct gmon_arc {
    uintptr_t frompc;
    uintptr_t selfpc;
    size_t count;
} gmon_arc_t;

/* BST node */
typedef struct gmon_node {
    uintptr_t selfpc;
    uint32_t count;
    uint16_t left;
    uint16_t right;
} gmon_node_t;

/* GMON profiling states */
typedef enum gmon_state {
    GMON_PROF_ON,
    GMON_PROF_BUSY,
    GMON_PROF_ERROR,
    GMON_PROF_OFF
} gmon_state_t;

/* GMON context */
typedef struct gmon_context {
    gmon_state_t state;
    uintptr_t lowpc;
    uintptr_t highpc;
    size_t textsize;

    /* Array of indices to heads of BST */
    size_t nfroms;
    uint16_t *froms;

    /* BST */
    size_t nnodes;
    gmon_node_t *nodes;
    size_t node_count;

    size_t allocate_size;

    /* Histogram */
    size_t ncounters;
    HIST_COUNTER_TYPE *histogram;

    kthread_t *main_thread;
    kthread_t *histogram_thread;

    volatile bool running_thread;
    volatile bool initialized;
} gmon_context_t;

static gmon_context_t g_context = {
    .state = GMON_PROF_OFF,
    .lowpc = 0,
    .highpc = 0,
    .textsize = 0,
    .nfroms = 0,
    .froms = NULL,
    .nnodes = 0,
    .nodes = NULL,
    .node_count = 0,
    .ncounters = 0,
    .allocate_size = 0,
    .histogram = NULL,
    .main_thread = NULL,
    .histogram_thread = NULL,
    .running_thread = false,
    .initialized = false
};

/* Build the gmon.out path.If GMON_OUT_PREFIX is set, the file is "<prefix>.<pid>";
   otherwise the default path is used. */
static void gmon_build_out_path(char *buf, size_t buflen) {
    const char *prefix = getenv(GMON_OUT_PREFIX);

    if(prefix && *prefix)
        snprintf(buf, buflen, "%s.%d", prefix, getpid());
    else
        snprintf(buf, buflen, "%s", GMON_OUT_DEFAULT_PATH);
}

/* Function to write the arcs to gmon.out */
static void traverse_and_write(FILE *out, gmon_context_t *cxt, uint32_t index, uintptr_t from_addr) {
    gmon_arc_t arc;
    gmon_node_t *node;
    uint8_t tag = GMON_TAG_CG_ARC;

    if(index == 0)
        return;

    /* Grab a node */
    node = &cxt->nodes[index];

    /* Traverse the left subtree */
    traverse_and_write(out, cxt, node->left, from_addr);

    /* Write the arc */
    arc.frompc = from_addr;
    arc.selfpc = node->selfpc;
    arc.count = node->count;
    if(fwrite(&tag, sizeof(tag), 1, out) != 1 ||
       fwrite(&arc, sizeof(gmon_arc_t), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write arc for %p -> %p\n",
               (void *)from_addr, (void *)arc.selfpc);
        return;
    }

    /* Traverse the right subtree */
    traverse_and_write(out, cxt, node->right, from_addr);
}

static bool write_histogram(void) {
    gmon_context_t *cxt = &g_context;
    char out_path[GMON_OUT_PATH_MAX];
    FILE *out;
    uint8_t tag = GMON_TAG_TIME_HIST;
    gmon_hist_hdr_t hist_hdr;
    size_t written;

    gmon_build_out_path(out_path, sizeof(out_path));
    out = fopen(out_path, "a");
    if(!out) {
        dbglog(DBG_ERROR, "[GPROF] Failed to open %s for histogram.\n", out_path);
        return false;
    }

    /* Write Histogram Tag */
    if(fwrite(&tag, sizeof(tag), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write histogram tag.\n");
        fclose(out);
        return false;
    }

    /* Write Histogram Header */
    hist_hdr = (gmon_hist_hdr_t) {
        .lowpc = cxt->lowpc,
        .highpc = cxt->highpc,
        .num_bins = cxt->ncounters,
        .profrate = PROF_HZ,
        .units = "seconds",
        .units_abbrev = 's'
    };
    if(fwrite(&hist_hdr, sizeof(hist_hdr), 1, out) != 1) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write histogram header.\n");
        fclose(out);
        return false;
    }

    /* Write Histogram Data */
    written = fwrite(cxt->histogram, sizeof(HIST_COUNTER_TYPE), cxt->ncounters, out);
    if(written != cxt->ncounters) {
        dbglog(DBG_ERROR, "[GPROF] Failed to write complete histogram data (%zu/%zu bins).\n",
               written, cxt->ncounters);
        fclose(out);
        return false;
    }

    fclose(out);

    return true;
}

static bool write_arcs(void) {
    uintptr_t from_addr;
    uint32_t from_index;
    gmon_context_t *cxt = &g_context;
    char out_path[GMON_OUT_PATH_MAX];
    FILE *out;

    gmon_build_out_path(out_path, sizeof(out_path));
    out = fopen(out_path, "a");
    if(!out) {
        dbglog(DBG_ERROR, "[GPROF] Failed to open %s for arcs.\n", out_path);
        return false;
    }

    /* Write Arcs */
    for(from_index = 0; from_index < cxt->nfroms; from_index++) {
        /* Skip if no BST built for this index */
        if(cxt->froms[from_index] == 0)
            continue;

        /* Construct 'from' address by performing reciprocal of the hash used
           on insert. */
        from_addr = (HASHFRACTION * from_index) + cxt->lowpc;

        /* Write out the arcs in the BST */
        traverse_and_write(out, cxt, cxt->froms[from_index], from_addr);
    }

    fclose(out);

    return true;
}

/* Function that gets executed every HISTOGRAM_INTERVAL_MS */
static void *histogram_thread(void *arg) {
    (void)arg;
    uintptr_t pc;
    uint32_t index;
    gmon_context_t *cxt = &g_context;

    while(cxt->running_thread) {
        if(cxt->state == GMON_PROF_ON) {
            /* Grab the saved PC of the profiled (main) thread. */
            pc = CONTEXT_PC(cxt->main_thread->context);

            /* If the PC is within the profiled .text range... */
            if(pc >= cxt->lowpc && pc < cxt->highpc) {
                /* Compute the bucket and bump its counter. */
                index = (pc - cxt->lowpc) / HISTFRACTION;
                cxt->histogram[index]++;
            }
        }

        /* Always sleep before the next sample */
        thd_sleep(HISTOGRAM_INTERVAL_MS);
    }

    return NULL;
}

/* Function to enable/disable gprofiling */
void moncontrol(bool enable) {
    gmon_context_t *cxt = &g_context;

    /* Don't change the state if we ran into an error */
    if(cxt->state == GMON_PROF_ERROR)
        return;

    /* Start only if enabled and initialized */
    if(enable && cxt->initialized)
        cxt->state = GMON_PROF_ON;
    else
        cxt->state = GMON_PROF_OFF;
}

/* Flush gmon.out and tear down profiling (see gprof/gmon.h). */
void _mcleanup(void) {
    gmon_context_t *cxt = &g_context;

    /* Idempotent: nothing to do if profiling never started or was already
       cleaned up (e.g. a manual _mcleanup() followed by the atexit() one). */
    if(!cxt->initialized)
        return;

    /* Stop profiling, stop the histogram thread and join it. */
    moncontrol(false);
    gprof_arch_stop();
    cxt->running_thread = false;
    if(cxt->histogram_thread != NULL)
        thd_join(cxt->histogram_thread, NULL);

    /* Skip output if we encountered an error; still free and reset below. */
    if(cxt->state == GMON_PROF_ERROR)
        goto cleanup;

    if(!write_histogram())
        goto cleanup;

    if(cxt->nfroms > 0 && !write_arcs())
        goto cleanup;

cleanup:
    if(cxt->histogram) {
        free(cxt->histogram);
        cxt->histogram = NULL;
    }

    /* Reset context to its initial state for safety. */
    memset(cxt, 0, sizeof(g_context));

    /* Somewhat confusingly, ON=0, OFF=3 */
    cxt->state = GMON_PROF_OFF;
}

/* Called each time we enter a profiled function (from the arch backend). */
void gprof_mcount(uintptr_t frompc, uintptr_t selfpc) {
    uint32_t index;
    uint16_t node_index;
    uint16_t *index_ptr;
    gmon_node_t *node;
    size_t arc_buf_size;
    gmon_context_t *cxt = &g_context;

    /* Exit early if profiling isn't turned on. */
    if(cxt->state != GMON_PROF_ON)
        return;

    /* Exit early if the call site is outside the profiled .text range. */
    if(frompc < cxt->lowpc || frompc >= cxt->highpc)
        return;

    /* Calculate index into 'froms' array */
    index = (frompc - cxt->lowpc) / HASHFRACTION;
    index_ptr = &cxt->froms[index];

    /* Grab index into node array */
    node_index = *index_ptr;

    /* Node doesn't exist? */
    if(node_index == 0)
        goto create_node;

    /* Try to find the node */
    node = &cxt->nodes[node_index];

    while(true) {
        /* Found the node */
        if(node->selfpc == selfpc) {
            node->count++;
            return;
        }
        /* Check if selfpc is less */
        else if(selfpc < node->selfpc) {
            if(node->left == 0) {
                index_ptr = &node->left;
                goto create_node;
            }
            /* Visit left node */
            node = &cxt->nodes[node->left];
        }
        else {
            if(node->right == 0) {
                index_ptr = &node->right;
                goto create_node;
            }
            /* Visit right node */
            node = &cxt->nodes[node->right];
        }
    }

create_node:
    /* 'Allocate' a node */
    node_index = ++cxt->node_count;
    if(node_index >= cxt->nnodes)
        goto overflow;

    *index_ptr = node_index;
    node = &cxt->nodes[node_index];
    node->selfpc = selfpc;
    node->count = 1;
    return;

overflow:
    /* Pause profiling to flush the arcs we have so far. */
    cxt->state = GMON_PROF_OFF;

    if(!write_arcs()) {
        cxt->state = GMON_PROF_ERROR;
        _mcleanup();
        return;
    }

    /* Reset arcs only (keep the histogram). */
    arc_buf_size = cxt->allocate_size - (cxt->ncounters * sizeof(HIST_COUNTER_TYPE));
    memset(cxt->froms, 0, arc_buf_size);
    cxt->node_count = 0;

    /* Restart profiling */
    cxt->state = GMON_PROF_ON;
}

/* Weak no-op defaults for the arch backend hooks. An architecture that needs to
   arm/disarm its function-entry instrumentation overrides these with strong definitions.
   Architectures whose -pg instrumentation is a direct _mcount call need no hook and use
   these no-ops. */
void __weak_symbol gprof_arch_start(void) { }
void __weak_symbol gprof_arch_stop(void) { }

/* Sets up profiling buffers, the histogram thread, and the arch hook. */
void monstartup(uintptr_t lowpc, uintptr_t highpc) {
    size_t counter_size;
    size_t froms_size;
    size_t nodes_size;
    char out_path[GMON_OUT_PATH_MAX];
    gmon_hdr_t hdr;
    FILE *out;
    gmon_context_t *cxt = &g_context;

    /* Exit early if we already initialized. */
    if(cxt->initialized) {
        dbglog(DBG_NOTICE, "[GPROF] monstartup: Already initialized.\n");
        return;
    }

    cxt->lowpc = ROUNDDOWN(lowpc, HISTFRACTION * sizeof(HIST_COUNTER_TYPE));
    cxt->highpc = ROUNDUP(highpc, HISTFRACTION * sizeof(HIST_COUNTER_TYPE));
    cxt->textsize = cxt->highpc - cxt->lowpc;

    /* Number of histogram counters, rounded up so all of textsize is covered. */
    cxt->ncounters = (cxt->textsize + HISTFRACTION - 1) / HISTFRACTION;
    counter_size = cxt->ncounters * sizeof(HIST_COUNTER_TYPE);

    cxt->nfroms = (cxt->textsize + HASHFRACTION - 1) / HASHFRACTION;
    froms_size = cxt->nfroms * sizeof(uint16_t);

    cxt->nnodes = (cxt->textsize * ARCDENSITY) / 100;
    if(cxt->nnodes > MAX_NODES)
        cxt->nnodes = MAX_NODES;
    nodes_size = cxt->nnodes * sizeof(gmon_node_t);

    /* Allocate one block, padded so each sub-buffer can be 32-byte aligned. */
    cxt->allocate_size = ROUNDUP(counter_size, 32) +
                         ROUNDUP(froms_size, 32) +
                         ROUNDUP(nodes_size, 32) +
                         32; /* slack for alignment adjustments */

    dbglog(DBG_NOTICE, "[GPROF] Profiling from <%p to %p>\n"
                       "[GPROF] Range size: %zu bytes\n",
           (void *)cxt->lowpc, (void *)cxt->highpc, cxt->textsize);

    /* Arm the architecture-specific function-entry instrumentation. */
    gprof_arch_start();

    if(posix_memalign((void **)&cxt->histogram, 32, cxt->allocate_size)) {
        cxt->state = GMON_PROF_ERROR;
        dbglog(DBG_ERROR, "[GPROF] monstartup: Unable to allocate memory.\n");
        return;
    }

    dbglog(DBG_NOTICE, "[GPROF] Total memory allocated: %zu bytes\n\n",
           cxt->allocate_size);

    /* Clear the block and carve out 32-byte-aligned sub-buffers. */
    memset(cxt->histogram, 0, cxt->allocate_size);
    cxt->froms = (uint16_t *)((uintptr_t)cxt->histogram + ROUNDUP(counter_size, 32));
    cxt->nodes = (gmon_node_t *)((uintptr_t)cxt->froms + ROUNDUP(froms_size, 32));

    /* Create gmon.out and write the file header. */
    gmon_build_out_path(out_path, sizeof(out_path));
    out = fopen(out_path, "w");
    if(!out) {
        cxt->state = GMON_PROF_ERROR;
        dbglog(DBG_ERROR, "[GPROF] %s not opened.\n", out_path);
        free(cxt->histogram);
        cxt->histogram = NULL;
        return;
    }

    hdr = (gmon_hdr_t) {
        .cookie = { 'g', 'm', 'o', 'n' },
        .version = GMON_VERSION,
        .spare = { 0 }
    };
    if(fwrite(&hdr, sizeof(hdr), 1, out) != 1) {
        cxt->state = GMON_PROF_ERROR;
        dbglog(DBG_ERROR, "[GPROF] Failed to write GMON header.\n");
        fclose(out);
        free(cxt->histogram);
        cxt->histogram = NULL;
        return;
    }

    fclose(out);

    /* Spin up the histogram sampling thread. */
    cxt->running_thread = true;
    cxt->main_thread = thd_by_tid(KOS_PID);
    cxt->histogram_thread = thd_create(false, histogram_thread, NULL);
    if(cxt->histogram_thread == NULL) {
        cxt->running_thread = false;
        cxt->state = GMON_PROF_ERROR;
        dbglog(DBG_ERROR, "[GPROF] monstartup: Unable to create histogram thread.\n");
        free(cxt->histogram);
        cxt->histogram = NULL;
        return;
    }
    thd_set_prio(cxt->histogram_thread, PRIO_DEFAULT / 2);
    thd_set_label(cxt->histogram_thread, "histogram_thread");

    cxt->initialized = true;

    /* Write out gmon.out when the program exits. */
    atexit(_mcleanup);

    /* Turn profiling on. */
    moncontrol(true);
}

/* Strong override for the weak gprof_init() in arch_main; pulled into the link
   by the -pg instrumentation (MIPS) or by --whole-archive (SH4). Starts
   profiling over the whole program .text range. */
void gprof_init(void) {
    monstartup((uintptr_t)&gprof_text_start, (uintptr_t)&gprof_text_end);
}
